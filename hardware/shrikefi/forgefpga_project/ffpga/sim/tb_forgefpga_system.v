// =============================================================================
// GENERATED FILE -- DO NOT EDIT
//
// Synced from hardware/shrikefi/tb_forgefpga_system.v by
// hardware/shrikefi/gen_flat_source.py. It exists only because the Renesas
// project file references this path. Edit the original, then re-run the
// generator.
// =============================================================================

`timescale 1ns / 1ps

// =============================================================================
// tb_forgefpga_system.v
// Module: tb_forgefpga_system
// Project: SIH26181 Health Companion & Disaster Monitor
// Target: Renesas ForgeFPGA (SLG47910) / Vicharak Shrike-Fi board
//
// Self-checking testbench for forgefpga_ppg_top. It drives the DUT the way the
// ESP32-S3 firmware in firmware/shrikefi/shrikefi_link_driver.c actually drives
// it -- not the way an older 4-bit parallel protocol used to:
//
//   * 8-bit SPI, mode 0 (CPOL=0, CPHA=0), MSB first, CS asserted manually.
//   * One transfer per PPG sample; MOSI carries the raw 8-bit sample.
//   * MISO returns {beat_latched, filt_sample[6:0]}.
//   * The FPGA's reply for transfer k+1 carries the result of the sample sent
//     in transfer k (one transfer of pipeline latency: the filter, the beat
//     latch and the SPI transmit shift register each cost a clock).
//
// Two properties of the reply shape what can be checked through it:
//
//   * Bit 7 is the beat flag, so only the LOW SEVEN bits of the 8-tap average
//     come back. The filter tests therefore keep their stimulus below 128 and
//     the 7-bit wrap never comes into play. The FPGA's own detector uses the
//     full 8-bit average internally, so detection is unaffected.
//   * The RTL exposes ibi_cycles, but no SPI register carries it and the
//     firmware does not read it -- it times beats with esp_timer_get_time().
//     IBI accuracy is therefore checked here the way the product measures it:
//     transfers between consecutive beat flags.
//
// Run: hardware/shrikefi/build_shrikefi_sim.bat (same command as CI)
// =============================================================================

module tb_forgefpga_system;

    // -------------------------------------------------------------------------
    // Timing
    // -------------------------------------------------------------------------
    localparam CLK_PERIOD_NS  = 20;      // 50 MHz core clock
    localparam SCK_HALF_NS    = 100;     // SCK half period -> 5 MHz SPI
    localparam CS_GAP_NS      = 400;     // idle between transfers

    // Shortened so the design leaves reset quickly in simulation. The real
    // default is 255 clocks (~5 us at 50 MHz).
    localparam POR_CYC        = 8;

    // 250 ms of blanking at 50 MHz, expressed against this bench's transfer
    // period: one transfer is ~2900 ns, so 1800 core clocks is ~12.4 transfers
    // -- the same ratio the shipping 50 Hz sample rate gives (250 ms / 20 ms).
    localparam REFRACTORY_CYC = 1800;

    // -------------------------------------------------------------------------
    // Synthetic PPG waveform (transaction space = one sample per transfer)
    // -------------------------------------------------------------------------
    localparam PULSE_SAMPLES   = 14;     // rise + systolic decay
    localparam CYCLE_SAMPLES   = 50;     // 0.6 s at the shipping 50 Hz rate
    localparam N_CYCLES        = 6;
    localparam BASE_LEVEL      = 8'd65;  // diastolic baseline, below threshold
    localparam PEAK_THRESHOLD  = 8'd120; // dyn_threshold strapped in the top

    // The first crest after reset only primes first_beat_seen, so one fewer
    // beat comes out than there are pulses.
    localparam EXPECTED_BEATS  = N_CYCLES - 1;

    // -------------------------------------------------------------------------
    // DUT
    // -------------------------------------------------------------------------
    reg  clk = 1'b0;
    reg  spi_sck  = 1'b0;
    reg  spi_ss_n = 1'b1;
    reg  spi_mosi = 1'b0;

    wire spi_miso;
    wire spi_miso_oe;
    wire clk_en;
    wire led_user;
    wire led_user_oe;

    forgefpga_ppg_top #(
        .REFRACTORY_CYC (REFRACTORY_CYC),
        .POR_CYC        (POR_CYC)
    ) dut (
        .clk         (clk),
        .clk_en      (clk_en),
        .spi_sck     (spi_sck),
        .spi_ss_n    (spi_ss_n),
        .spi_mosi    (spi_mosi),
        .spi_miso    (spi_miso),
        .spi_miso_oe (spi_miso_oe),
        .led_user    (led_user),
        .led_user_oe (led_user_oe)
    );

    always #(CLK_PERIOD_NS / 2) clk = ~clk;

    // -------------------------------------------------------------------------
    // Test bookkeeping
    // -------------------------------------------------------------------------
    integer tests_passed = 0;
    integer tests_failed = 0;

    // Note: the label is passed as a 512-bit vector, not 256. Icarus Verilog's
    // %0s drops the first byte when the vector is exactly as wide as the string
    // it holds, so a 32-character label in a 256-bit field prints truncated.
    task expect_eq;
        input [8*64-1:0] name;
        input integer    got;
        input integer    exp;
        begin
            if (got === exp) begin
                tests_passed = tests_passed + 1;
                $display("  PASS: %0s (got %0d, expected %0d)", name, got, exp);
            end else begin
                tests_failed = tests_failed + 1;
                $display("  FAIL: %0s (got %0d, expected %0d)", name, got, exp);
            end
        end
    endtask

    task expect_defined;
        input [8*64-1:0] name;
        input [7:0]      got;
        begin
            if (^got === 1'bx) begin
                tests_failed = tests_failed + 1;
                $display("  FAIL: %0s (reply is X - the design never left reset)", name);
            end else begin
                tests_passed = tests_passed + 1;
                $display("  PASS: %0s (reply defined: 0x%02X)", name, got);
            end
        end
    endtask

    // -------------------------------------------------------------------------
    // ESP32-S3 SPI master bus functional model: mode 0, MSB first.
    //   MISO is driven by the slave before each rising edge (loaded while CS is
    //   high, then shifted on every falling edge), so it is sampled here during
    //   the SCK high phase.
    // -------------------------------------------------------------------------
    task spi_transfer;
        input  [7:0] tx;
        output [7:0] rx;
        integer b;
        begin
            rx = 8'h00;
            @(negedge clk);
            spi_ss_n = 1'b0;
            #(SCK_HALF_NS);
            for (b = 7; b >= 0; b = b - 1) begin
                spi_mosi = tx[b];
                #(SCK_HALF_NS);
                spi_sck  = 1'b1;            // slave samples MOSI on this edge
                #(SCK_HALF_NS);
                rx       = {rx[6:0], spi_miso};
                spi_sck  = 1'b0;            // slave shifts out the next bit
                #(SCK_HALF_NS);
            end
            spi_ss_n = 1'b1;
            #(CS_GAP_NS);
        end
    endtask

    // -------------------------------------------------------------------------
    // Independent sliding-window model of the 8-tap average.
    // Computed from the raw sample stream, not from DUT state, so it catches a
    // wrong window depth, a stale eviction index, or truncation vs rounding.
    // -------------------------------------------------------------------------
    reg [7:0]  win [0:7];
    reg [11:0] win_sum;
    integer    wi;

    initial begin
        win_sum = 12'd0;
        for (wi = 0; wi < 8; wi = wi + 1) win[wi] = 8'd0;
    end

    task model_avg;
        input  [7:0] s;
        output [7:0] avg;
        begin
            win_sum = win_sum + s - win[7];
            for (wi = 7; wi > 0; wi = wi - 1) win[wi] = win[wi - 1];
            win[0] = s;
            avg = win_sum[10:3];
        end
    endtask

    // -------------------------------------------------------------------------
    // Stimulus
    // -------------------------------------------------------------------------
    reg [7:0] pulse_wave [0:PULSE_SAMPLES-1];
    reg [7:0] rx, model_out, sample;
    integer   k, cyc, s, idx;
    integer   nbeats;
    integer   beat_at [0:15];
    reg       prev_beat;
    reg [7:0] prev_expected;

    initial begin
        pulse_wave[0]  = 8'd140; pulse_wave[1]  = 8'd180;
        pulse_wave[2]  = 8'd215; pulse_wave[3]  = 8'd230;
        pulse_wave[4]  = 8'd225; pulse_wave[5]  = 8'd205;
        pulse_wave[6]  = 8'd175; pulse_wave[7]  = 8'd145;
        pulse_wave[8]  = 8'd120; pulse_wave[9]  = 8'd100;
        pulse_wave[10] = 8'd85;  pulse_wave[11] = 8'd75;
        pulse_wave[12] = 8'd70;  pulse_wave[13] = 8'd68;

        idx       = 0;
        nbeats    = 0;
        prev_beat = 1'b0;
    end

    initial begin
        $display("================================================================");
        $display("  tb_forgefpga_system -- ShrikeFi SPI link + PPG accelerator");
        $display("================================================================");

        spi_sck  = 1'b0;
        spi_ss_n = 1'b1;
        spi_mosi = 1'b0;

        // Let the internal power-on reset release before talking to the part.
        #1000;

        // =====================================================================
        // TEST 1 -- the design comes out of reset and the link is defined
        // =====================================================================
        $display("\n[TEST 1] Internal POR released; first reply is defined");
        spi_transfer(8'h00, rx);
        expect_defined("first SPI reply is a defined byte", rx);
        expect_eq("beat flag clear on an idle link", rx[7], 0);
        expect_eq("clk_en asserted (oscillator enable)", clk_en, 1);
        expect_eq("spi_miso_oe asserted (MISO pad driven)", spi_miso_oe, 1);

        // =====================================================================
        // TEST 2 -- filter converges to a constant input through the SPI path
        // =====================================================================
        $display("\n[TEST 2] 8-tap average converges to a constant (100)");
        sample = 8'd100;
        for (k = 0; k < 16; k = k + 1) begin
            spi_transfer(sample, rx);
            model_avg(sample, model_out);
        end
        // 16 transfers is twice the window depth, so the reply has settled.
        expect_eq("converged filtered sample (low 7 bits)", rx[6:0], 100);
        expect_eq("no false beat during a steady input", nbeats, 0);

        // =====================================================================
        // TEST 3 -- the returned average is bit-exact for a varying input
        // =====================================================================
        // One transfer of pipeline latency: the reply to transfer k+1 carries
        // the window that ends at sample k. Values stay below 128 so the 7-bit
        // reply field is lossless.
        $display("\n[TEST 3] Returned average is bit-exact against a sliding window");
        begin : test3
            integer mismatches;
            mismatches = 0;
            prev_expected = 8'd0;
            for (k = 0; k < 40; k = k + 1) begin
                sample = 8'd20 + ((k * 7) % 90);   // 20..109, varying, < 128
                spi_transfer(sample, rx);
                model_avg(sample, model_out);
                // rx still holds the reply for the PREVIOUS sample; compare it
                // with the model output for that same previous sample.
                if (k > 0) begin
                    if (rx[6:0] !== prev_expected[6:0]) begin
                        if (mismatches < 3) begin
                            $display("    mismatch at sample %0d: reply %0d, model %0d",
                                     k - 1, rx[6:0], prev_expected[6:0]);
                        end
                        mismatches = mismatches + 1;
                    end
                end
                prev_expected = model_out;
            end
            expect_eq("bit-exact filter replies over 39 comparisons", mismatches, 0);
        end

        // =====================================================================
        // TEST 4 -- one beat flag per pulse; the first crest only primes the IBI
        // =====================================================================
        $display("\n[TEST 4] Pulse train produces exactly one beat flag per crest");
        // Guard the test's own premise: the diastolic baseline must sit below
        // the threshold strapped in forgefpga_ppg_top, or the FSM would never
        // leave STATE_ARMED and a "0 beats" result would mean nothing.
        expect_eq("stimulus baseline is below the strapped threshold",
                  BASE_LEVEL < PEAK_THRESHOLD, 1);
        idx = 0;
        nbeats = 0;
        prev_beat = 1'b0;
        for (cyc = 0; cyc < N_CYCLES; cyc = cyc + 1) begin
            for (s = 0; s < CYCLE_SAMPLES; s = s + 1) begin
                if (s < PULSE_SAMPLES) sample = pulse_wave[s];
                else                   sample = BASE_LEVEL;

                spi_transfer(sample, rx);
                idx = idx + 1;

                if (rx[7] === 1'b1 && prev_beat === 1'b0 && nbeats < 16) begin
                    beat_at[nbeats] = idx;
                    nbeats = nbeats + 1;
                end
                if (rx[7] !== 1'bx) prev_beat = rx[7];
            end
        end
        $display("    %0d pulse(s) fed, %0d beat flag(s) returned", N_CYCLES, nbeats);
        expect_eq("beat flags for 6 pulses (first crest primes the IBI clock)",
                  nbeats, EXPECTED_BEATS);

        // =====================================================================
        // TEST 5 -- beat spacing matches the stimulus period
        // =====================================================================
        // This is the IBI the ESP32-S3 would compute: it timestamps beats itself
        // and never reads the RTL's ibi_cycles counter.
        $display("\n[TEST 5] Beat spacing equals the stimulus period (IBI accuracy)");
        begin : test5
            integer i, spacing, max_err;
            max_err = 0;
            for (i = 1; i < nbeats; i = i + 1) begin
                spacing = beat_at[i] - beat_at[i - 1];
                $display("    beat %0d -> %0d: %0d transfers", i, i + 1, spacing);
                if (spacing - CYCLE_SAMPLES > max_err) max_err = spacing - CYCLE_SAMPLES;
                if (CYCLE_SAMPLES - spacing > max_err) max_err = CYCLE_SAMPLES - spacing;
            end
            expect_eq("beat spacing matches the 50-transfer stimulus period",
                      max_err, 0);
        end

        // =====================================================================
        // Summary
        // =====================================================================
        $display("\n================================================================");
        $display("  RESULTS: %0d passed, %0d failed (out of %0d checks)",
                 tests_passed, tests_failed, tests_passed + tests_failed);
        if (tests_failed == 0) $display("  >>> ALL TESTS PASSED <<<");
        else                   $display("  >>> FAILURES PRESENT <<<");
        $display("================================================================");
        $finish;
    end

endmodule
