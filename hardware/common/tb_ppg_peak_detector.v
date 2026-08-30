`timescale 1ns / 1ps
//
// Unit-level testbench for ppg_peak_detector.v.
//
// This module previously had no dedicated unit test -- it was only
// exercised indirectly through the two system-level testbenches
// (tb_ppg_system.v, tb_forgefpga_system.v), which mostly checked "did an
// interrupt fire" rather than "was the reported IBI actually correct."
// That gap let two real bugs ship silently:
//
//   1. A single-sample dip during the rising edge of a pulse (noise,
//      filter quantization) could prematurely commit to STATE_PEAK_FOUND
//      before the true systolic peak was reached.
//   2. If REFRACTORY_CYC expired while the signal was still above
//      threshold (still decaying from the same pulse), STATE_ARMED would
//      immediately re-trigger STATE_RISING on the tail of that same
//      pulse, producing a second spurious beat_detected pulse and
//      silently overwriting ibi_cycles with a bogus short interval.
//
// TEST A directly reproduces bug #1 with a clean pulse containing one
// single-sample dip mid-rise. TEST B directly reproduces bug #2 by using
// a short REFRACTORY_CYC and a pulse whose tail decays slowly relative to
// it (the same conditions that caused tb_forgefpga_system.v's TEST 4 to
// silently read back ~134 cycles instead of ~3000 before this fix).
//
module tb_ppg_peak_detector;
    localparam CLK_PERIOD = 20; // 50 MHz

    reg clk = 0;
    reg rst_n;
    reg sample_valid;
    reg [7:0] sample_in;
    reg [7:0] dyn_threshold;
    wire beat_detected;
    wire [31:0] ibi_cycles;

    integer beat_count;
    integer tests_passed = 0;
    integer tests_failed = 0;

    always #(CLK_PERIOD/2) clk = ~clk;

    // Small REFRACTORY_CYC so TEST B's tail-decay scenario is reachable in
    // a reasonable simulation time, same spirit as the system testbenches'
    // reduced-refractory overrides.
    ppg_peak_detector #(
        .DATA_WIDTH(8),
        .REFRACTORY_CYC(50)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        .sample_valid(sample_valid),
        .sample_in(sample_in),
        .dyn_threshold(dyn_threshold),
        .beat_detected(beat_detected),
        .ibi_cycles(ibi_cycles)
    );

    always @(posedge beat_detected) begin
        beat_count = beat_count + 1;
        $display("    beat_detected pulse #%0d at t=%0t, ibi_cycles=%0d", beat_count, $time, ibi_cycles);
    end

    // Feed one sample per clock edge.
    task feed_sample(input [7:0] val);
        begin
            @(posedge clk);
            sample_in    <= val;
            sample_valid <= 1'b1;
            @(posedge clk);
            sample_valid <= 1'b0;
        end
    endtask

    task reset_dut;
        begin
            rst_n = 0;
            sample_valid = 0;
            sample_in = 0;
            dyn_threshold = 8'd120;
            beat_count = 0;
            repeat (4) @(posedge clk);
            rst_n = 1;
            repeat (2) @(posedge clk);
        end
    endtask

    integer i;
    initial begin
        $display("================================================================");
        $display("  ppg_peak_detector.v unit testbench");
        $display("================================================================");

        // -------------------------------------------------------------
        // TEST A: single-sample dip mid-rise must NOT be reported as the
        // peak. Ramp 50 -> 200 with one downward tick at sample 5 of 10,
        // then continue rising to the true peak at 220, then fall away.
        // Before the fix, the dip at sample 5 would have been reported
        // as the peak; after the fix, only the true 220 peak should be.
        // -------------------------------------------------------------
        $display("");
        $display("[TEST A] Single-sample dip on rising edge must not be treated as the peak");
        reset_dut();

        // First pulse just to clear first_beat_seen (IBI isn't checked
        // for it) and get into a clean baseline. Needs >=2 consecutive
        // decreasing samples at the end so it actually completes a peak
        // under the fixed detector, not just 1.
        for (i = 0; i < 8; i = i + 1) feed_sample(60 + i*15); // rises through threshold, up to ~165
        feed_sample(8'd100); // decrease #1
        feed_sample(8'd60);  // decrease #2 -- completes the (suppressed) first beat
        // Settle: keep feeding a below-threshold sample periodically while
        // refractory elapses, matching a continuously-sampling pipeline.
        for (i = 0; i < 10; i = i + 1) begin
            feed_sample(8'd50);
            repeat (4) @(posedge clk);
        end

        beat_count = 0;

        // Second pulse, with an intentional single-sample dip mid-rise.
        // The flat hold after the dip is deliberately long enough for
        // REFRACTORY_CYC (50) to fully elapse before the rise continues:
        // a buggy detector (single decrease = peak, no baseline-return
        // gate) will have already re-armed by then and will find a
        // SECOND "peak" on the real descent below, reporting 2 beats
        // total. The fixed detector should still be looking for its 2nd
        // consecutive decrease throughout, and report exactly 1.
        feed_sample(8'd130); // above threshold, rising
        feed_sample(8'd150);
        feed_sample(8'd170);
        feed_sample(8'd165); // <-- single-sample dip (noise), NOT the real peak
        for (i = 0; i < 15; i = i + 1) begin // hold near-flat long enough to clear refractory
            feed_sample(8'd165);
            repeat (4) @(posedge clk);
        end
        feed_sample(8'd190); // resumes rising past the dip
        feed_sample(8'd220); // TRUE peak
        feed_sample(8'd180); // now genuinely falling
        feed_sample(8'd130);
        feed_sample(8'd80);  // below threshold, pulse over
        for (i = 0; i < 10; i = i + 1) begin
            feed_sample(8'd50);
            repeat (4) @(posedge clk);
        end

        if (beat_count == 1) begin
            $display("  PASS: exactly one beat_detected pulse for this waveform (dip correctly ignored)");
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: expected exactly 1 beat_detected pulse, got %0d (the mid-rise dip was likely mistaken for the peak)", beat_count);
            tests_failed = tests_failed + 1;
        end

        // -------------------------------------------------------------
        // TEST B: refractory expiring while the signal is still above
        // threshold (slow-decaying tail) must NOT trigger a second,
        // spurious beat on the same pulse.
        // -------------------------------------------------------------
        $display("");
        $display("[TEST B] Refractory expiring mid-decay must not double-count the same pulse");
        reset_dut();
        beat_count = 0;

        // Warm-up pulse to clear first_beat_seen (not itself checked).
        feed_sample(8'd130);
        feed_sample(8'd170);
        feed_sample(8'd150); // decrease #1
        feed_sample(8'd80);  // decrease #2 -- completes the (suppressed) first beat
        for (i = 0; i < 10; i = i + 1) begin
            feed_sample(8'd50);
            repeat (4) @(posedge clk);
        end
        beat_count = 0;

        feed_sample(8'd130); // rising
        feed_sample(8'd160);
        feed_sample(8'd200); // TRUE peak
        // Slow decay: several samples still above threshold (120) while
        // REFRACTORY_CYC=50 elapses in the gaps between them.
        feed_sample(8'd190); repeat(20) @(posedge clk);
        feed_sample(8'd170); repeat(20) @(posedge clk);
        feed_sample(8'd150); repeat(20) @(posedge clk); // refractory (50) has now elapsed while still > threshold
        feed_sample(8'd130); repeat(20) @(posedge clk);
        feed_sample(8'd90);  // finally below threshold -- pulse genuinely over
        for (i = 0; i < 10; i = i + 1) begin
            feed_sample(8'd50);
            repeat (4) @(posedge clk);
        end

        if (beat_count == 1) begin
            $display("  PASS: exactly one beat_detected pulse for the whole decaying pulse (no spurious re-trigger)");
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: expected exactly 1 beat_detected pulse, got %0d (refractory likely re-armed mid-decay)", beat_count);
            tests_failed = tests_failed + 1;
        end

        $display("");
        $display("================================================================");
        if (tests_failed == 0) begin
            $display("  >>> ALL %0d TESTS PASSED <<<", tests_passed);
        end else begin
            $display("  >>> %0d TEST(S) FAILED (of %0d) <<<", tests_failed, tests_passed + tests_failed);
        end
        $display("================================================================");
        $finish;
    end
endmodule
