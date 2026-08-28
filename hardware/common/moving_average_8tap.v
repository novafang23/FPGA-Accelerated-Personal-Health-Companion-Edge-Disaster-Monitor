`timescale 1ns / 1ps

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
    reg [DATA_WIDTH+2:0] running_sum; // +3 bits prevents overflow for 8 samples
    integer i;

    always @(posedge clk) begin
        if (!rst_n) begin
            running_sum <= 0;
            data_out    <= 0;
            out_valid   <= 1'b0;
            for (i = 0; i < 8; i = i + 1) begin
                shift_reg[i] <= 0;
            end
        end else if (data_valid) begin
            // Shift pipeline
            shift_reg[0] <= data_in;
            for (i = 1; i < 8; i = i + 1) begin
                shift_reg[i] <= shift_reg[i-1];
            end

            // Update sum: (Old Sum + New Sample - Oldest Sample)
            running_sum <= running_sum + data_in - shift_reg[7];
            
            // Division by 8 via 3-bit right shift
            data_out    <= (running_sum + data_in - shift_reg[7]) >> 3;
            out_valid   <= 1'b1;
        end else begin
            out_valid   <= 1'b0;
        end
    end

endmodule