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
// required drop is proportional for strong signals (peak/8) and a fixed floor
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

    // Required drop from peak_val to confirm a crest.
    //   peak > 48 : peak/8 -- proportional, so a tall pulse is not confirmed by
    //               a ripple-sized dip on its own shoulder.
    //   peak <= 48: fixed floor of 6 -- a weak pulse (poor contact) has a small
    //               absolute swing, and a proportional-only rule would demand a
    //               drop smaller than the ADC noise floor or larger than the
    //               pulse itself.
    // Constants assume the 8-bit sample path used by both platforms.
    wire [DATA_WIDTH-1:0] crest_fall_min =
        (peak_val > 8'd48) ? (peak_val >> 3) : 8'd6;

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
        end else begin
            current_state <= next_state;

            // Interval counter with saturation clamp at 32'hFFFF_FFFF
            if (interval_cnt != 32'hFFFF_FFFF) begin
                interval_cnt <= interval_cnt + 32'd1;
            end

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
                        peak_val <= sample_in;
                    end
                end

                STATE_RISING: begin
                    beat_detected <= 1'b0;
                    if (sample_valid && (sample_in > peak_val)) begin
                        peak_val <= sample_in;
                    end
                end

                STATE_PEAK_FOUND: begin
                    if (first_beat_seen) begin
                        beat_detected  <= 1'b1;
                        ibi_cycles     <= interval_cnt;
                    end else begin
                        beat_detected   <= 1'b0;
                        first_beat_seen <= 1'b1;
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
                if (sample_valid && (sample_in >= dyn_threshold)) begin
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
