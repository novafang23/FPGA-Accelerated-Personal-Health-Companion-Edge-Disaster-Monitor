// =============================================================================
// File: forgefpga_ppg_top.v
// Module: forgefpga_ppg_top
// Project: SIH26181 Health Companion & Disaster Monitor
// Target: Renesas ForgeFPGA (SLG47910) / Vicharak Shrike-Fi Board
// Description:
//   Hardware accelerator for photoplethysmography (PPG).
//   Interfaces with ESP32-S3 over full-duplex 4-wire SPI bus (Pins 3, 4, 5, 6).
//   Drives onboard Blue User LED on Pin 16 on each detected heartbeat.
//
// Official Vicharak Shrike-Fi ForgeFPGA Architecture Implementation:
//   1. Clocking: 'clk' (OSC_CLK) and 'clk_en' (OSC_EN = 1'b1) for internal 50MHz oscillator.
//   2. Reset: Internally generated power-on reset (avoids floating undriven external pins).
//   3. User LED: 'led_user' (GPIO3_OUT / Pin 16) and 'led_user_oe' (GPIO3_OE / Pin 16 = 1'b1).
//   4. SPI Target: spi_sck (PIN 3), spi_ss_n (PIN 4), spi_mosi (PIN 5),
//                  spi_miso (PIN 6), spi_miso_oe (PIN 6).
//
// MAINTENANCE NOTICE:
//   This file is the authoritative, unified hardware accelerator module compiled
//   by Renesas Go Configure Software Hub for the ForgeFPGA SLG47910. The modular
//   files in hardware/common/ (moving_average_8tap.v, ppg_peak_detector.v) are
//   maintained for unit simulation testbenches.
// =============================================================================

`timescale 1ns / 1ps

(* top *)
module forgefpga_ppg_top #(
    parameter integer CLK_FREQ_HZ    = 50_000_000,
    parameter integer REFRACTORY_CYC = 12_500_000,  // 250ms blanking at 50MHz
    parameter integer LED_PULSE_CYC  = 2_500_000    // 50ms LED pulse at 50MHz
)(
    // System Clock
    (* iopad_external_pin, clkbuf_inhibit *) input  wire        clk,             // 50MHz system clock (OSC_CLK resource)
    (* iopad_external_pin *)                 output wire        clk_en,          // OSC_EN - MUST be driven or the core has NO CLOCK

    // 4-Wire SPI Target Interface to ESP32-S3 (Pins 3, 4, 5, 6)
    (* iopad_external_pin *) input  wire        spi_sck,         // Pin 3 (ESP32 GPIO 12 - SPI SCK)
    (* iopad_external_pin *) input  wire        spi_ss_n,        // Pin 4 (ESP32 GPIO 10 - SPI CS)
    (* iopad_external_pin *) input  wire        spi_mosi,        // Pin 5 (ESP32 GPIO 11 - SPI MOSI)
    (* iopad_external_pin *) output wire        spi_miso,        // Pin 19 (ESP32 GPIO 13 - SPI MISO / GPIO6)
    (* iopad_external_pin *) output wire        spi_miso_oe,     // Output enable for MISO pad (Pin 19 / GPIO6_OE)

    // Observable Hardware Output
    (* iopad_external_pin *) output reg         led_user,        // Pin 16: Blue User LED (heartbeat flash)
    (* iopad_external_pin *) output wire        led_user_oe      // GPIO16_OE - MUST be high or the LED pad is never driven
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
    // Internal reset.
    // rst_n was a top-level input mapped to FPGA PIN_13, but the Shrike-Fi
    // ESP32<->FPGA interconnect carries only EN, PWR, SCLK, SS, MOSI and MISO -
    // NOTHING drives PIN_13. Since rst_n gates every always block in this file,
    // an undriven reset pin holds the entire design in reset. Reset is therefore
    // sourced internally; the device's own power-on reset handles initialisation.
    // -------------------------------------------------------------------------
    wire rst_n = 1'b1;

    // -------------------------------------------------------------------------
    // 1. SPI Target Submodule
    // -------------------------------------------------------------------------
    wire [7:0] rx_data;
    wire       rx_valid;
    reg  [7:0] tx_data;
    wire       beat_raw;
    reg        beat_latched;
    wire [7:0] filt_sample;
    wire       filt_valid;
    wire [31:0] ibi_val;

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
        .o_miso_oe(),
        .o_rx_data(rx_data),
        .o_rx_data_valid(rx_valid),
        .i_tx_data(tx_data),
        .o_tx_data_hold()
    );

    // -------------------------------------------------------------------------
    // 2. 8-Tap Moving Average Filter
    // -------------------------------------------------------------------------
    moving_average_8tap #(
        .DATA_WIDTH(8)
    ) u_ma_filter (
        .clk(clk),
        .rst_n(rst_n),
        .data_valid(rx_valid),
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
    // 4. Beat Latch & SPI Response Register
    // -------------------------------------------------------------------------
    // Packet returned to ESP32:
    // Bit 7: Beat detected flag (1 = systolic crest)
    // Bits [6:0]: 7-bit filtered PPG amplitude
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            beat_latched <= 1'b0;
            tx_data      <= 8'hA5; // Distinct hardware-alive signature
        end else begin
            if (beat_raw) begin
                beat_latched <= 1'b1;
            end else if (rx_valid) begin
                // Clear beat latch once transmitted over SPI
                beat_latched <= 1'b0;
            end

            tx_data <= {beat_latched, filt_sample[6:0]};
        end
    end

    // -------------------------------------------------------------------------
    // 5. Observable Output: Pulse Stretcher for User LED (Pin 16)
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

// ============================================================================
// Submodule 1: Vicharak SPI Target (Slave) for Renesas ForgeFPGA
// ============================================================================

module spi_target #(
    parameter CPOL = 1'b0,
    parameter CPHA = 1'b0,
    parameter WIDTH = 8,
    parameter LSB = 1'b0
)(
    input  wire             i_clk,
    input  wire             i_rst_n,
    input  wire             i_enable,
    input  wire             i_ss_n,
    input  wire             i_sck,
    input  wire             i_mosi,
    output reg              o_miso,
    output reg              o_miso_oe,
    output reg  [WIDTH-1:0] o_rx_data,
    output reg              o_rx_data_valid,
    input  wire [WIDTH-1:0] i_tx_data,
    output reg              o_tx_data_hold
);
    localparam [2:0] BIT_MAX = WIDTH[2:0] - 3'd1;

    reg [1:0] sck_sync;
    reg [1:0] ss_sync;
    reg [2:0] bit_cnt;
    reg [WIDTH-1:0] rx_shift;
    reg [WIDTH-1:0] tx_shift;

    wire sck_r = (sck_sync == 2'b01);
    wire sck_f = (sck_sync == 2'b10);
    wire ss_n  = ss_sync[1];

    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            sck_sync <= 2'b00;
            ss_sync  <= 2'b11;
        end else begin
            sck_sync <= {sck_sync[0], i_sck};
            ss_sync  <= {ss_sync[0], i_ss_n};
        end
    end

    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            bit_cnt         <= 3'd0;
            rx_shift        <= {WIDTH{1'b0}};
            tx_shift        <= {WIDTH{1'b0}};
            o_rx_data       <= {WIDTH{1'b0}};
            o_rx_data_valid <= 1'b0;
            o_miso          <= 1'b0;
            o_miso_oe       <= 1'b0;
            o_tx_data_hold  <= 1'b0;
        end else begin
            o_rx_data_valid <= 1'b0;
            if (ss_n) begin
                bit_cnt   <= 3'd0;
                tx_shift  <= i_tx_data;
                o_miso    <= (LSB) ? i_tx_data[0] : i_tx_data[WIDTH-1];
                o_miso_oe <= 1'b0;
            end else if (i_enable) begin
                o_miso_oe <= 1'b1;
                if (sck_r) begin
                    if (LSB)
                        rx_shift <= {i_mosi, rx_shift[WIDTH-1:1]};
                    else
                        rx_shift <= {rx_shift[WIDTH-2:0], i_mosi};

                    bit_cnt <= bit_cnt + 3'd1;

                    if (bit_cnt == BIT_MAX) begin
                        if (LSB)
                            o_rx_data <= {i_mosi, rx_shift[WIDTH-1:1]};
                        else
                            o_rx_data <= {rx_shift[WIDTH-2:0], i_mosi};
                        o_rx_data_valid <= 1'b1;
                    end
                end

                if (sck_f) begin
                    if (LSB) begin
                        o_miso   <= tx_shift[1];
                        tx_shift <= {1'b0, tx_shift[WIDTH-1:1]};
                    end else begin
                        o_miso   <= tx_shift[WIDTH-2];
                        tx_shift <= {tx_shift[WIDTH-2:0], 1'b0};
                    end
                end
            end
        end
    end

endmodule

// ============================================================================
// Submodule 2: 8-Tap Moving Average Filter
// ============================================================================

module moving_average_8tap #(
    parameter DATA_WIDTH = 8
)(
    input  wire                   clk,
    input  wire                   rst_n,
    input  wire                   data_valid,
    input  wire [DATA_WIDTH-1:0]  data_in,
    output reg  [DATA_WIDTH-1:0]  data_out,
    output reg                    out_valid
);
    reg [DATA_WIDTH-1:0] shift_reg [0:7];
    reg [DATA_WIDTH+2:0] running_sum;
    integer i;

    wire [DATA_WIDTH+2:0] next_sum = running_sum + {3'b000, data_in} - {3'b000, shift_reg[7]};

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            running_sum <= {(DATA_WIDTH+3){1'b0}};
            data_out    <= {DATA_WIDTH{1'b0}};
            out_valid   <= 1'b0;
            for (i = 0; i < 8; i = i + 1) begin
                shift_reg[i] <= {DATA_WIDTH{1'b0}};
            end
        end else if (data_valid) begin
            shift_reg[0] <= data_in;
            for (i = 1; i < 8; i = i + 1) begin
                shift_reg[i] <= shift_reg[i-1];
            end
            running_sum <= next_sum;
            data_out    <= next_sum[DATA_WIDTH+2:3];
            out_valid   <= 1'b1;
        end else begin
            out_valid   <= 1'b0;
        end
    end

endmodule

// ============================================================================
// Submodule 3: Systolic Peak Detector with 250ms Refractory Blanking
// ============================================================================

module ppg_peak_detector #(
    parameter DATA_WIDTH      = 8,
    parameter REFRACTORY_CYC  = 12_500_000, // 250ms at 50MHz
    parameter DEFAULT_THRESH  = 8'd120
)(
    input  wire                   clk,
    input  wire                   rst_n,
    input  wire                   sample_valid,
    input  wire [DATA_WIDTH-1:0]  sample_in,
    input  wire [DATA_WIDTH-1:0]  dyn_threshold,
    output reg                    beat_detected,
    output reg  [31:0]            ibi_cycles
);
    localparam STATE_ARMED      = 2'b00;
    localparam STATE_RISING     = 2'b01;
    localparam STATE_PEAK_FOUND = 2'b10;
    localparam STATE_REFRACTORY = 2'b11;

    reg [1:0]  current_state, next_state;
    reg [DATA_WIDTH-1:0] prev_sample;
    reg [31:0] refractory_cnt;
    reg [31:0] interval_cnt;
    reg        first_beat_seen;
    reg [1:0]  fall_count;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current_state   <= STATE_ARMED;
            prev_sample     <= {DATA_WIDTH{1'b0}};
            refractory_cnt  <= 32'd0;
            interval_cnt    <= 32'd0;
            ibi_cycles      <= 32'd0;
            beat_detected   <= 1'b0;
            first_beat_seen <= 1'b0;
            fall_count      <= 2'd0;
        end else begin
            current_state <= next_state;

            if (interval_cnt != 32'hFFFF_FFFF) begin
                interval_cnt <= interval_cnt + 32'd1;
            end

            if (sample_valid) begin
                prev_sample <= sample_in;
            end

            case (current_state)
                STATE_ARMED: begin
                    beat_detected <= 1'b0;
                    fall_count    <= 2'd0;
                end
                STATE_RISING: begin
                    beat_detected <= 1'b0;
                    if (sample_valid) begin
                        if (sample_in < prev_sample)
                            fall_count <= fall_count + 2'd1;
                        else
                            fall_count <= 2'd0;
                    end
                end
                STATE_PEAK_FOUND: begin
                    if (first_beat_seen) begin
                        beat_detected <= 1'b1;
                        ibi_cycles    <= interval_cnt;
                    end else begin
                        beat_detected   <= 1'b0;
                        first_beat_seen <= 1'b1;
                    end
                    interval_cnt   <= 32'd0;
                    refractory_cnt <= REFRACTORY_CYC[31:0];
                end
                STATE_REFRACTORY: begin
                    beat_detected <= 1'b0;
                    if (refractory_cnt > 32'd0)
                        refractory_cnt <= refractory_cnt - 32'd1;
                end
            endcase
        end
    end

    always @(*) begin
        next_state = current_state;
        case (current_state)
            STATE_ARMED: begin
                if (sample_valid && (sample_in >= dyn_threshold))
                    next_state = STATE_RISING;
            end
            STATE_RISING: begin
                if (sample_valid && (sample_in < prev_sample) && (fall_count >= 2'd1))
                    next_state = STATE_PEAK_FOUND;
            end
            STATE_PEAK_FOUND: begin
                next_state = STATE_REFRACTORY;
            end
            STATE_REFRACTORY: begin
                if (refractory_cnt == 32'd0 && sample_valid && (sample_in < dyn_threshold))
                    next_state = STATE_ARMED;
            end
            default: next_state = STATE_ARMED;
        endcase
    end

endmodule
