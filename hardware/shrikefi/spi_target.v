`timescale 1ns / 1ps

// ---------------------------------------------------------------------------
// Submodule: Vicharak SPI Target (Slave) for Renesas ForgeFPGA
// ---------------------------------------------------------------------------
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
    reg [1:0] mosi_sync;
    reg [2:0] bit_cnt;
    reg [WIDTH-1:0] rx_shift;
    reg [WIDTH-1:0] tx_shift;

    wire sck_r = (sck_sync == 2'b01);
    wire sck_f = (sck_sync == 2'b10);
    wire ss_n  = ss_sync[1];

    always @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            sck_sync  <= 2'b00;
            ss_sync   <= 2'b11;
            mosi_sync <= 2'b00;
        end else begin
            sck_sync  <= {sck_sync[0], i_sck};
            ss_sync   <= {ss_sync[0], i_ss_n};
            mosi_sync <= {mosi_sync[0], i_mosi};
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
                        rx_shift <= {mosi_sync[1], rx_shift[WIDTH-1:1]};
                    else
                        rx_shift <= {rx_shift[WIDTH-2:0], mosi_sync[1]};

                    bit_cnt <= bit_cnt + 3'd1;

                    if (bit_cnt == BIT_MAX) begin
                        if (LSB)
                            o_rx_data <= {mosi_sync[1], rx_shift[WIDTH-1:1]};
                        else
                            o_rx_data <= {rx_shift[WIDTH-2:0], mosi_sync[1]};
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
