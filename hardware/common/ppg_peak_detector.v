`timescale 1ns / 1ps

module ppg_peak_detector #(
    parameter DATA_WIDTH      = 8,
    parameter REFRACTORY_CYC  = 12_500_000, // 250ms at 50MHz clock
    parameter DEFAULT_THRESH  = 8'd120
)(
    input  wire                   clk,
    input  wire                   rst_n,
    input  wire                   sample_valid,
    input  wire [DATA_WIDTH-1:0]  sample_in,
    input  wire [DATA_WIDTH-1:0]  dyn_threshold, // Dynamically programmable from AXI
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
    reg        first_beat_seen;  // Guard: IBI only valid from 2nd beat

    // Sequential state & timer management
    always @(posedge clk) begin
        if (!rst_n) begin
            current_state   <= STATE_ARMED;
            prev_sample     <= 0;
            refractory_cnt  <= 0;
            interval_cnt    <= 0;
            ibi_cycles      <= 0;
            beat_detected   <= 1'b0;
            first_beat_seen <= 1'b0;
        end else begin
            current_state <= next_state;

            // Timer with saturation clamp at 32'hFFFF_FFFF
            if (interval_cnt != 32'hFFFF_FFFF) begin
                interval_cnt <= interval_cnt + 1;
            end

            if (sample_valid) begin
                prev_sample <= sample_in;
            end

            case (current_state)
                STATE_ARMED: begin
                    beat_detected <= 1'b0;
                end

                STATE_RISING: begin
                    beat_detected <= 1'b0;
                end

                STATE_PEAK_FOUND: begin
                    if (first_beat_seen) begin
                        beat_detected  <= 1'b1;
                        ibi_cycles     <= interval_cnt;
                    end else begin
                        beat_detected   <= 1'b0;
                        first_beat_seen <= 1'b1;
                    end
                    interval_cnt   <= 0;
                    refractory_cnt <= REFRACTORY_CYC;
                end

                STATE_REFRACTORY: begin
                    beat_detected <= 1'b0;
                    if (refractory_cnt > 0) begin
                        refractory_cnt <= refractory_cnt - 1;
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
                // True peak crest detected when slope flips negative
                if (sample_valid && (sample_in < prev_sample)) begin
                    next_state = STATE_PEAK_FOUND;
                end
            end

            STATE_PEAK_FOUND: begin
                next_state = STATE_REFRACTORY;
            end

            STATE_REFRACTORY: begin
                if (refractory_cnt == 0) begin
                    next_state = STATE_ARMED;
                end
            end

            default: next_state = STATE_ARMED;
        endcase
    end

endmodule