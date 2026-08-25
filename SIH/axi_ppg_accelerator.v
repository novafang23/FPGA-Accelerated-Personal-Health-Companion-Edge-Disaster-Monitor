`timescale 1ns / 1ps

// ============================================================================
// axi_ppg_accelerator.v — AXI4-Lite PPG Processing Accelerator
// SIH26181: AI-Powered Personal Health Companion (Qualcomm)
//
// Features:
//   - Dual-channel PPG filtering (Red 660nm + IR 940nm) via moving average
//   - Hardware peak detection with cycle-accurate IBI timing
//   - Dynamic threshold programming via software
//   - Decoupled AXI4-Lite write handshake (address/data independent phases)
//   - Beat interrupt output (irq_beat)
//
// Register Map (Word-aligned, 5-bit address):
//   0x00  REG_RED_RAW       [7:0]  R/W  Red raw sample (write triggers pipeline)
//   0x04  REG_RED_FILTERED  [7:0]  RO   Red filtered output
//   0x08  REG_IBI_CYCLES    [31:0] RO   Inter-Beat Interval (clock cycles)
//   0x0C  REG_STATUS_THRESH [0]    RO/W1C  beat_flag (Write-1-to-Clear)
//                           [15:8] R/W     Dynamic peak threshold
//   0x10  REG_IR_RAW        [7:0]  R/W  IR raw sample (write triggers pipeline)
//   0x14  REG_IR_FILTERED   [7:0]  RO   IR filtered output
// ============================================================================

module axi_ppg_accelerator #(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 5,   // Expanded from 4 to 5 for SpO2 regs
    parameter integer REFRACTORY_CYC     = 12_500_000 // 250ms refractory at 50MHz
)(
    input  wire                              s_axi_aclk,
    input  wire                              s_axi_aresetn,

    // Write Address Channel
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]     s_axi_awaddr,
    input  wire                              s_axi_awvalid,
    output reg                               s_axi_awready,

    // Write Data Channel
    input  wire [C_S_AXI_DATA_WIDTH-1:0]     s_axi_wdata,
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1:0] s_axi_wstrb,
    input  wire                              s_axi_wvalid,
    output reg                               s_axi_wready,

    // Write Response Channel
    output wire [1:0]                        s_axi_bresp,
    output reg                               s_axi_bvalid,
    input  wire                              s_axi_bready,

    // Read Address Channel
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]     s_axi_araddr,
    input  wire                              s_axi_arvalid,
    output reg                               s_axi_arready,

    // Read Data Channel
    output reg  [C_S_AXI_DATA_WIDTH-1:0]     s_axi_rdata,
    output wire [1:0]                        s_axi_rresp,
    output reg                               s_axi_rvalid,
    input  wire                              s_axi_rready,

    output wire                              irq_beat
);

    assign s_axi_bresp = 2'b00; // OKAY
    assign s_axi_rresp = 2'b00; // OKAY

    // ================================================================
    //  Decoupled Write Handshake State Flags
    // ================================================================
    // These flags allow the AXI address and data phases to complete
    // independently on separate clock cycles. The write executes only
    // when both phases have completed.
    reg aw_done;
    reg w_done;
    reg [C_S_AXI_ADDR_WIDTH-1:0] aw_addr_latched;
    reg [C_S_AXI_DATA_WIDTH-1:0] w_data_latched;

    // ================================================================
    //  Internal Registers
    // ================================================================
    reg [7:0]  reg_red_raw;        // Red channel raw sample
    reg        red_sample_valid;   // Pulse: new Red sample available
    reg [7:0]  reg_ir_raw;         // IR channel raw sample
    reg        ir_sample_valid;    // Pulse: new IR sample available
    reg [7:0]  reg_threshold;      // Dynamic peak detection threshold
    reg        beat_flag;          // Sticky beat-detected status flag

    // Read address latch
    reg [C_S_AXI_ADDR_WIDTH-1:0] axi_araddr_latched;

    // ================================================================
    //  Internal Interconnect Wires
    // ================================================================
    wire [7:0]  red_filtered;       // Red channel filtered output
    wire        red_filter_valid;
    wire [7:0]  ir_filtered;        // IR channel filtered output
    wire        ir_filter_valid;
    wire        hw_beat_pulse;      // Single-cycle beat detection pulse
    wire [31:0] hw_ibi_cycles;      // Inter-beat interval in clock cycles

    assign irq_beat = hw_beat_pulse;

    // ================================================================
    //  Sub-Module Instantiations
    // ================================================================

    // Red channel noise filter (8-tap moving average)
    moving_average_8tap #(.DATA_WIDTH(8)) u_filter_red (
        .clk        (s_axi_aclk),
        .rst_n      (s_axi_aresetn),
        .data_valid (red_sample_valid),
        .data_in    (reg_red_raw),
        .data_out   (red_filtered),
        .out_valid  (red_filter_valid)
    );

    // IR channel noise filter (8-tap moving average)
    moving_average_8tap #(.DATA_WIDTH(8)) u_filter_ir (
        .clk        (s_axi_aclk),
        .rst_n      (s_axi_aresetn),
        .data_valid (ir_sample_valid),
        .data_in    (reg_ir_raw),
        .data_out   (ir_filtered),
        .out_valid  (ir_filter_valid)
    );

    // Peak detector & IBI timer (operates on Red channel)
    ppg_peak_detector #(
        .DATA_WIDTH     (8),
        .REFRACTORY_CYC (REFRACTORY_CYC),
        .DEFAULT_THRESH (8'd120)
    ) u_peak_det (
        .clk           (s_axi_aclk),
        .rst_n         (s_axi_aresetn),
        .sample_valid  (red_filter_valid),
        .sample_in     (red_filtered),
        .dyn_threshold (reg_threshold),
        .beat_detected (hw_beat_pulse),
        .ibi_cycles    (hw_ibi_cycles)
    );

    // ================================================================
    //  Write Execute Flag
    // ================================================================
    wire write_execute = aw_done && w_done && ~s_axi_bvalid;

    // ================================================================
    //  AXI4-Lite Write Channel (Decoupled Address/Data Phases)
    // ================================================================
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            s_axi_awready   <= 1'b0;
            s_axi_wready    <= 1'b0;
            s_axi_bvalid    <= 1'b0;
            aw_done         <= 1'b0;
            w_done          <= 1'b0;
            aw_addr_latched <= 0;
            w_data_latched  <= 0;
            reg_red_raw        <= 8'd0;
            red_sample_valid   <= 1'b0;
            reg_ir_raw         <= 8'd0;
            ir_sample_valid    <= 1'b0;
            reg_threshold      <= 8'd120;
            beat_flag          <= 1'b0;
        end else begin
            // Default: deassert single-cycle handshake and data-valid pulses
            s_axi_awready  <= 1'b0;
            s_axi_wready   <= 1'b0;
            red_sample_valid <= 1'b0;
            ir_sample_valid  <= 1'b0;

            // Capture hardware beat pulse as sticky flag
            if (hw_beat_pulse) begin
                beat_flag <= 1'b1;
            end

            // ---- Address Phase (independent) ----
            if (~aw_done && s_axi_awvalid) begin
                s_axi_awready   <= 1'b1;
                aw_done         <= 1'b1;
                aw_addr_latched <= s_axi_awaddr;
            end

            // ---- Data Phase (independent) ----
            if (~w_done && s_axi_wvalid) begin
                s_axi_wready   <= 1'b1;
                w_done         <= 1'b1;
                w_data_latched <= s_axi_wdata;
            end

            // ---- Execute Write (when both phases done) ----
            if (write_execute) begin
                aw_done      <= 1'b0;
                w_done       <= 1'b0;
                s_axi_bvalid <= 1'b1;

                case (aw_addr_latched[4:2])
                    3'b000: begin  // 0x00 — Red raw sample
                        reg_red_raw      <= w_data_latched[7:0];
                        red_sample_valid <= 1'b1;
                    end
                    3'b011: begin  // 0x0C — Status / Threshold
                        reg_threshold <= w_data_latched[15:8];
                        if (w_data_latched[0]) beat_flag <= 1'b0;  // W1C
                    end
                    3'b100: begin  // 0x10 — IR raw sample
                        reg_ir_raw      <= w_data_latched[7:0];
                        ir_sample_valid <= 1'b1;
                    end
                    default: ; // Writes to RO registers ignored
                endcase
            end

            // ---- Write Response Handshake ----
            if (s_axi_bvalid && s_axi_bready) begin
                s_axi_bvalid <= 1'b0;
            end
        end
    end

    // ================================================================
    //  AXI4-Lite Read Channel
    // ================================================================
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            s_axi_arready      <= 1'b0;
            s_axi_rvalid       <= 1'b0;
            s_axi_rdata        <= 32'd0;
            axi_araddr_latched <= 0;
        end else begin
            // Address Handshake & Latch
            if (~s_axi_arready && s_axi_arvalid) begin
                s_axi_arready      <= 1'b1;
                axi_araddr_latched <= s_axi_araddr;
            end else begin
                s_axi_arready <= 1'b0;
            end

            // Data Return (one cycle after address accepted)
            if (s_axi_arready && ~s_axi_rvalid) begin
                s_axi_rvalid <= 1'b1;
                case (axi_araddr_latched[4:2])
                    3'b000: s_axi_rdata <= {24'd0, reg_red_raw};        // 0x00
                    3'b001: s_axi_rdata <= {24'd0, red_filtered};       // 0x04
                    3'b010: s_axi_rdata <= hw_ibi_cycles;               // 0x08
                    3'b011: s_axi_rdata <= {16'd0, reg_threshold,       // 0x0C
                                            7'd0, beat_flag};
                    3'b100: s_axi_rdata <= {24'd0, reg_ir_raw};         // 0x10
                    3'b101: s_axi_rdata <= {24'd0, ir_filtered};        // 0x14
                    default: s_axi_rdata <= 32'd0;
                endcase
            end else if (s_axi_rvalid && s_axi_rready) begin
                s_axi_rvalid <= 1'b0;
            end
        end
    end

endmodule