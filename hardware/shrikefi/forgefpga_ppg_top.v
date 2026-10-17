`timescale 1ns / 1ps

(* top *)
module forgefpga_ppg_top #(
    parameter integer CLK_FREQ_HZ    = 50_000_000,
    parameter integer REFRACTORY_CYC = 12_500_000,  // 250ms blanking at 50MHz
    parameter integer LED_PULSE_CYC  = 2_500_000,   // 50ms LED pulse at 50MHz
    parameter integer POR_CYC        = 255          // clocks reset is held after power-up
)(
    // System Clock
    (* iopad_external_pin, clkbuf_inhibit *) input  wire        clk,             // 50MHz system clock (OSC_CLK resource)
    (* iopad_external_pin *)                 output wire        clk_en,          // OSC_EN - MUST be driven or the core has NO CLOCK

    // 4-Wire SPI Target Interface to ESP32-S3 (assignments in forgefpga_pins.pcf)
    (* iopad_external_pin *) input  wire        spi_sck,         // FPGA PIN_16 <- ESP32 GPIO12
    (* iopad_external_pin *) input  wire        spi_ss_n,        // FPGA PIN_17 <- ESP32 GPIO10
    (* iopad_external_pin *) input  wire        spi_mosi,        // FPGA PIN_18 <- ESP32 GPIO11
    (* iopad_external_pin *) output wire        spi_miso,        // FPGA PIN_19 -> ESP32 GPIO13
    (* iopad_external_pin *) output wire        spi_miso_oe,     // output enable for PIN_19

    // Observable Hardware Output
    (* iopad_external_pin *) output reg         led_user,        // FPGA PIN_7: blue user LED D12
    (* iopad_external_pin *) output wire        led_user_oe      // output enable for PIN_7
);

    // -------------------------------------------------------------------------
    // Clock enable and output enables.
    // The Vicharak ForgeFPGA requires an explicit enable for the oscillator and
    // for every output pad. Without clk_en the configured design has NO CLOCK;
    // without led_user_oe the LED pad is never driven, so the design is silent
    // even when it is running correctly.
    // -------------------------------------------------------------------------
    assign clk_en      = 1'b1;
    assign led_user_oe = 1'b1;
    assign spi_miso_oe = 1'b1;

    // -------------------------------------------------------------------------
    // Internal power-on reset
    // -------------------------------------------------------------------------
    // rst_n was a top-level input mapped to FPGA PIN_13, but the Shrike-Fi
    // ESP32<->FPGA interconnect carries only EN, PWR, SCLK, SS, MOSI and MISO --
    // NOTHING drives PIN_13, so rst_n cannot be an input.
    //
    // It must not be tied to 1'b1 either. rst_n gates every register in this
    // file, so tying it high leaves the SPI shifter, the filter and the peak
    // detector FSM with no defined start state: they would depend entirely on
    // whatever the fabric happens to power up holding. Under simulation that is
    // fatal -- every register starts as X, the FSM's `case` matches nothing, and
    // the design never produces a single output -- and on hardware it is a
    // latent intermittent-boot-failure risk.
    //
    // Reset is therefore generated here: hold low for POR_CYC clocks, then
    // release. por_cnt is initialised at its declaration, which both simulators
    // and the ForgeFPGA configuration flow treat as the register's power-up
    // value. POR_CYC is a parameter so a testbench can shorten the wait.
    // -------------------------------------------------------------------------
    reg [7:0] por_cnt = 8'd0;
    localparam [7:0] POR_LAST = POR_CYC[7:0];

    wire rst_n = (por_cnt == POR_LAST);

    always @(posedge clk) begin
        if (por_cnt != POR_LAST) begin
            por_cnt <= por_cnt + 8'd1;
        end
    end

    // -------------------------------------------------------------------------
    // 1. SPI Target Submodule
    // -------------------------------------------------------------------------
    wire [7:0] rx_data;
    wire       rx_valid;
    reg  [7:0] tx_data;
    wire       spi_miso_oe_internal;
    wire       tx_data_hold_internal;
    wire       beat_raw;
    reg        beat_latched;
    wire [7:0] filt_sample;
    wire       filt_valid;
    wire [31:0] ibi_val;

    // IBI burst state (see section 4). Declared here because the filter below
    // needs sample_valid to ignore the dummy MOSI bytes of a burst.
    reg  [2:0]  burst_cnt;
    reg  [31:0] ibi_latched;
    wire        sample_valid = rx_valid && (burst_cnt == 3'd0);

    spi_target #(
        .WIDTH(8)
    ) u_spi_target (
        .i_clk(clk),
        .i_rst_n(rst_n),
        .i_enable(1'b1),
        .i_ss_n(spi_ss_n),
        .i_sck(spi_sck),
        .i_mosi(spi_mosi),
        .o_miso(spi_miso),
        .o_miso_oe(spi_miso_oe_internal),
        .o_rx_data(rx_data),
        .o_rx_data_valid(rx_valid),
        .i_tx_data(tx_data),
        .o_tx_data_hold(tx_data_hold_internal)
    );

    // -------------------------------------------------------------------------
    // 2. 8-Tap Moving Average Filter
    // -------------------------------------------------------------------------
    moving_average_8tap #(
        .DATA_WIDTH(8)
    ) u_ma_filter (
        .clk(clk),
        .rst_n(rst_n),
        .data_valid(sample_valid),
        .data_in(rx_data),
        .data_out(filt_sample),
        .out_valid(filt_valid)
    );

    // -------------------------------------------------------------------------
    // 3. Systolic Peak Detector & IBI Timing
    // -------------------------------------------------------------------------
    ppg_peak_detector #(
        .DATA_WIDTH(8),
        .REFRACTORY_CYC(REFRACTORY_CYC),
        .DEFAULT_THRESH(8'd120)
    ) u_peak_det (
        .clk(clk),
        .rst_n(rst_n),
        .sample_valid(filt_valid),
        .sample_in(filt_sample),
        .dyn_threshold(8'd120),
        .beat_detected(beat_raw),
        .ibi_cycles(ibi_val)
    );

    // -------------------------------------------------------------------------
    // 4. Beat Latch, IBI Burst & SPI Response Register
    // -------------------------------------------------------------------------
    // Normal packet (one per sample):
    //   Bit 7: Beat detected flag (1 = systolic crest)
    //   Bits [6:0]: 7-bit filtered PPG amplitude
    //
    // On a beat the ESP32 reads FIVE extra bytes, which this block queues:
    //   byte 0: 8'hC3 magic
    //   bytes 1-4: the 32-bit IBI counter, MSB first (50 MHz cycles = 20 ns)
    //
    // The IBI is measured by the peak detector on the same clock as the beat, so
    // it is cycle-accurate. The old link exposed only the beat flag, forcing the
    // ESP32 to time beats at its SPI poll rate (~10 ms quantisation) - a third
    // of a 20-60 ms RSA signal. The five burst transfers carry dummy MOSI bytes;
    // sample_valid keeps them out of the filter.
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            beat_latched <= 1'b0;
            burst_cnt    <= 3'd0;
            ibi_latched  <= 32'd0;
            // Reset value only. tx_data is overwritten on the very next clock,
            // so this byte never reaches MISO -- do not use it as a liveness
            // signature in firmware.
            tx_data      <= 8'hA5;
        end else begin
            if (beat_raw) begin
                beat_latched <= 1'b1;
                ibi_latched  <= ibi_val;
            end else if (rx_valid && burst_cnt == 3'd0) begin
                // Clear the beat latch once it has been transmitted.
                beat_latched <= 1'b0;
            end

            if (rx_valid) begin
                if (burst_cnt == 3'd0) begin
                    if (beat_latched) begin
                        // This transfer was the beat byte; queue the burst.
                        burst_cnt <= 3'd5;
                        tx_data   <= 8'hC3;
                    end else begin
                        tx_data <= {beat_latched, filt_sample[6:0]};
                    end
                end else begin
                    burst_cnt <= burst_cnt - 3'd1;
                    case (burst_cnt)
                        3'd5: tx_data <= ibi_latched[31:24];
                        3'd4: tx_data <= ibi_latched[23:16];
                        3'd3: tx_data <= ibi_latched[15:8];
                        3'd2: tx_data <= ibi_latched[7:0];
                        default: tx_data <= {beat_latched, filt_sample[6:0]};
                    endcase
                end
            end else if (burst_cnt == 3'd0) begin
                // Keep the normal reply current between transfers, but never
                // overwrite a burst byte that is waiting to be sent.
                tx_data <= {beat_latched, filt_sample[6:0]};
            end
        end
    end

    // -------------------------------------------------------------------------
    // 5. Observable Output: Pulse Stretcher for User LED (Package PIN_7 / Macro GPIO16)
    // -------------------------------------------------------------------------
    // Hardware Circuit is Active-HIGH (Schematic Sheet 5: Pin 7 -> R16 -> D12 Anode -> GND):
    //   1'b0 = Pin LOW  (0.0V) -> LED is OFF (Dark between heartbeats)
    //   1'b1 = Pin HIGH (3.3V) -> LED is ON  (Flashes bright blue on systolic peak)
    reg [23:0] led_timer;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            led_timer <= 24'd0;
            led_user  <= 1'b0;  // Power-on / reset: LED dark
        end else if (beat_raw) begin
            led_timer <= LED_PULSE_CYC[23:0];
            led_user  <= 1'b1;  // Heartbeat detected: flash LED bright blue!
        end else if (led_timer > 24'd0) begin
            led_timer <= led_timer - 24'd1;
            led_user  <= 1'b1;  // Hold flash for duration of pulse
        end else begin
            led_user  <= 1'b0;  // Idle: LED dark
        end
    end

endmodule
