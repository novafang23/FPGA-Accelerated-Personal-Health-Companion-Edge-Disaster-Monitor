// =============================================================================
// File: forgefpga_ppg_top.v
// Module: forgefpga_ppg_top
// Project: SIH26181 Health Companion & Disaster Monitor
// Target: Renesas ForgeFPGA (SLG47910) / ShrikeFi Development Board
// Description:
//   Top-level FPGA hardware accelerator for ShrikeFi. Interfaces the ESP32-S3
//   microcontroller with the moving-average filters and systolic peak detector
//   over a 4-bit parallel nibble link.
// =============================================================================

`timescale 1ns / 1ps

(* top *)
module forgefpga_ppg_top #(
    parameter integer CLK_FREQ_HZ    = 50_000_000, // Core clock (50 MHz)
    parameter integer REFRACTORY_CYC = 12_500_000  // 250ms blanking window at 50MHz
)(
    // System Clock & Reset
    input  wire        clk,             // 50MHz System clock
    input  wire        rst_n,           // Active-low synchronous reset

    // 4-Bit Parallel Link Interface (from ESP32-S3)
    input  wire        link_strobe,     // Strobe clock pulse driven by ESP32
    input  wire        link_dir,        // Link direction: 0 = ESP32 Write, 1 = ESP32 Read
    input  wire [3:0]  link_din,        // 4-bit data input bus from ESP32
    output reg  [3:0]  link_dout,       // 4-bit data output bus to ESP32
    output reg         link_dout_oe,    // Output enable for bidirectional pin driver

    // Hardware Interrupt to ESP32-S3
    output reg         irq_beat         // Latched beat interrupt (cleared via CMD_CLEAR_IRQ)
);

    // =========================================================================
    // Protocol Command Definitions (CMD Nibble)
    // =========================================================================
    localparam [3:0] CMD_NOP          = 4'h0;
    localparam [3:0] CMD_WRITE_RED    = 4'h1; // Write Red PPG sample (2 nibbles)
    localparam [3:0] CMD_WRITE_IR     = 4'h2; // Write IR PPG sample (2 nibbles)
    localparam [3:0] CMD_WRITE_THRESH = 4'h3; // Write Systolic Threshold (2 nibbles)
    localparam [3:0] CMD_READ_RED     = 4'h4; // Read Filtered Red (2 nibbles)
    localparam [3:0] CMD_READ_IR      = 4'h5; // Read Filtered IR (2 nibbles)
    localparam [3:0] CMD_READ_IBI     = 4'h6; // Read 32-bit IBI cycles (8 nibbles)
    localparam [3:0] CMD_CLEAR_IRQ    = 4'h7; // Clear beat_flag & irq_beat (1 nibble)
    localparam [3:0] CMD_READ_STATUS  = 4'h8; // Read Status Byte (1 nibble)

    // =========================================================================
    // Link Transceiver FSM States
    // =========================================================================
    localparam [4:0] ST_IDLE          = 5'd0;
    
    // Write States
    localparam [4:0] ST_W_RED_H       = 5'd1;
    localparam [4:0] ST_W_RED_L       = 5'd2;
    localparam [4:0] ST_W_IR_H        = 5'd3;
    localparam [4:0] ST_W_IR_L        = 5'd4;
    localparam [4:0] ST_W_TH_H        = 5'd5;
    localparam [4:0] ST_W_TH_L        = 5'd6;
    
    // Read States
    localparam [4:0] ST_R_RED_H       = 5'd7;
    localparam [4:0] ST_R_RED_L       = 5'd8;
    localparam [4:0] ST_R_IR_H        = 5'd9;
    localparam [4:0] ST_R_IR_L        = 5'd10;
    localparam [4:0] ST_R_IBI_0       = 5'd11; // [31:28]
    localparam [4:0] ST_R_IBI_1       = 5'd12; // [27:24]
    localparam [4:0] ST_R_IBI_2       = 5'd13; // [23:20]
    localparam [4:0] ST_R_IBI_3       = 5'd14; // [19:16]
    localparam [4:0] ST_R_IBI_4       = 5'd15; // [15:12]
    localparam [4:0] ST_R_IBI_5       = 5'd16; // [11:8]
    localparam [4:0] ST_R_IBI_6       = 5'd17; // [7:4]
    localparam [4:0] ST_R_IBI_7       = 5'd18; // [3:0]
    localparam [4:0] ST_R_STATUS      = 5'd19;

    reg [4:0] state;

    // =========================================================================
    // Internal Registers & Signals
    // =========================================================================
    reg  [7:0]  reg_red_raw;
    reg  [7:0]  reg_ir_raw;
    reg  [7:0]  reg_threshold;
    reg  [31:0] reg_ibi_latched;
    reg         red_valid_pulse;
    reg         ir_valid_pulse;
    reg  [3:0]  nibble_temp;

    // Wires from submodules
    wire [7:0]  red_filtered;
    wire        red_filtered_valid;
    wire [7:0]  ir_filtered;
    wire        ir_filtered_valid;
    wire        peak_beat_detected;
    wire [31:0] peak_ibi_cycles;

    // Synchronizer & Edge Detector for link_strobe
    reg [2:0] strobe_sync;
    wire strobe_rise = (strobe_sync[1] && (strobe_sync[2] == 1'b0));

    always @(posedge clk) begin
        if (rst_n == 1'b0) begin
            strobe_sync <= 3'b000;
        end else begin
            strobe_sync <= {strobe_sync[1:0], link_strobe};
        end
    end

    // Synchronizer for link_dir (asynchronous control input from ESP32,
    // same clock-domain-crossing hazard as link_strobe above). Previously
    // sampled directly with no synchronizer, relying entirely on the
    // software-side delay margins between direction changes and strobes to
    // avoid metastability -- correct in practice given the ~1us settling
    // delay in shrikefi_link_driver.c, but not something the hardware itself
    // guaranteed. Two flip-flops is the standard minimum for a single-bit
    // level synchronizer (no edge detection needed here, unlike strobe).
    reg [1:0] dir_sync;
    wire link_dir_sync = dir_sync[1];

    always @(posedge clk) begin
        if (rst_n == 1'b0) begin
            dir_sync <= 2'b00;
        end else begin
            dir_sync <= {dir_sync[0], link_dir};
        end
    end

    // =========================================================================
    // Core DSP Submodules Instantiation
    // =========================================================================

    // Red Channel 8-Tap Moving Average Filter
    moving_average_8tap u_filter_red (
        .clk        (clk),
        .rst_n      (rst_n),
        .data_valid (red_valid_pulse),
        .data_in    (reg_red_raw),
        .data_out   (red_filtered),
        .out_valid  (red_filtered_valid)
    );

    // IR Channel 8-Tap Moving Average Filter
    moving_average_8tap u_filter_ir (
        .clk        (clk),
        .rst_n      (rst_n),
        .data_valid (ir_valid_pulse),
        .data_in    (reg_ir_raw),
        .data_out   (ir_filtered),
        .out_valid  (ir_filtered_valid)
    );

    // Systolic Peak Detector & IBI Hardware Counter
    ppg_peak_detector #(
        .REFRACTORY_CYC (REFRACTORY_CYC)
    ) u_peak_det (
        .clk           (clk),
        .rst_n         (rst_n),
        .sample_valid  (red_filtered_valid),
        .sample_in     (red_filtered),
        .dyn_threshold (reg_threshold),
        .beat_detected (peak_beat_detected),
        .ibi_cycles    (peak_ibi_cycles)
    );

    // =========================================================================
    // 4-Bit Parallel Link Protocol FSM & IRQ Latch
    // =========================================================================
    always @(posedge clk) begin
        if (rst_n == 1'b0) begin
            state           <= ST_IDLE;
            reg_red_raw     <= 8'd0;
            reg_ir_raw      <= 8'd0;
            reg_threshold   <= 8'd120; // Default threshold: 120
            reg_ibi_latched <= 32'd0;
            irq_beat        <= 1'b0;
            red_valid_pulse <= 1'b0;
            ir_valid_pulse  <= 1'b0;
            nibble_temp     <= 4'd0;
            link_dout       <= 4'd0;
            link_dout_oe    <= 1'b0;
        end else begin
            // Single-cycle pulse clearing
            red_valid_pulse <= 1'b0;
            ir_valid_pulse  <= 1'b0;

            // Output Enable control based on direction
            link_dout_oe    <= link_dir_sync;

            // Hardware Beat Latches
            if (peak_beat_detected) begin
                reg_ibi_latched <= peak_ibi_cycles;
                irq_beat        <= 1'b1;
            end

            if (strobe_rise) begin
                case (state)
                    // ---------------------------------------------------------
                    // IDLE State: Decode Command Nibble
                    // ---------------------------------------------------------
                    ST_IDLE: begin
                        if (link_dir_sync == 1'b0) begin
                            // Write Commands (from ESP32)
                            case (link_din)
                                CMD_WRITE_RED:    state <= ST_W_RED_H;
                                CMD_WRITE_IR:     state <= ST_W_IR_H;
                                CMD_WRITE_THRESH: state <= ST_W_TH_H;
                                CMD_READ_RED: begin
                                    link_dout <= red_filtered[7:4];
                                    state     <= ST_R_RED_L;
                                end
                                CMD_READ_IR: begin
                                    link_dout <= ir_filtered[7:4];
                                    state     <= ST_R_IR_L;
                                end
                                CMD_READ_IBI: begin
                                    link_dout <= reg_ibi_latched[31:28];
                                    state     <= ST_R_IBI_1;
                                end
                                CMD_READ_STATUS: begin
                                    link_dout <= {2'b00, red_filtered_valid, irq_beat};
                                    state     <= ST_IDLE;
                                end
                                CMD_CLEAR_IRQ: begin
                                    irq_beat  <= 1'b0;
                                    state     <= ST_IDLE;
                                end
                                default:          state <= ST_IDLE;
                            endcase
                        end
                    end

                    // ---------------------------------------------------------
                    // Write Sample Red (2 Nibbles)
                    // ---------------------------------------------------------
                    ST_W_RED_H: begin
                        nibble_temp <= link_din;
                        state       <= ST_W_RED_L;
                    end
                    ST_W_RED_L: begin
                        reg_red_raw     <= {nibble_temp, link_din};
                        red_valid_pulse <= 1'b1;
                        state           <= ST_IDLE;
                    end

                    // ---------------------------------------------------------
                    // Write Sample IR (2 Nibbles)
                    // ---------------------------------------------------------
                    ST_W_IR_H: begin
                        nibble_temp <= link_din;
                        state       <= ST_W_IR_L;
                    end
                    ST_W_IR_L: begin
                        reg_ir_raw      <= {nibble_temp, link_din};
                        ir_valid_pulse  <= 1'b1;
                        state           <= ST_IDLE;
                    end

                    // ---------------------------------------------------------
                    // Write Systolic Threshold (2 Nibbles)
                    // ---------------------------------------------------------
                    ST_W_TH_H: begin
                        nibble_temp <= link_din;
                        state       <= ST_W_TH_L;
                    end
                    ST_W_TH_L: begin
                        reg_threshold <= {nibble_temp, link_din};
                        state         <= ST_IDLE;
                    end

                    // ---------------------------------------------------------
                    // Read Filtered Red Output (2 Nibbles)
                    // ---------------------------------------------------------
                    ST_R_RED_L: begin
                        link_dout <= red_filtered[3:0];
                        state     <= ST_IDLE;
                    end

                    // ---------------------------------------------------------
                    // Read Filtered IR Output (2 Nibbles)
                    // ---------------------------------------------------------
                    ST_R_IR_L: begin
                        link_dout <= ir_filtered[3:0];
                        state     <= ST_IDLE;
                    end

                    // ---------------------------------------------------------
                    // Read 32-Bit IBI Cycles (8 Nibbles)
                    // ---------------------------------------------------------
                    ST_R_IBI_1: begin
                        link_dout <= reg_ibi_latched[27:24];
                        state     <= ST_R_IBI_2;
                    end
                    ST_R_IBI_2: begin
                        link_dout <= reg_ibi_latched[23:20];
                        state     <= ST_R_IBI_3;
                    end
                    ST_R_IBI_3: begin
                        link_dout <= reg_ibi_latched[19:16];
                        state     <= ST_R_IBI_4;
                    end
                    ST_R_IBI_4: begin
                        link_dout <= reg_ibi_latched[15:12];
                        state     <= ST_R_IBI_5;
                    end
                    ST_R_IBI_5: begin
                        link_dout <= reg_ibi_latched[11:8];
                        state     <= ST_R_IBI_6;
                    end
                    ST_R_IBI_6: begin
                        link_dout <= reg_ibi_latched[7:4];
                        state     <= ST_R_IBI_7;
                    end
                    ST_R_IBI_7: begin
                        link_dout <= reg_ibi_latched[3:0];
                        state     <= ST_IDLE;
                    end

                    default: state <= ST_IDLE;
                endcase
            end
        end
    end

endmodule
