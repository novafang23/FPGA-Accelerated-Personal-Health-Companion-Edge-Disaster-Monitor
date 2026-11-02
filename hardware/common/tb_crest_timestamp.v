/*
 * tb_crest_timestamp.v - proves the IBI is taken at the crest, not at the
 * falling-edge confirmation.
 *
 * The defect being tested: IBI used to be interval_cnt latched when the
 * downstroke had fallen crest_fall_min below the running peak. That confirmation
 * arrives later than the crest itself, and how much later depends on how steep
 * the downstroke is. Two beats with identical crest spacing but different
 * downstroke slopes therefore measured different intervals.
 *
 * This bench drives two pairs of pulses. In each pair the two crests are the
 * SAME number of samples apart; only the fall rate after each crest differs.
 * With crest timestamping the reported IBI must match closely. With
 * confirmation timestamping it would differ by the latency delta.
 *
 * NOTE ON TIMING SCALE: SAMPLE_CYC is deliberately small in this bench. The
 * comparison under test is between two intervals measured in the same units, so
 * a 500_000-cycle (10 ms) sample period buys nothing but simulation time - it
 * would run for minutes and dump gigabytes. The logic is identical at
 * SAMPLE_CYC = 64, and the real period is exercised by tb_forgefpga_system.v.
 */
`timescale 1ns/1ps

module tb_crest_timestamp;

    reg clk = 0;
    reg rst_n = 0;
    reg sample_valid = 0;
    reg [7:0] sample_in = 0;
    reg [7:0] dyn_threshold = 8'd20;
    wire beat_detected;
    wire [31:0] ibi_cycles;

    always #10 clk = ~clk;          // 50 MHz

    /* REFRACTORY_CYC is overridden to 256 cycles.
     *
     * The real 250 ms refractory at 50 MHz is 12,500,000 cycles - far longer
     * than this bench needs to simulate, and longer than the entire test would
     * take at any useful sample period. The refractory is a time gate that
     * separates beats; shrinking it changes how FAST the test runs, not whether
     * the crest timestamp is taken correctly, which is the only thing under
     * test here. tb_forgefpga_system.v exercises the real value. */
    localparam [31:0] TEST_REFRACTORY_CYC = 32'd256;

    ppg_peak_detector #(
        .REFRACTORY_CYC(TEST_REFRACTORY_CYC)
    ) dut (
        .clk(clk), .rst_n(rst_n),
        .sample_valid(sample_valid), .sample_in(sample_in),
        .dyn_threshold(dyn_threshold),
        .beat_detected(beat_detected), .ibi_cycles(ibi_cycles)
    );

    localparam integer SAMPLE_CYC = 64;

    integer fails = 0;
    integer beats = 0;
    reg [31:0] ibi_at_beat [0:7];

    always @(posedge clk) begin
        if (beat_detected && beats < 8) begin
            ibi_at_beat[beats] = ibi_cycles;
            $display("    [beat %0d] peak_cycle=%0d prev=%0d delta=%0d ibi_cycles=%0d",
                     beats, dut.peak_cycle, dut.prev_peak_cycle,
                     dut.peak_cycle - dut.prev_peak_cycle, ibi_cycles);
            beats = beats + 1;
        end
    end

    task send_sample(input [7:0] v);
        begin
            @(negedge clk); sample_valid = 1; sample_in = v;
            @(negedge clk); sample_valid = 0;
            repeat (SAMPLE_CYC) @(posedge clk);
        end
    endtask

    /* Emit a pulse of EXACTLY `total_samples` samples.
     *
     * The pad-to-length matters. A steeper downstroke reaches the baseline in
     * fewer samples, so without padding the two pulse variants would occupy
     * different durations and their crests would not be equally spaced - the
     * test would then be measuring the pulse length, not the timestamp. Padding
     * at baseline after the fall fixes the crest-to-crest spacing at
     * total_samples regardless of fall_step, which is the only way the
     * comparison is meaningful.
     *
     * fall_step controls the downstroke steepness, hence the confirmation
     * latency, without changing the crest spacing. */
    task pulse(input [7:0] base, input [7:0] peak,
               input integer fall_step, input integer total_samples);
        integer i, n;
        begin
            n = 0;
            for (i = base; i <= peak; i = i + 20) begin
                send_sample(i[7:0]); n = n + 1;
            end
            for (i = peak - fall_step; i > base; i = i - fall_step) begin
                send_sample(i[7:0]); n = n + 1;
            end
            while (n < total_samples) begin
                send_sample(base); n = n + 1;
            end
        end
    endtask

    localparam integer PULSE_SAMPLES = 48;

    integer ibi_a = 0, ibi_b = 0;

    initial begin
        repeat (4) @(posedge clk);
        rst_n = 1;
        repeat (4) @(posedge clk);

        /* ---- Pair 1: gentle downstroke (fall 8 per sample) -------------- */
        beats = 0;
        pulse(8'd5, 8'd200, 8, PULSE_SAMPLES);
        pulse(8'd5, 8'd200, 8, PULSE_SAMPLES);
        if (beats < 1) begin
            $display("  FAIL: no beat produced in pair 1");
            fails = fails + 1;
        end else begin
            ibi_a = ibi_at_beat[0];
        end

        /* ---- Pair 2: steep downstroke (fall 40 per sample) --------------
         * Same crest-to-crest sample spacing, very different confirmation
         * latency.
         *
         * The DUT must be reset between pairs. `peak_cycle_valid` is a register
         * that persists across beats, so without a reset the FIRST crest of pair
         * 2 would emit a beat immediately (because a previous crest already
         * exists) and the measured interval would be the inter-pair gap rather
         * than a within-pair spacing. That is a property of the design, not a
         * defect: a long pause between pulses legitimately produces one long
         * interval, and the downstream IBI filter rejects it. */
        repeat (10) send_sample(8'd5);
        @(negedge clk); rst_n = 0;
        repeat (4) @(posedge clk);
        @(negedge clk); rst_n = 1;
        repeat (4) @(posedge clk);
        beats = 0;
        pulse(8'd5, 8'd200, 40, PULSE_SAMPLES);
        pulse(8'd5, 8'd200, 40, PULSE_SAMPLES);
        if (beats < 1) begin
            $display("  FAIL: no beat produced in pair 2");
            fails = fails + 1;
        end else begin
            ibi_b = ibi_at_beat[0];
        end

        $display("");
        $display("  IBI, gentle downstroke (fall 8/sample)  = %0d cycles", ibi_a);
        $display("  IBI, steep  downstroke (fall 40/sample) = %0d cycles", ibi_b);

        /* Both pulses use identical rise profiles, so their crests are the same
         * distance apart. Confirmation timestamping would separate these by the
         * latency delta; crest timestamping keeps them within a sample or two. */
        if ((ibi_a - ibi_b) <= 2 * SAMPLE_CYC && (ibi_b - ibi_a) <= 2 * SAMPLE_CYC) begin
            $display("  PASS: IBI is independent of downstroke steepness (within 2 samples)");
        end else begin
            $display("  FAIL: IBI differs by %0d cycles between equal-spacing pulses",
                     (ibi_a > ibi_b) ? (ibi_a - ibi_b) : (ibi_b - ibi_a));
            fails = fails + 1;
        end

        if (ibi_a > 0 && ibi_a < 32'd20000000) begin
            $display("  PASS: IBI is within a plausible range");
        end else begin
            $display("  FAIL: IBI %0d out of range", ibi_a);
            fails = fails + 1;
        end

        $display("");
        if (fails == 0)
            $display("  >>> ALL CREST-TIMESTAMP CHECKS PASSED <<<");
        else
            $display("  >>> %0d CHECK(S) FAILED <<<", fails);
        $finish;
    end

    initial begin
        #5000000;      // ~5 ms of simulated time is ample at SAMPLE_CYC=64
        $display("  FAIL: timeout");
        $finish;
    end

endmodule
