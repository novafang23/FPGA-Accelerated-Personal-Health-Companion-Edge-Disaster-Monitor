`timescale 1ns / 1ps

// =============================================================================
// ppg_peak_detector.v — systolic crest detector + IBI timer
//
// Vendor-agnostic: no bus protocol, no vendor primitives. Instantiated by both
// the Zynq AXI accelerator (hardware/zynq/axi_ppg_accelerator.v) and the
// ShrikeFi ForgeFPGA top (hardware/shrikefi/forgefpga_ppg_top.v). This file is
// the single source for the detector on both platforms — do not fork it into a
// consuming top level, or the two copies will drift (they did once: the Zynq
// path kept an older crest rule for months without anyone noticing).
//
// Crest rule
// ----------
// A beat is committed when the sample falls far enough below the running peak
// (peak_val) to be a real systolic downstroke rather than filter ripple. The
// required drop is proportional for strong signals (peak/4) and a fixed floor
// for weak ones. It is deliberately NOT "two consecutive decreasing samples":
// that rule truncates the peak at the first pair of noisy decreasing samples on
// the rising edge. Hardware measurements on the ShrikeFi link drove both
// changes -- see reports/FPGA_PPG_WAVEFORM_ANALYSIS.md.
// =============================================================================

module ppg_peak_detector #(
    parameter DATA_WIDTH      = 8,
    parameter REFRACTORY_CYC  = 12_500_000, // 250 ms at 50 MHz clock
    parameter DEFAULT_THRESH  = 8'd120      // arm level if caller ties dyn_threshold
)(
    input  wire                   clk,
    input  wire                   rst_n,
    input  wire                   sample_valid,
    input  wire [DATA_WIDTH-1:0]  sample_in,
    input  wire [DATA_WIDTH-1:0]  dyn_threshold, // dynamically programmable (AXI reg / SPI constant)
    output reg                    beat_detected,
    output reg  [31:0]            ibi_cycles
);

    localparam STATE_ARMED      = 2'b00;
    localparam STATE_RISING     = 2'b01;
    localparam STATE_PEAK_FOUND = 2'b10;
    localparam STATE_REFRACTORY = 2'b11;

    reg [1:0]  current_state, next_state;
    reg [DATA_WIDTH-1:0] prev_sample;
    reg [DATA_WIDTH-1:0] peak_val;         // running maximum of the current rise
    reg [31:0] refractory_cnt;
    reg [31:0] interval_cnt;
    reg        first_beat_seen;            // guard: IBI is only valid from the 2nd beat

    /* ---- Crest timestamps -------------------------------------------------
     * IBI used to be interval_cnt latched at STATE_PEAK_FOUND, i.e. at the
     * moment the downstroke had fallen crest_fall_min below the running peak.
     * That is the CONFIRMATION, not the crest, and the delay between them
     * depends on how steep the downstroke is - which varies with heart rate and
     * pulse morphology. It partly cancels between two consecutive beats, but
     * only partly, so the old interval carried a rate-dependent bias on top of
     * the 10 ms sample quantisation.
     *
     * Now the crest itself is timestamped: peak_cycle records the cycle at
     * which peak_val last advanced, and the IBI is the difference between
     * successive crest timestamps. The confirmation latency still exists, but
     * it no longer enters the measurement.
     *
     * cycle_now is free-running (never reset) so the delta is well defined
     * across the interval_cnt reset that happens on every confirmation. */
    reg [31:0] cycle_now;
    reg [31:0] peak_cycle;                 // cycle of the most recent crest
    reg [31:0] prev_peak_cycle;            // cycle of the previous crest
    reg        peak_cycle_valid;           // a crest has been timestamped before
    reg        prev_peak_valid;            // prev_peak_cycle holds a real crest */

    /* Number of 20 ns cycles in one 100 Hz sample period, for the diagnostic
     * output below. 50 MHz / 100 Hz = 500_000. */
    localparam [31:0] CYCLES_PER_SAMPLE = 32'd500000;

    // Required drop from peak_val to confirm a crest.
    //   peak > 48 : peak/4 -- 25 %. The earlier peak/8 (12.5 %) rule still
    //               confirmed the first systolic maximum when the dicrotic
    //               notch dipped more than 12.5 % on the actual filtered
    //               signal. That produced short/long pairs (measured
    //               888/645 ms, 781/641, 666/797 against ~730 ms rhythm).
    //               A real systolic downstroke falls well past 25 % of the
    //               peak, so peak/4 rejects the notch without a time-based
    //               holdoff.
    //   peak <= 48: fixed floor of 6 -- a weak pulse (poor contact) has a small
    //               absolute swing, and a proportional-only rule would demand a
    //               drop smaller than the ADC noise floor or larger than the
    //               pulse itself.
    // Constants assume the 8-bit sample path used by both platforms.
    wire [DATA_WIDTH-1:0] crest_fall_min =
        (peak_val > 8'd48) ? (peak_val >> 2) : 8'd6;

    // Sequential state, timers and running-peak tracking
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current_state   <= STATE_ARMED;
            prev_sample     <= {DATA_WIDTH{1'b0}};
            peak_val        <= {DATA_WIDTH{1'b0}};
            refractory_cnt  <= 32'd0;
            interval_cnt    <= 32'd0;
            ibi_cycles      <= 32'd0;
            beat_detected   <= 1'b0;
            first_beat_seen <= 1'b0;
            cycle_now       <= 32'd0;
            peak_cycle      <= 32'd0;
            prev_peak_cycle <= 32'd0;
            peak_cycle_valid<= 1'b0;
            prev_peak_valid <= 1'b0;
        end else begin
            current_state <= next_state;

            // Interval counter with saturation clamp at 32'hFFFF_FFFF
            if (interval_cnt != 32'hFFFF_FFFF) begin
                interval_cnt <= interval_cnt + 32'd1;
            end

            // Free-running cycle counter: the timebase for crest stamps. It is
            // deliberately NOT reset, so successive crest timestamps remain
            // comparable even though interval_cnt restarts on every beat.
            cycle_now <= cycle_now + 32'd1;

            if (sample_valid) begin
                prev_sample <= sample_in;
            end

            case (current_state)
                STATE_ARMED: begin
                    beat_detected <= 1'b0;
                    // Re-seed the running peak on the way back down, so the next
                    // rise starts measuring from this sample rather than from the
                    // previous beat's crest.
                    if (sample_valid) begin
                        peak_val   <= sample_in;
                        // The re-seeded sample is the first candidate crest of the
                        // new rise, so stamp it. Without this the first crest after
                        // a re-arm would inherit the previous beat's timestamp.
                        peak_cycle <= cycle_now;
                    end
                end

                STATE_RISING: begin
                    beat_detected <= 1'b0;
                    if (sample_valid && (sample_in > peak_val)) begin
                        peak_val   <= sample_in;
                        // This is the crest as far as the signal has shown so far.
                        // Stamping here - rather than at the confirmation below -
                        // is what removes the downstroke-slope dependence from the
                        // interval.
                        peak_cycle <= cycle_now;
                    end
                end

                STATE_PEAK_FOUND: begin
                    /* First crest of the session: establish a reference and emit
                     * nothing, because an interval needs two crests. */
                    if (!peak_cycle_valid) begin
                        beat_detected    <= 1'b0;
                        first_beat_seen  <= 1'b1;
                        prev_peak_cycle  <= peak_cycle;
                        prev_peak_valid  <= 1'b1;
                        peak_cycle_valid <= 1'b1;
                    end else begin
                        beat_detected    <= 1'b1;
                        /* Crest-to-crest, both stamped at the sample where the
                         * running peak last advanced. */
                        ibi_cycles       <= peak_cycle - prev_peak_cycle;
                        prev_peak_cycle  <= peak_cycle;
                        prev_peak_valid  <= 1'b1;
                        peak_cycle_valid <= 1'b1;
                        first_beat_seen  <= 1'b1;
                    end
                    interval_cnt   <= 32'd0;
                    refractory_cnt <= REFRACTORY_CYC[31:0];
                end

                STATE_REFRACTORY: begin
                    beat_detected <= 1'b0;
                    if (refractory_cnt > 32'd0) begin
                        refractory_cnt <= refractory_cnt - 32'd1;
                    end
                end
            endcase
        end
    end

    // Combinational next-state transitions
    always @(*) begin
        next_state = current_state;
        case (current_state)
            STATE_ARMED: begin
                // Require both threshold crossing and positive slope (rising edge)
                // to prevent false re-triggering during wide pulse downslope decay.
                if (sample_valid && (sample_in >= dyn_threshold) && (sample_in > prev_sample)) begin
                    next_state = STATE_RISING;
                end
            end

            STATE_RISING: begin
                // Crest confirmed when the sample has dropped at least
                // crest_fall_min below the running peak.
                if (sample_valid && (peak_val > sample_in) &&
                    ((peak_val - sample_in) >= crest_fall_min)) begin
                    next_state = STATE_PEAK_FOUND;
                end
            end

            STATE_PEAK_FOUND: begin
                next_state = STATE_REFRACTORY;
            end

            STATE_REFRACTORY: begin
                // Re-arm on the refractory timer alone. Requiring the signal to
                // also fall back below dyn_threshold first would blind the
                // detector whenever the timer expires while a pulse is still
                // decaying above threshold -- and in a real recording the tail
                // frequently is still high at 250 ms, so that extra condition
                // dropped beats outright.
                if (refractory_cnt == 32'd0 && sample_valid) begin
                    next_state = STATE_ARMED;
                end
            end

            default: next_state = STATE_ARMED;
        endcase
    end

endmodule
