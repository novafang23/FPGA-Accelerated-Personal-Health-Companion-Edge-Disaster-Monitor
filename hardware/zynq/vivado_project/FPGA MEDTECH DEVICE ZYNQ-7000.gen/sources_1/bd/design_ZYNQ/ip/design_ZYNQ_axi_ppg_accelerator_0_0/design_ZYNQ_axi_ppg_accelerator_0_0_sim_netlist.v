// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2026.1 (win64) Build 6511674 Tue Jun 16 11:02:23 MDT 2026
// Date        : Fri Sep  4 13:54:12 2026
// Host        : ABHINAV running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode funcsim {c:/Users/abhin/OneDrive/Desktop/verilog/FPGA MEDTECH DEVICE
//               ZYNQ-7000/FPGA MEDTECH DEVICE
//               ZYNQ-7000.gen/sources_1/bd/design_ZYNQ/ip/design_ZYNQ_axi_ppg_accelerator_0_0/design_ZYNQ_axi_ppg_accelerator_0_0_sim_netlist.v}
// Design      : design_ZYNQ_axi_ppg_accelerator_0_0
// Purpose     : This verilog netlist is a functional simulation representation of the design and should not be modified
//               or synthesized. This netlist cannot be used for SDF annotated simulation.
// Device      : xc7z020clg400-1
// --------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

(* CHECK_LICENSE_TYPE = "design_ZYNQ_axi_ppg_accelerator_0_0,axi_ppg_accelerator,{}" *) (* DowngradeIPIdentifiedWarnings = "yes" *) (* IP_DEFINITION_SOURCE = "module_ref" *) 
(* X_CORE_INFO = "axi_ppg_accelerator,Vivado 2026.1" *) 
(* NotValidForBitStream *)
module design_ZYNQ_axi_ppg_accelerator_0_0
   (s_axi_aclk,
    s_axi_aresetn,
    s_axi_awaddr,
    s_axi_awvalid,
    s_axi_awready,
    s_axi_wdata,
    s_axi_wstrb,
    s_axi_wvalid,
    s_axi_wready,
    s_axi_bresp,
    s_axi_bvalid,
    s_axi_bready,
    s_axi_araddr,
    s_axi_arvalid,
    s_axi_arready,
    s_axi_rdata,
    s_axi_rresp,
    s_axi_rvalid,
    s_axi_rready,
    irq_beat);
  (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 s_axi_aclk CLK" *) (* X_INTERFACE_MODE = "slave" *) (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi_aclk, ASSOCIATED_BUSIF s_axi, ASSOCIATED_RESET s_axi_aresetn, FREQ_HZ 50000000, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_ZYNQ_processing_system7_0_0_FCLK_CLK0, INSERT_VIP 0" *) input s_axi_aclk;
  (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 s_axi_aresetn RST" *) (* X_INTERFACE_MODE = "slave" *) (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi_aresetn, POLARITY ACTIVE_LOW, INSERT_VIP 0" *) input s_axi_aresetn;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi AWADDR" *) (* X_INTERFACE_MODE = "slave" *) (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi, DATA_WIDTH 32, PROTOCOL AXI4LITE, FREQ_HZ 50000000, ID_WIDTH 0, ADDR_WIDTH 5, AWUSER_WIDTH 0, ARUSER_WIDTH 0, WUSER_WIDTH 0, RUSER_WIDTH 0, BUSER_WIDTH 0, READ_WRITE_MODE READ_WRITE, HAS_BURST 0, HAS_LOCK 0, HAS_PROT 0, HAS_CACHE 0, HAS_QOS 0, HAS_REGION 0, HAS_WSTRB 1, HAS_BRESP 1, HAS_RRESP 1, SUPPORTS_NARROW_BURST 0, NUM_READ_OUTSTANDING 1, NUM_WRITE_OUTSTANDING 1, MAX_BURST_LENGTH 1, PHASE 0.0, CLK_DOMAIN design_ZYNQ_processing_system7_0_0_FCLK_CLK0, NUM_READ_THREADS 1, NUM_WRITE_THREADS 1, RUSER_BITS_PER_BYTE 0, WUSER_BITS_PER_BYTE 0, INSERT_VIP 0" *) input [4:0]s_axi_awaddr;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi AWVALID" *) input s_axi_awvalid;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi AWREADY" *) output s_axi_awready;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi WDATA" *) input [31:0]s_axi_wdata;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi WSTRB" *) input [3:0]s_axi_wstrb;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi WVALID" *) input s_axi_wvalid;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi WREADY" *) output s_axi_wready;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi BRESP" *) output [1:0]s_axi_bresp;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi BVALID" *) output s_axi_bvalid;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi BREADY" *) input s_axi_bready;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi ARADDR" *) input [4:0]s_axi_araddr;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi ARVALID" *) input s_axi_arvalid;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi ARREADY" *) output s_axi_arready;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi RDATA" *) output [31:0]s_axi_rdata;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi RRESP" *) output [1:0]s_axi_rresp;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi RVALID" *) output s_axi_rvalid;
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 s_axi RREADY" *) input s_axi_rready;
  output irq_beat;

  wire \<const0> ;
  wire irq_beat;
  wire s_axi_aclk;
  wire [4:0]s_axi_araddr;
  wire s_axi_aresetn;
  wire s_axi_arready;
  wire s_axi_arvalid;
  wire [4:0]s_axi_awaddr;
  wire s_axi_awready;
  wire s_axi_awvalid;
  wire s_axi_bready;
  wire s_axi_bvalid;
  wire [31:0]s_axi_rdata;
  wire s_axi_rready;
  wire s_axi_rvalid;
  wire [31:0]s_axi_wdata;
  wire s_axi_wready;
  wire s_axi_wvalid;

  assign s_axi_bresp[1] = \<const0> ;
  assign s_axi_bresp[0] = \<const0> ;
  assign s_axi_rresp[1] = \<const0> ;
  assign s_axi_rresp[0] = \<const0> ;
  GND GND
       (.G(\<const0> ));
  design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator inst
       (.irq_beat(irq_beat),
        .s_axi_aclk(s_axi_aclk),
        .s_axi_araddr(s_axi_araddr[4:2]),
        .s_axi_aresetn(s_axi_aresetn),
        .s_axi_arready(s_axi_arready),
        .s_axi_arvalid(s_axi_arvalid),
        .s_axi_awaddr(s_axi_awaddr[4:2]),
        .s_axi_awready(s_axi_awready),
        .s_axi_awvalid(s_axi_awvalid),
        .s_axi_bready(s_axi_bready),
        .s_axi_bvalid_reg_0(s_axi_bvalid),
        .s_axi_rdata(s_axi_rdata),
        .s_axi_rready(s_axi_rready),
        .s_axi_rvalid(s_axi_rvalid),
        .s_axi_wdata(s_axi_wdata[15:0]),
        .s_axi_wready(s_axi_wready),
        .s_axi_wvalid(s_axi_wvalid));
endmodule

(* ORIG_REF_NAME = "axi_ppg_accelerator" *) 
module design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator
   (irq_beat,
    s_axi_awready,
    s_axi_wready,
    s_axi_arready,
    s_axi_rdata,
    s_axi_bvalid_reg_0,
    s_axi_rvalid,
    s_axi_aclk,
    s_axi_wdata,
    s_axi_awaddr,
    s_axi_awvalid,
    s_axi_araddr,
    s_axi_arvalid,
    s_axi_aresetn,
    s_axi_bready,
    s_axi_wvalid,
    s_axi_rready);
  output irq_beat;
  output s_axi_awready;
  output s_axi_wready;
  output s_axi_arready;
  output [31:0]s_axi_rdata;
  output s_axi_bvalid_reg_0;
  output s_axi_rvalid;
  input s_axi_aclk;
  input [15:0]s_axi_wdata;
  input [2:0]s_axi_awaddr;
  input s_axi_awvalid;
  input [2:0]s_axi_araddr;
  input s_axi_arvalid;
  input s_axi_aresetn;
  input s_axi_bready;
  input s_axi_wvalid;
  input s_axi_rready;

  wire \aw_addr_latched[2]_i_1_n_0 ;
  wire \aw_addr_latched[3]_i_1_n_0 ;
  wire \aw_addr_latched[4]_i_1_n_0 ;
  wire aw_done;
  wire aw_done_i_1_n_0;
  wire \axi_araddr_latched[2]_i_1_n_0 ;
  wire \axi_araddr_latched[3]_i_1_n_0 ;
  wire \axi_araddr_latched[4]_i_1_n_0 ;
  wire [15:0]data3;
  wire [7:0]data_out;
  wire [7:0]ibi_cycles;
  wire ir_sample_valid;
  wire ir_sample_valid_i_1_n_0;
  wire irq_beat;
  wire [2:0]p_1_in_0;
  wire red_filter_valid;
  wire red_sample_valid;
  wire red_sample_valid_i_1_n_0;
  wire [7:0]reg_ir_raw;
  wire [0:0]reg_ir_raw_3;
  wire [7:0]reg_red_raw;
  wire [0:0]reg_red_raw_2;
  wire [0:0]reg_threshold;
  wire s_axi_aclk;
  wire [2:0]s_axi_araddr;
  wire s_axi_aresetn;
  wire s_axi_arready;
  wire s_axi_arready0;
  wire s_axi_arvalid;
  wire [2:0]s_axi_awaddr;
  wire s_axi_awready;
  wire s_axi_awready0;
  wire s_axi_awvalid;
  wire s_axi_bready;
  wire s_axi_bvalid_i_1_n_0;
  wire s_axi_bvalid_reg_0;
  wire [31:0]s_axi_rdata;
  wire [31:0]s_axi_rdata_1;
  wire s_axi_rready;
  wire s_axi_rvalid;
  wire s_axi_rvalid01_out;
  wire s_axi_rvalid_i_1_n_0;
  wire [15:0]s_axi_wdata;
  wire s_axi_wready;
  wire s_axi_wready0;
  wire s_axi_wvalid;
  wire [2:0]sel0;
  wire u_filter_ir_n_0;
  wire u_filter_ir_n_1;
  wire u_filter_ir_n_2;
  wire u_filter_ir_n_3;
  wire u_filter_ir_n_4;
  wire u_filter_ir_n_5;
  wire u_filter_ir_n_6;
  wire u_filter_red_n_10;
  wire u_filter_red_n_11;
  wire u_filter_red_n_12;
  wire u_filter_red_n_13;
  wire u_filter_red_n_14;
  wire u_filter_red_n_15;
  wire u_filter_red_n_16;
  wire u_filter_red_n_17;
  wire u_filter_red_n_18;
  wire u_filter_red_n_19;
  wire u_filter_red_n_20;
  wire u_filter_red_n_21;
  wire u_filter_red_n_29;
  wire u_filter_red_n_30;
  wire u_filter_red_n_31;
  wire u_filter_red_n_32;
  wire u_filter_red_n_9;
  wire u_peak_det_n_1;
  wire u_peak_det_n_34;
  wire [0:0]w_data_latched;
  wire \w_data_latched_reg_n_0_[10] ;
  wire \w_data_latched_reg_n_0_[11] ;
  wire \w_data_latched_reg_n_0_[12] ;
  wire \w_data_latched_reg_n_0_[13] ;
  wire \w_data_latched_reg_n_0_[14] ;
  wire \w_data_latched_reg_n_0_[15] ;
  wire \w_data_latched_reg_n_0_[1] ;
  wire \w_data_latched_reg_n_0_[2] ;
  wire \w_data_latched_reg_n_0_[3] ;
  wire \w_data_latched_reg_n_0_[4] ;
  wire \w_data_latched_reg_n_0_[5] ;
  wire \w_data_latched_reg_n_0_[6] ;
  wire \w_data_latched_reg_n_0_[7] ;
  wire \w_data_latched_reg_n_0_[8] ;
  wire \w_data_latched_reg_n_0_[9] ;
  wire w_done_i_1_n_0;
  wire w_done_reg_n_0;
  wire write_execute;

  LUT4 #(
    .INIT(16'hFB08)) 
    \aw_addr_latched[2]_i_1 
       (.I0(s_axi_awaddr[0]),
        .I1(s_axi_awvalid),
        .I2(aw_done),
        .I3(p_1_in_0[0]),
        .O(\aw_addr_latched[2]_i_1_n_0 ));
  LUT4 #(
    .INIT(16'hFB08)) 
    \aw_addr_latched[3]_i_1 
       (.I0(s_axi_awaddr[1]),
        .I1(s_axi_awvalid),
        .I2(aw_done),
        .I3(p_1_in_0[1]),
        .O(\aw_addr_latched[3]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair51" *) 
  LUT4 #(
    .INIT(16'hFB08)) 
    \aw_addr_latched[4]_i_1 
       (.I0(s_axi_awaddr[2]),
        .I1(s_axi_awvalid),
        .I2(aw_done),
        .I3(p_1_in_0[2]),
        .O(\aw_addr_latched[4]_i_1_n_0 ));
  FDRE \aw_addr_latched_reg[2] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\aw_addr_latched[2]_i_1_n_0 ),
        .Q(p_1_in_0[0]),
        .R(u_peak_det_n_1));
  FDRE \aw_addr_latched_reg[3] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\aw_addr_latched[3]_i_1_n_0 ),
        .Q(p_1_in_0[1]),
        .R(u_peak_det_n_1));
  FDRE \aw_addr_latched_reg[4] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\aw_addr_latched[4]_i_1_n_0 ),
        .Q(p_1_in_0[2]),
        .R(u_peak_det_n_1));
  LUT5 #(
    .INIT(32'hC808C8C8)) 
    aw_done_i_1
       (.I0(s_axi_awvalid),
        .I1(s_axi_aresetn),
        .I2(aw_done),
        .I3(s_axi_bvalid_reg_0),
        .I4(w_done_reg_n_0),
        .O(aw_done_i_1_n_0));
  FDRE aw_done_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(aw_done_i_1_n_0),
        .Q(aw_done),
        .R(1'b0));
  LUT4 #(
    .INIT(16'hFB08)) 
    \axi_araddr_latched[2]_i_1 
       (.I0(s_axi_araddr[0]),
        .I1(s_axi_arvalid),
        .I2(s_axi_arready),
        .I3(sel0[0]),
        .O(\axi_araddr_latched[2]_i_1_n_0 ));
  LUT4 #(
    .INIT(16'hFB08)) 
    \axi_araddr_latched[3]_i_1 
       (.I0(s_axi_araddr[1]),
        .I1(s_axi_arvalid),
        .I2(s_axi_arready),
        .I3(sel0[1]),
        .O(\axi_araddr_latched[3]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair52" *) 
  LUT4 #(
    .INIT(16'hFB08)) 
    \axi_araddr_latched[4]_i_1 
       (.I0(s_axi_araddr[2]),
        .I1(s_axi_arvalid),
        .I2(s_axi_arready),
        .I3(sel0[2]),
        .O(\axi_araddr_latched[4]_i_1_n_0 ));
  FDRE \axi_araddr_latched_reg[2] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\axi_araddr_latched[2]_i_1_n_0 ),
        .Q(sel0[0]),
        .R(u_peak_det_n_1));
  FDRE \axi_araddr_latched_reg[3] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\axi_araddr_latched[3]_i_1_n_0 ),
        .Q(sel0[1]),
        .R(u_peak_det_n_1));
  FDRE \axi_araddr_latched_reg[4] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\axi_araddr_latched[4]_i_1_n_0 ),
        .Q(sel0[2]),
        .R(u_peak_det_n_1));
  FDRE beat_flag_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(u_peak_det_n_34),
        .Q(data3[0]),
        .R(u_peak_det_n_1));
  (* SOFT_HLUTNM = "soft_lutpair50" *) 
  LUT5 #(
    .INIT(32'h10000000)) 
    ir_sample_valid_i_1
       (.I0(p_1_in_0[1]),
        .I1(p_1_in_0[0]),
        .I2(p_1_in_0[2]),
        .I3(write_execute),
        .I4(s_axi_aresetn),
        .O(ir_sample_valid_i_1_n_0));
  FDRE ir_sample_valid_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(ir_sample_valid_i_1_n_0),
        .Q(ir_sample_valid),
        .R(1'b0));
  (* SOFT_HLUTNM = "soft_lutpair50" *) 
  LUT5 #(
    .INIT(32'h01000000)) 
    red_sample_valid_i_1
       (.I0(p_1_in_0[2]),
        .I1(p_1_in_0[0]),
        .I2(p_1_in_0[1]),
        .I3(write_execute),
        .I4(s_axi_aresetn),
        .O(red_sample_valid_i_1_n_0));
  (* SOFT_HLUTNM = "soft_lutpair49" *) 
  LUT3 #(
    .INIT(8'h20)) 
    red_sample_valid_i_2
       (.I0(aw_done),
        .I1(s_axi_bvalid_reg_0),
        .I2(w_done_reg_n_0),
        .O(write_execute));
  FDRE red_sample_valid_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(red_sample_valid_i_1_n_0),
        .Q(red_sample_valid),
        .R(1'b0));
  LUT6 #(
    .INIT(64'h0000002000000000)) 
    \reg_ir_raw[7]_i_1 
       (.I0(w_done_reg_n_0),
        .I1(s_axi_bvalid_reg_0),
        .I2(aw_done),
        .I3(p_1_in_0[1]),
        .I4(p_1_in_0[0]),
        .I5(p_1_in_0[2]),
        .O(reg_ir_raw_3));
  FDRE \reg_ir_raw_reg[0] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(w_data_latched),
        .Q(reg_ir_raw[0]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[1] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[1] ),
        .Q(reg_ir_raw[1]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[2] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[2] ),
        .Q(reg_ir_raw[2]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[3] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[3] ),
        .Q(reg_ir_raw[3]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[4] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[4] ),
        .Q(reg_ir_raw[4]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[5] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[5] ),
        .Q(reg_ir_raw[5]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[6] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[6] ),
        .Q(reg_ir_raw[6]),
        .R(u_peak_det_n_1));
  FDRE \reg_ir_raw_reg[7] 
       (.C(s_axi_aclk),
        .CE(reg_ir_raw_3),
        .D(\w_data_latched_reg_n_0_[7] ),
        .Q(reg_ir_raw[7]),
        .R(u_peak_det_n_1));
  LUT6 #(
    .INIT(64'h0000000000000020)) 
    \reg_red_raw[7]_i_1 
       (.I0(w_done_reg_n_0),
        .I1(s_axi_bvalid_reg_0),
        .I2(aw_done),
        .I3(p_1_in_0[2]),
        .I4(p_1_in_0[0]),
        .I5(p_1_in_0[1]),
        .O(reg_red_raw_2));
  FDRE \reg_red_raw_reg[0] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(w_data_latched),
        .Q(reg_red_raw[0]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[1] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[1] ),
        .Q(reg_red_raw[1]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[2] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[2] ),
        .Q(reg_red_raw[2]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[3] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[3] ),
        .Q(reg_red_raw[3]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[4] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[4] ),
        .Q(reg_red_raw[4]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[5] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[5] ),
        .Q(reg_red_raw[5]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[6] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[6] ),
        .Q(reg_red_raw[6]),
        .R(u_peak_det_n_1));
  FDRE \reg_red_raw_reg[7] 
       (.C(s_axi_aclk),
        .CE(reg_red_raw_2),
        .D(\w_data_latched_reg_n_0_[7] ),
        .Q(reg_red_raw[7]),
        .R(u_peak_det_n_1));
  LUT6 #(
    .INIT(64'h0000000020000000)) 
    \reg_threshold[7]_i_1 
       (.I0(w_done_reg_n_0),
        .I1(s_axi_bvalid_reg_0),
        .I2(aw_done),
        .I3(p_1_in_0[1]),
        .I4(p_1_in_0[0]),
        .I5(p_1_in_0[2]),
        .O(reg_threshold));
  FDRE \reg_threshold_reg[0] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[8] ),
        .Q(data3[8]),
        .R(u_peak_det_n_1));
  FDRE \reg_threshold_reg[1] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[9] ),
        .Q(data3[9]),
        .R(u_peak_det_n_1));
  FDRE \reg_threshold_reg[2] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[10] ),
        .Q(data3[10]),
        .R(u_peak_det_n_1));
  FDSE \reg_threshold_reg[3] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[11] ),
        .Q(data3[11]),
        .S(u_peak_det_n_1));
  FDSE \reg_threshold_reg[4] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[12] ),
        .Q(data3[12]),
        .S(u_peak_det_n_1));
  FDSE \reg_threshold_reg[5] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[13] ),
        .Q(data3[13]),
        .S(u_peak_det_n_1));
  FDSE \reg_threshold_reg[6] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[14] ),
        .Q(data3[14]),
        .S(u_peak_det_n_1));
  FDRE \reg_threshold_reg[7] 
       (.C(s_axi_aclk),
        .CE(reg_threshold),
        .D(\w_data_latched_reg_n_0_[15] ),
        .Q(data3[15]),
        .R(u_peak_det_n_1));
  (* SOFT_HLUTNM = "soft_lutpair52" *) 
  LUT2 #(
    .INIT(4'h2)) 
    s_axi_arready_i_1
       (.I0(s_axi_arvalid),
        .I1(s_axi_arready),
        .O(s_axi_arready0));
  FDRE s_axi_arready_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(s_axi_arready0),
        .Q(s_axi_arready),
        .R(u_peak_det_n_1));
  (* SOFT_HLUTNM = "soft_lutpair51" *) 
  LUT2 #(
    .INIT(4'h2)) 
    s_axi_awready_i_2
       (.I0(s_axi_awvalid),
        .I1(aw_done),
        .O(s_axi_awready0));
  FDRE s_axi_awready_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(s_axi_awready0),
        .Q(s_axi_awready),
        .R(u_peak_det_n_1));
  (* SOFT_HLUTNM = "soft_lutpair49" *) 
  LUT5 #(
    .INIT(32'h0080F080)) 
    s_axi_bvalid_i_1
       (.I0(aw_done),
        .I1(w_done_reg_n_0),
        .I2(s_axi_aresetn),
        .I3(s_axi_bvalid_reg_0),
        .I4(s_axi_bready),
        .O(s_axi_bvalid_i_1_n_0));
  FDRE s_axi_bvalid_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(s_axi_bvalid_i_1_n_0),
        .Q(s_axi_bvalid_reg_0),
        .R(1'b0));
  LUT2 #(
    .INIT(4'h2)) 
    \s_axi_rdata[31]_i_1 
       (.I0(s_axi_arready),
        .I1(s_axi_rvalid),
        .O(s_axi_rvalid01_out));
  FDRE \s_axi_rdata_reg[0] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[0]),
        .Q(s_axi_rdata[0]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[10] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[10]),
        .Q(s_axi_rdata[10]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[11] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[11]),
        .Q(s_axi_rdata[11]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[12] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[12]),
        .Q(s_axi_rdata[12]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[13] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[13]),
        .Q(s_axi_rdata[13]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[14] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[14]),
        .Q(s_axi_rdata[14]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[15] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[15]),
        .Q(s_axi_rdata[15]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[16] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[16]),
        .Q(s_axi_rdata[16]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[17] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[17]),
        .Q(s_axi_rdata[17]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[18] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[18]),
        .Q(s_axi_rdata[18]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[19] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[19]),
        .Q(s_axi_rdata[19]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[1] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[1]),
        .Q(s_axi_rdata[1]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[20] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[20]),
        .Q(s_axi_rdata[20]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[21] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[21]),
        .Q(s_axi_rdata[21]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[22] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[22]),
        .Q(s_axi_rdata[22]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[23] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[23]),
        .Q(s_axi_rdata[23]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[24] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[24]),
        .Q(s_axi_rdata[24]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[25] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[25]),
        .Q(s_axi_rdata[25]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[26] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[26]),
        .Q(s_axi_rdata[26]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[27] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[27]),
        .Q(s_axi_rdata[27]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[28] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[28]),
        .Q(s_axi_rdata[28]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[29] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[29]),
        .Q(s_axi_rdata[29]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[2] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[2]),
        .Q(s_axi_rdata[2]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[30] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[30]),
        .Q(s_axi_rdata[30]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[31] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[31]),
        .Q(s_axi_rdata[31]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[3] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[3]),
        .Q(s_axi_rdata[3]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[4] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[4]),
        .Q(s_axi_rdata[4]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[5] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[5]),
        .Q(s_axi_rdata[5]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[6] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[6]),
        .Q(s_axi_rdata[6]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[7] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[7]),
        .Q(s_axi_rdata[7]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[8] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[8]),
        .Q(s_axi_rdata[8]),
        .R(u_peak_det_n_1));
  FDRE \s_axi_rdata_reg[9] 
       (.C(s_axi_aclk),
        .CE(s_axi_rvalid01_out),
        .D(s_axi_rdata_1[9]),
        .Q(s_axi_rdata[9]),
        .R(u_peak_det_n_1));
  LUT3 #(
    .INIT(8'h3A)) 
    s_axi_rvalid_i_1
       (.I0(s_axi_arready),
        .I1(s_axi_rready),
        .I2(s_axi_rvalid),
        .O(s_axi_rvalid_i_1_n_0));
  FDRE s_axi_rvalid_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(s_axi_rvalid_i_1_n_0),
        .Q(s_axi_rvalid),
        .R(u_peak_det_n_1));
  LUT2 #(
    .INIT(4'h2)) 
    s_axi_wready_i_1
       (.I0(s_axi_wvalid),
        .I1(w_done_reg_n_0),
        .O(s_axi_wready0));
  FDRE s_axi_wready_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(s_axi_wready0),
        .Q(s_axi_wready),
        .R(u_peak_det_n_1));
  design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap u_filter_ir
       (.D(s_axi_rdata_1[0]),
        .Q(reg_ir_raw),
        .\data_out_reg[1]_0 (u_filter_ir_n_1),
        .\data_out_reg[2]_0 (u_filter_ir_n_2),
        .\data_out_reg[3]_0 (u_filter_ir_n_3),
        .\data_out_reg[4]_0 (u_filter_ir_n_4),
        .\data_out_reg[5]_0 (u_filter_ir_n_5),
        .\data_out_reg[6]_0 (u_filter_ir_n_6),
        .\data_out_reg[7]_0 (u_filter_ir_n_0),
        .ir_sample_valid(ir_sample_valid),
        .\running_sum_reg[0]_0 (u_peak_det_n_1),
        .s_axi_aclk(s_axi_aclk),
        .\s_axi_rdata_reg[0] (u_filter_red_n_21),
        .\s_axi_rdata_reg[0]_0 (ibi_cycles[0]),
        .sel0(sel0));
  design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0 u_filter_red
       (.D(s_axi_rdata_1[7:1]),
        .DI({u_filter_red_n_9,u_filter_red_n_10,u_filter_red_n_11,u_filter_red_n_12}),
        .Q(reg_red_raw),
        .S({u_filter_red_n_17,u_filter_red_n_18,u_filter_red_n_19,u_filter_red_n_20}),
        .data3({data3[15:8],data3[0]}),
        .data_out(data_out),
        .\data_out_reg[0]_0 (u_peak_det_n_1),
        .\data_out_reg[6]_0 ({u_filter_red_n_13,u_filter_red_n_14,u_filter_red_n_15,u_filter_red_n_16}),
        .\data_out_reg[6]_1 ({u_filter_red_n_29,u_filter_red_n_30,u_filter_red_n_31,u_filter_red_n_32}),
        .red_filter_valid(red_filter_valid),
        .red_sample_valid(red_sample_valid),
        .\reg_red_raw_reg[0] (u_filter_red_n_21),
        .s_axi_aclk(s_axi_aclk),
        .\s_axi_rdata_reg[1] (u_filter_ir_n_1),
        .\s_axi_rdata_reg[2] (u_filter_ir_n_2),
        .\s_axi_rdata_reg[3] (u_filter_ir_n_3),
        .\s_axi_rdata_reg[4] (u_filter_ir_n_4),
        .\s_axi_rdata_reg[5] (u_filter_ir_n_5),
        .\s_axi_rdata_reg[6] (u_filter_ir_n_6),
        .\s_axi_rdata_reg[7] (ibi_cycles[7:1]),
        .\s_axi_rdata_reg[7]_0 (u_filter_ir_n_0),
        .\s_axi_rdata_reg[7]_1 (reg_ir_raw[7:1]),
        .sel0(sel0));
  design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector u_peak_det
       (.D(data_out),
        .DI({u_filter_red_n_9,u_filter_red_n_10,u_filter_red_n_11,u_filter_red_n_12}),
        .\FSM_sequential_current_state[1]_i_2_0 ({u_filter_red_n_13,u_filter_red_n_14,u_filter_red_n_15,u_filter_red_n_16}),
        .\FSM_sequential_current_state[1]_i_2_1 ({u_filter_red_n_29,u_filter_red_n_30,u_filter_red_n_31,u_filter_red_n_32}),
        .Q(ibi_cycles),
        .S({u_filter_red_n_17,u_filter_red_n_18,u_filter_red_n_19,u_filter_red_n_20}),
        .SR(u_peak_det_n_1),
        .\axi_araddr_latched_reg[2] (s_axi_rdata_1[31:8]),
        .beat_detected_reg_0(irq_beat),
        .beat_detected_reg_1(u_peak_det_n_34),
        .beat_flag_reg(w_data_latched),
        .data3({data3[15:8],data3[0]}),
        .p_1_in_0(p_1_in_0),
        .red_filter_valid(red_filter_valid),
        .s_axi_aclk(s_axi_aclk),
        .s_axi_aresetn(s_axi_aresetn),
        .sel0(sel0),
        .write_execute(write_execute));
  FDRE \w_data_latched_reg[0] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[0]),
        .Q(w_data_latched),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[10] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[10]),
        .Q(\w_data_latched_reg_n_0_[10] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[11] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[11]),
        .Q(\w_data_latched_reg_n_0_[11] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[12] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[12]),
        .Q(\w_data_latched_reg_n_0_[12] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[13] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[13]),
        .Q(\w_data_latched_reg_n_0_[13] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[14] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[14]),
        .Q(\w_data_latched_reg_n_0_[14] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[15] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[15]),
        .Q(\w_data_latched_reg_n_0_[15] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[1] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[1]),
        .Q(\w_data_latched_reg_n_0_[1] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[2] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[2]),
        .Q(\w_data_latched_reg_n_0_[2] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[3] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[3]),
        .Q(\w_data_latched_reg_n_0_[3] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[4] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[4]),
        .Q(\w_data_latched_reg_n_0_[4] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[5] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[5]),
        .Q(\w_data_latched_reg_n_0_[5] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[6] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[6]),
        .Q(\w_data_latched_reg_n_0_[6] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[7] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[7]),
        .Q(\w_data_latched_reg_n_0_[7] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[8] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[8]),
        .Q(\w_data_latched_reg_n_0_[8] ),
        .R(u_peak_det_n_1));
  FDRE \w_data_latched_reg[9] 
       (.C(s_axi_aclk),
        .CE(s_axi_wready0),
        .D(s_axi_wdata[9]),
        .Q(\w_data_latched_reg_n_0_[9] ),
        .R(u_peak_det_n_1));
  LUT5 #(
    .INIT(32'hCC0C8888)) 
    w_done_i_1
       (.I0(s_axi_wvalid),
        .I1(s_axi_aresetn),
        .I2(aw_done),
        .I3(s_axi_bvalid_reg_0),
        .I4(w_done_reg_n_0),
        .O(w_done_i_1_n_0));
  FDRE w_done_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(w_done_i_1_n_0),
        .Q(w_done_reg_n_0),
        .R(1'b0));
endmodule

(* ORIG_REF_NAME = "moving_average_8tap" *) 
module design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap
   (\data_out_reg[7]_0 ,
    \data_out_reg[1]_0 ,
    \data_out_reg[2]_0 ,
    \data_out_reg[3]_0 ,
    \data_out_reg[4]_0 ,
    \data_out_reg[5]_0 ,
    \data_out_reg[6]_0 ,
    D,
    \running_sum_reg[0]_0 ,
    ir_sample_valid,
    s_axi_aclk,
    Q,
    \s_axi_rdata_reg[0] ,
    \s_axi_rdata_reg[0]_0 ,
    sel0);
  output \data_out_reg[7]_0 ;
  output \data_out_reg[1]_0 ;
  output \data_out_reg[2]_0 ;
  output \data_out_reg[3]_0 ;
  output \data_out_reg[4]_0 ;
  output \data_out_reg[5]_0 ;
  output \data_out_reg[6]_0 ;
  output [0:0]D;
  input \running_sum_reg[0]_0 ;
  input ir_sample_valid;
  input s_axi_aclk;
  input [7:0]Q;
  input \s_axi_rdata_reg[0] ;
  input [0:0]\s_axi_rdata_reg[0]_0 ;
  input [2:0]sel0;

  wire [0:0]D;
  wire [7:0]Q;
  wire \data_out_reg[1]_0 ;
  wire \data_out_reg[2]_0 ;
  wire \data_out_reg[3]_0 ;
  wire \data_out_reg[4]_0 ;
  wire \data_out_reg[5]_0 ;
  wire \data_out_reg[6]_0 ;
  wire \data_out_reg[7]_0 ;
  wire \data_out_reg_n_0_[0] ;
  wire ir_sample_valid;
  wire next_sum_carry__0_i_1__0_n_0;
  wire next_sum_carry__0_i_2__0_n_0;
  wire next_sum_carry__0_i_3__0_n_0;
  wire next_sum_carry__0_i_4__0_n_0;
  wire next_sum_carry__0_i_5__0_n_0;
  wire next_sum_carry__0_i_6__0_n_0;
  wire next_sum_carry__0_i_7__0_n_0;
  wire next_sum_carry__0_i_8__0_n_0;
  wire next_sum_carry__0_n_0;
  wire next_sum_carry__0_n_1;
  wire next_sum_carry__0_n_2;
  wire next_sum_carry__0_n_3;
  wire next_sum_carry__0_n_4;
  wire next_sum_carry__0_n_5;
  wire next_sum_carry__0_n_6;
  wire next_sum_carry__0_n_7;
  wire next_sum_carry__1_i_1__0_n_0;
  wire next_sum_carry__1_i_2__0_n_0;
  wire next_sum_carry__1_i_3__0_n_0;
  wire next_sum_carry__1_i_4__0_n_0;
  wire next_sum_carry__1_n_2;
  wire next_sum_carry__1_n_3;
  wire next_sum_carry__1_n_5;
  wire next_sum_carry__1_n_6;
  wire next_sum_carry__1_n_7;
  wire next_sum_carry_i_1__0_n_0;
  wire next_sum_carry_i_2__0_n_0;
  wire next_sum_carry_i_3__0_n_0;
  wire next_sum_carry_i_4__0_n_0;
  wire next_sum_carry_i_5__0_n_0;
  wire next_sum_carry_i_6__0_n_0;
  wire next_sum_carry_i_7__0_n_0;
  wire next_sum_carry_n_0;
  wire next_sum_carry_n_1;
  wire next_sum_carry_n_2;
  wire next_sum_carry_n_3;
  wire next_sum_carry_n_4;
  wire next_sum_carry_n_5;
  wire next_sum_carry_n_6;
  wire next_sum_carry_n_7;
  wire \running_sum_reg[0]_0 ;
  wire \running_sum_reg_n_0_[0] ;
  wire \running_sum_reg_n_0_[1] ;
  wire \running_sum_reg_n_0_[2] ;
  wire s_axi_aclk;
  wire \s_axi_rdata[0]_i_2_n_0 ;
  wire \s_axi_rdata_reg[0] ;
  wire [0:0]\s_axi_rdata_reg[0]_0 ;
  wire [2:0]sel0;
  wire \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ;
  wire \shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire \shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ;
  wire shift_reg_reg_gate__0_n_0;
  wire shift_reg_reg_gate__1_n_0;
  wire shift_reg_reg_gate__2_n_0;
  wire shift_reg_reg_gate__3_n_0;
  wire shift_reg_reg_gate__4_n_0;
  wire shift_reg_reg_gate__5_n_0;
  wire shift_reg_reg_gate__6_n_0;
  wire shift_reg_reg_gate_n_0;
  wire \shift_reg_reg_n_0_[7][0] ;
  wire \shift_reg_reg_n_0_[7][1] ;
  wire \shift_reg_reg_n_0_[7][2] ;
  wire \shift_reg_reg_n_0_[7][3] ;
  wire \shift_reg_reg_n_0_[7][4] ;
  wire \shift_reg_reg_n_0_[7][5] ;
  wire \shift_reg_reg_n_0_[7][6] ;
  wire \shift_reg_reg_n_0_[7][7] ;
  wire shift_reg_reg_r_10_n_0;
  wire shift_reg_reg_r_11_n_0;
  wire shift_reg_reg_r_6_n_0;
  wire shift_reg_reg_r_7_n_0;
  wire shift_reg_reg_r_8_n_0;
  wire shift_reg_reg_r_9_n_0;
  wire shift_reg_reg_r_n_0;
  wire [3:2]NLW_next_sum_carry__1_CO_UNCONNECTED;
  wire [3:3]NLW_next_sum_carry__1_O_UNCONNECTED;

  FDRE \data_out_reg[0] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry_n_4),
        .Q(\data_out_reg_n_0_[0] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[1] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__0_n_7),
        .Q(\data_out_reg[1]_0 ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[2] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__0_n_6),
        .Q(\data_out_reg[2]_0 ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[3] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__0_n_5),
        .Q(\data_out_reg[3]_0 ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[4] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__0_n_4),
        .Q(\data_out_reg[4]_0 ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[5] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__1_n_7),
        .Q(\data_out_reg[5]_0 ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[6] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__1_n_6),
        .Q(\data_out_reg[6]_0 ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \data_out_reg[7] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry__1_n_5),
        .Q(\data_out_reg[7]_0 ),
        .R(\running_sum_reg[0]_0 ));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 next_sum_carry
       (.CI(1'b0),
        .CO({next_sum_carry_n_0,next_sum_carry_n_1,next_sum_carry_n_2,next_sum_carry_n_3}),
        .CYINIT(1'b0),
        .DI({next_sum_carry_i_1__0_n_0,next_sum_carry_i_2__0_n_0,next_sum_carry_i_3__0_n_0,\running_sum_reg_n_0_[0] }),
        .O({next_sum_carry_n_4,next_sum_carry_n_5,next_sum_carry_n_6,next_sum_carry_n_7}),
        .S({next_sum_carry_i_4__0_n_0,next_sum_carry_i_5__0_n_0,next_sum_carry_i_6__0_n_0,next_sum_carry_i_7__0_n_0}));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 next_sum_carry__0
       (.CI(next_sum_carry_n_0),
        .CO({next_sum_carry__0_n_0,next_sum_carry__0_n_1,next_sum_carry__0_n_2,next_sum_carry__0_n_3}),
        .CYINIT(1'b0),
        .DI({next_sum_carry__0_i_1__0_n_0,next_sum_carry__0_i_2__0_n_0,next_sum_carry__0_i_3__0_n_0,next_sum_carry__0_i_4__0_n_0}),
        .O({next_sum_carry__0_n_4,next_sum_carry__0_n_5,next_sum_carry__0_n_6,next_sum_carry__0_n_7}),
        .S({next_sum_carry__0_i_5__0_n_0,next_sum_carry__0_i_6__0_n_0,next_sum_carry__0_i_7__0_n_0,next_sum_carry__0_i_8__0_n_0}));
  (* HLUTNM = "lutpair11" *) 
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry__0_i_1__0
       (.I0(\shift_reg_reg_n_0_[7][6] ),
        .I1(Q[6]),
        .I2(\data_out_reg[3]_0 ),
        .O(next_sum_carry__0_i_1__0_n_0));
  (* HLUTNM = "lutpair10" *) 
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry__0_i_2__0
       (.I0(\shift_reg_reg_n_0_[7][5] ),
        .I1(Q[5]),
        .I2(\data_out_reg[2]_0 ),
        .O(next_sum_carry__0_i_2__0_n_0));
  (* HLUTNM = "lutpair9" *) 
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry__0_i_3__0
       (.I0(\shift_reg_reg_n_0_[7][4] ),
        .I1(Q[4]),
        .I2(\data_out_reg[1]_0 ),
        .O(next_sum_carry__0_i_3__0_n_0));
  (* HLUTNM = "lutpair8" *) 
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry__0_i_4__0
       (.I0(\shift_reg_reg_n_0_[7][3] ),
        .I1(Q[3]),
        .I2(\data_out_reg_n_0_[0] ),
        .O(next_sum_carry__0_i_4__0_n_0));
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_5__0
       (.I0(next_sum_carry__0_i_1__0_n_0),
        .I1(\shift_reg_reg_n_0_[7][7] ),
        .I2(Q[7]),
        .I3(\data_out_reg[4]_0 ),
        .O(next_sum_carry__0_i_5__0_n_0));
  (* HLUTNM = "lutpair11" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_6__0
       (.I0(\shift_reg_reg_n_0_[7][6] ),
        .I1(Q[6]),
        .I2(\data_out_reg[3]_0 ),
        .I3(next_sum_carry__0_i_2__0_n_0),
        .O(next_sum_carry__0_i_6__0_n_0));
  (* HLUTNM = "lutpair10" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_7__0
       (.I0(\shift_reg_reg_n_0_[7][5] ),
        .I1(Q[5]),
        .I2(\data_out_reg[2]_0 ),
        .I3(next_sum_carry__0_i_3__0_n_0),
        .O(next_sum_carry__0_i_7__0_n_0));
  (* HLUTNM = "lutpair9" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_8__0
       (.I0(\shift_reg_reg_n_0_[7][4] ),
        .I1(Q[4]),
        .I2(\data_out_reg[1]_0 ),
        .I3(next_sum_carry__0_i_4__0_n_0),
        .O(next_sum_carry__0_i_8__0_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 next_sum_carry__1
       (.CI(next_sum_carry__0_n_0),
        .CO({NLW_next_sum_carry__1_CO_UNCONNECTED[3:2],next_sum_carry__1_n_2,next_sum_carry__1_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,\data_out_reg[5]_0 ,next_sum_carry__1_i_1__0_n_0}),
        .O({NLW_next_sum_carry__1_O_UNCONNECTED[3],next_sum_carry__1_n_5,next_sum_carry__1_n_6,next_sum_carry__1_n_7}),
        .S({1'b0,next_sum_carry__1_i_2__0_n_0,next_sum_carry__1_i_3__0_n_0,next_sum_carry__1_i_4__0_n_0}));
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry__1_i_1__0
       (.I0(\shift_reg_reg_n_0_[7][7] ),
        .I1(Q[7]),
        .I2(\data_out_reg[4]_0 ),
        .O(next_sum_carry__1_i_1__0_n_0));
  LUT2 #(
    .INIT(4'h9)) 
    next_sum_carry__1_i_2__0
       (.I0(\data_out_reg[6]_0 ),
        .I1(\data_out_reg[7]_0 ),
        .O(next_sum_carry__1_i_2__0_n_0));
  LUT2 #(
    .INIT(4'h9)) 
    next_sum_carry__1_i_3__0
       (.I0(\data_out_reg[5]_0 ),
        .I1(\data_out_reg[6]_0 ),
        .O(next_sum_carry__1_i_3__0_n_0));
  LUT4 #(
    .INIT(16'h8E71)) 
    next_sum_carry__1_i_4__0
       (.I0(\data_out_reg[4]_0 ),
        .I1(Q[7]),
        .I2(\shift_reg_reg_n_0_[7][7] ),
        .I3(\data_out_reg[5]_0 ),
        .O(next_sum_carry__1_i_4__0_n_0));
  (* HLUTNM = "lutpair7" *) 
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry_i_1__0
       (.I0(\shift_reg_reg_n_0_[7][2] ),
        .I1(\running_sum_reg_n_0_[2] ),
        .I2(Q[2]),
        .O(next_sum_carry_i_1__0_n_0));
  (* HLUTNM = "lutpair6" *) 
  LUT3 #(
    .INIT(8'hD4)) 
    next_sum_carry_i_2__0
       (.I0(\shift_reg_reg_n_0_[7][1] ),
        .I1(\running_sum_reg_n_0_[1] ),
        .I2(Q[1]),
        .O(next_sum_carry_i_2__0_n_0));
  (* HLUTNM = "lutpair13" *) 
  LUT2 #(
    .INIT(4'hB)) 
    next_sum_carry_i_3__0
       (.I0(Q[0]),
        .I1(\shift_reg_reg_n_0_[7][0] ),
        .O(next_sum_carry_i_3__0_n_0));
  (* HLUTNM = "lutpair8" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry_i_4__0
       (.I0(\shift_reg_reg_n_0_[7][3] ),
        .I1(Q[3]),
        .I2(\data_out_reg_n_0_[0] ),
        .I3(next_sum_carry_i_1__0_n_0),
        .O(next_sum_carry_i_4__0_n_0));
  (* HLUTNM = "lutpair7" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry_i_5__0
       (.I0(\shift_reg_reg_n_0_[7][2] ),
        .I1(\running_sum_reg_n_0_[2] ),
        .I2(Q[2]),
        .I3(next_sum_carry_i_2__0_n_0),
        .O(next_sum_carry_i_5__0_n_0));
  (* HLUTNM = "lutpair6" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry_i_6__0
       (.I0(\shift_reg_reg_n_0_[7][1] ),
        .I1(\running_sum_reg_n_0_[1] ),
        .I2(Q[1]),
        .I3(next_sum_carry_i_3__0_n_0),
        .O(next_sum_carry_i_6__0_n_0));
  (* HLUTNM = "lutpair13" *) 
  LUT3 #(
    .INIT(8'h96)) 
    next_sum_carry_i_7__0
       (.I0(Q[0]),
        .I1(\shift_reg_reg_n_0_[7][0] ),
        .I2(\running_sum_reg_n_0_[0] ),
        .O(next_sum_carry_i_7__0_n_0));
  FDRE \running_sum_reg[0] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry_n_7),
        .Q(\running_sum_reg_n_0_[0] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \running_sum_reg[1] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry_n_6),
        .Q(\running_sum_reg_n_0_[1] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \running_sum_reg[2] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(next_sum_carry_n_5),
        .Q(\running_sum_reg_n_0_[2] ),
        .R(\running_sum_reg[0]_0 ));
  LUT2 #(
    .INIT(4'hE)) 
    \s_axi_rdata[0]_i_1 
       (.I0(\s_axi_rdata[0]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[0] ),
        .O(D));
  LUT6 #(
    .INIT(64'h0000CC0000F0AA00)) 
    \s_axi_rdata[0]_i_2 
       (.I0(Q[0]),
        .I1(\data_out_reg_n_0_[0] ),
        .I2(\s_axi_rdata_reg[0]_0 ),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(\s_axi_rdata[0]_i_2_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[0]),
        .Q(\shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[1]),
        .Q(\shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[2]),
        .Q(\shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[3]),
        .Q(\shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[4]),
        .Q(\shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[5]),
        .Q(\shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[6]),
        .Q(\shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_ir/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_ir/shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 " *) 
  SRL16E \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(ir_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[7]),
        .Q(\shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ));
  FDRE \shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(\shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0 ),
        .Q(\shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[7][0] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__6_n_0),
        .Q(\shift_reg_reg_n_0_[7][0] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][1] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__5_n_0),
        .Q(\shift_reg_reg_n_0_[7][1] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][2] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__4_n_0),
        .Q(\shift_reg_reg_n_0_[7][2] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][3] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__3_n_0),
        .Q(\shift_reg_reg_n_0_[7][3] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][4] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__2_n_0),
        .Q(\shift_reg_reg_n_0_[7][4] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][5] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__1_n_0),
        .Q(\shift_reg_reg_n_0_[7][5] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][6] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate__0_n_0),
        .Q(\shift_reg_reg_n_0_[7][6] ),
        .R(\running_sum_reg[0]_0 ));
  FDRE \shift_reg_reg[7][7] 
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_gate_n_0),
        .Q(\shift_reg_reg_n_0_[7][7] ),
        .R(\running_sum_reg[0]_0 ));
  (* SOFT_HLUTNM = "soft_lutpair0" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate
       (.I0(\shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate_n_0));
  (* SOFT_HLUTNM = "soft_lutpair0" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__0
       (.I0(\shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__0_n_0));
  (* SOFT_HLUTNM = "soft_lutpair1" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__1
       (.I0(\shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__1_n_0));
  (* SOFT_HLUTNM = "soft_lutpair1" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__2
       (.I0(\shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__2_n_0));
  (* SOFT_HLUTNM = "soft_lutpair2" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__3
       (.I0(\shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__3_n_0));
  (* SOFT_HLUTNM = "soft_lutpair2" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__4
       (.I0(\shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__4_n_0));
  (* SOFT_HLUTNM = "soft_lutpair3" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__5
       (.I0(\shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__5_n_0));
  (* SOFT_HLUTNM = "soft_lutpair3" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__6
       (.I0(\shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11_n_0 ),
        .I1(shift_reg_reg_r_11_n_0),
        .O(shift_reg_reg_gate__6_n_0));
  FDRE shift_reg_reg_r
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(1'b1),
        .Q(shift_reg_reg_r_n_0),
        .R(\running_sum_reg[0]_0 ));
  FDRE shift_reg_reg_r_10
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_r_9_n_0),
        .Q(shift_reg_reg_r_10_n_0),
        .R(\running_sum_reg[0]_0 ));
  FDRE shift_reg_reg_r_11
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_r_10_n_0),
        .Q(shift_reg_reg_r_11_n_0),
        .R(\running_sum_reg[0]_0 ));
  FDRE shift_reg_reg_r_6
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_r_n_0),
        .Q(shift_reg_reg_r_6_n_0),
        .R(\running_sum_reg[0]_0 ));
  FDRE shift_reg_reg_r_7
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_r_6_n_0),
        .Q(shift_reg_reg_r_7_n_0),
        .R(\running_sum_reg[0]_0 ));
  FDRE shift_reg_reg_r_8
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_r_7_n_0),
        .Q(shift_reg_reg_r_8_n_0),
        .R(\running_sum_reg[0]_0 ));
  FDRE shift_reg_reg_r_9
       (.C(s_axi_aclk),
        .CE(ir_sample_valid),
        .D(shift_reg_reg_r_8_n_0),
        .Q(shift_reg_reg_r_9_n_0),
        .R(\running_sum_reg[0]_0 ));
endmodule

(* ORIG_REF_NAME = "moving_average_8tap" *) 
module design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0
   (red_filter_valid,
    data_out,
    DI,
    \data_out_reg[6]_0 ,
    S,
    \reg_red_raw_reg[0] ,
    D,
    \data_out_reg[6]_1 ,
    \data_out_reg[0]_0 ,
    red_sample_valid,
    s_axi_aclk,
    Q,
    data3,
    sel0,
    \s_axi_rdata_reg[1] ,
    \s_axi_rdata_reg[7] ,
    \s_axi_rdata_reg[2] ,
    \s_axi_rdata_reg[3] ,
    \s_axi_rdata_reg[4] ,
    \s_axi_rdata_reg[5] ,
    \s_axi_rdata_reg[6] ,
    \s_axi_rdata_reg[7]_0 ,
    \s_axi_rdata_reg[7]_1 );
  output red_filter_valid;
  output [7:0]data_out;
  output [3:0]DI;
  output [3:0]\data_out_reg[6]_0 ;
  output [3:0]S;
  output \reg_red_raw_reg[0] ;
  output [6:0]D;
  output [3:0]\data_out_reg[6]_1 ;
  input \data_out_reg[0]_0 ;
  input red_sample_valid;
  input s_axi_aclk;
  input [7:0]Q;
  input [8:0]data3;
  input [2:0]sel0;
  input \s_axi_rdata_reg[1] ;
  input [6:0]\s_axi_rdata_reg[7] ;
  input \s_axi_rdata_reg[2] ;
  input \s_axi_rdata_reg[3] ;
  input \s_axi_rdata_reg[4] ;
  input \s_axi_rdata_reg[5] ;
  input \s_axi_rdata_reg[6] ;
  input \s_axi_rdata_reg[7]_0 ;
  input [6:0]\s_axi_rdata_reg[7]_1 ;

  wire [6:0]D;
  wire [3:0]DI;
  wire [7:0]Q;
  wire [3:0]S;
  wire [8:0]data3;
  wire [7:0]data_out;
  wire \data_out_reg[0]_0 ;
  wire [3:0]\data_out_reg[6]_0 ;
  wire [3:0]\data_out_reg[6]_1 ;
  wire next_sum_carry__0_i_1_n_0;
  wire next_sum_carry__0_i_2_n_0;
  wire next_sum_carry__0_i_3_n_0;
  wire next_sum_carry__0_i_4_n_0;
  wire next_sum_carry__0_i_5_n_0;
  wire next_sum_carry__0_i_6_n_0;
  wire next_sum_carry__0_i_7_n_0;
  wire next_sum_carry__0_i_8_n_0;
  wire next_sum_carry__0_n_0;
  wire next_sum_carry__0_n_1;
  wire next_sum_carry__0_n_2;
  wire next_sum_carry__0_n_3;
  wire next_sum_carry__1_i_1_n_0;
  wire next_sum_carry__1_i_2_n_0;
  wire next_sum_carry__1_i_3_n_0;
  wire next_sum_carry__1_i_4_n_0;
  wire next_sum_carry__1_n_2;
  wire next_sum_carry__1_n_3;
  wire next_sum_carry_i_1_n_0;
  wire next_sum_carry_i_2_n_0;
  wire next_sum_carry_i_3_n_0;
  wire next_sum_carry_i_4_n_0;
  wire next_sum_carry_i_5_n_0;
  wire next_sum_carry_i_6_n_0;
  wire next_sum_carry_i_7_n_0;
  wire next_sum_carry_n_0;
  wire next_sum_carry_n_1;
  wire next_sum_carry_n_2;
  wire next_sum_carry_n_3;
  wire next_sum_carry_n_5;
  wire next_sum_carry_n_6;
  wire next_sum_carry_n_7;
  wire [7:0]p_1_in;
  wire red_filter_valid;
  wire red_sample_valid;
  wire \reg_red_raw_reg[0] ;
  wire [2:0]running_sum;
  wire s_axi_aclk;
  wire \s_axi_rdata[1]_i_2_n_0 ;
  wire \s_axi_rdata[2]_i_2_n_0 ;
  wire \s_axi_rdata[3]_i_2_n_0 ;
  wire \s_axi_rdata[4]_i_2_n_0 ;
  wire \s_axi_rdata[5]_i_2_n_0 ;
  wire \s_axi_rdata[6]_i_2_n_0 ;
  wire \s_axi_rdata[7]_i_2_n_0 ;
  wire \s_axi_rdata_reg[1] ;
  wire \s_axi_rdata_reg[2] ;
  wire \s_axi_rdata_reg[3] ;
  wire \s_axi_rdata_reg[4] ;
  wire \s_axi_rdata_reg[5] ;
  wire \s_axi_rdata_reg[6] ;
  wire [6:0]\s_axi_rdata_reg[7] ;
  wire \s_axi_rdata_reg[7]_0 ;
  wire [6:0]\s_axi_rdata_reg[7]_1 ;
  wire [2:0]sel0;
  wire \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ;
  wire \shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire \shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ;
  wire [7:0]\shift_reg_reg[7] ;
  wire shift_reg_reg_gate__0_n_0;
  wire shift_reg_reg_gate__1_n_0;
  wire shift_reg_reg_gate__2_n_0;
  wire shift_reg_reg_gate__3_n_0;
  wire shift_reg_reg_gate__4_n_0;
  wire shift_reg_reg_gate__5_n_0;
  wire shift_reg_reg_gate__6_n_0;
  wire shift_reg_reg_gate_n_0;
  wire shift_reg_reg_r_0_n_0;
  wire shift_reg_reg_r_1_n_0;
  wire shift_reg_reg_r_2_n_0;
  wire shift_reg_reg_r_3_n_0;
  wire shift_reg_reg_r_4_n_0;
  wire shift_reg_reg_r_5_n_0;
  wire shift_reg_reg_r_n_0;
  wire [3:2]NLW_next_sum_carry__1_CO_UNCONNECTED;
  wire [3:3]NLW_next_sum_carry__1_O_UNCONNECTED;

  FDRE \data_out_reg[0] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[0]),
        .Q(data_out[0]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[1] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[1]),
        .Q(data_out[1]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[2] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[2]),
        .Q(data_out[2]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[3] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[3]),
        .Q(data_out[3]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[4] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[4]),
        .Q(data_out[4]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[5] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[5]),
        .Q(data_out[5]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[6] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[6]),
        .Q(data_out[6]),
        .R(\data_out_reg[0]_0 ));
  FDRE \data_out_reg[7] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(p_1_in[7]),
        .Q(data_out[7]),
        .R(\data_out_reg[0]_0 ));
  LUT4 #(
    .INIT(16'h20F2)) 
    i__carry_i_1
       (.I0(data3[7]),
        .I1(data_out[6]),
        .I2(data3[8]),
        .I3(data_out[7]),
        .O(DI[3]));
  LUT4 #(
    .INIT(16'h20F2)) 
    i__carry_i_2
       (.I0(data3[5]),
        .I1(data_out[4]),
        .I2(data3[6]),
        .I3(data_out[5]),
        .O(DI[2]));
  LUT4 #(
    .INIT(16'h20F2)) 
    i__carry_i_3
       (.I0(data3[3]),
        .I1(data_out[2]),
        .I2(data3[4]),
        .I3(data_out[3]),
        .O(DI[1]));
  LUT4 #(
    .INIT(16'h20F2)) 
    i__carry_i_4
       (.I0(data3[1]),
        .I1(data_out[0]),
        .I2(data3[2]),
        .I3(data_out[1]),
        .O(DI[0]));
  LUT4 #(
    .INIT(16'h9009)) 
    i__carry_i_5
       (.I0(data_out[6]),
        .I1(data3[7]),
        .I2(data_out[7]),
        .I3(data3[8]),
        .O(S[3]));
  LUT4 #(
    .INIT(16'h9009)) 
    i__carry_i_6
       (.I0(data_out[4]),
        .I1(data3[5]),
        .I2(data_out[5]),
        .I3(data3[6]),
        .O(S[2]));
  LUT4 #(
    .INIT(16'h9009)) 
    i__carry_i_7
       (.I0(data_out[2]),
        .I1(data3[3]),
        .I2(data_out[3]),
        .I3(data3[4]),
        .O(S[1]));
  LUT4 #(
    .INIT(16'h9009)) 
    i__carry_i_8
       (.I0(data_out[0]),
        .I1(data3[1]),
        .I2(data_out[1]),
        .I3(data3[2]),
        .O(S[0]));
  LUT4 #(
    .INIT(16'h20F2)) 
    next_state1_carry_i_1
       (.I0(data_out[6]),
        .I1(data3[7]),
        .I2(data_out[7]),
        .I3(data3[8]),
        .O(\data_out_reg[6]_0 [3]));
  LUT4 #(
    .INIT(16'h20F2)) 
    next_state1_carry_i_2
       (.I0(data_out[4]),
        .I1(data3[5]),
        .I2(data_out[5]),
        .I3(data3[6]),
        .O(\data_out_reg[6]_0 [2]));
  LUT4 #(
    .INIT(16'h20F2)) 
    next_state1_carry_i_3
       (.I0(data_out[2]),
        .I1(data3[3]),
        .I2(data_out[3]),
        .I3(data3[4]),
        .O(\data_out_reg[6]_0 [1]));
  LUT4 #(
    .INIT(16'h20F2)) 
    next_state1_carry_i_4
       (.I0(data_out[0]),
        .I1(data3[1]),
        .I2(data_out[1]),
        .I3(data3[2]),
        .O(\data_out_reg[6]_0 [0]));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state1_carry_i_5
       (.I0(data_out[6]),
        .I1(data3[7]),
        .I2(data_out[7]),
        .I3(data3[8]),
        .O(\data_out_reg[6]_1 [3]));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state1_carry_i_6
       (.I0(data_out[4]),
        .I1(data3[5]),
        .I2(data_out[5]),
        .I3(data3[6]),
        .O(\data_out_reg[6]_1 [2]));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state1_carry_i_7
       (.I0(data_out[2]),
        .I1(data3[3]),
        .I2(data_out[3]),
        .I3(data3[4]),
        .O(\data_out_reg[6]_1 [1]));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state1_carry_i_8
       (.I0(data_out[0]),
        .I1(data3[1]),
        .I2(data_out[1]),
        .I3(data3[2]),
        .O(\data_out_reg[6]_1 [0]));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 next_sum_carry
       (.CI(1'b0),
        .CO({next_sum_carry_n_0,next_sum_carry_n_1,next_sum_carry_n_2,next_sum_carry_n_3}),
        .CYINIT(1'b0),
        .DI({next_sum_carry_i_1_n_0,next_sum_carry_i_2_n_0,next_sum_carry_i_3_n_0,running_sum[0]}),
        .O({p_1_in[0],next_sum_carry_n_5,next_sum_carry_n_6,next_sum_carry_n_7}),
        .S({next_sum_carry_i_4_n_0,next_sum_carry_i_5_n_0,next_sum_carry_i_6_n_0,next_sum_carry_i_7_n_0}));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 next_sum_carry__0
       (.CI(next_sum_carry_n_0),
        .CO({next_sum_carry__0_n_0,next_sum_carry__0_n_1,next_sum_carry__0_n_2,next_sum_carry__0_n_3}),
        .CYINIT(1'b0),
        .DI({next_sum_carry__0_i_1_n_0,next_sum_carry__0_i_2_n_0,next_sum_carry__0_i_3_n_0,next_sum_carry__0_i_4_n_0}),
        .O(p_1_in[4:1]),
        .S({next_sum_carry__0_i_5_n_0,next_sum_carry__0_i_6_n_0,next_sum_carry__0_i_7_n_0,next_sum_carry__0_i_8_n_0}));
  (* HLUTNM = "lutpair5" *) 
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry__0_i_1
       (.I0(data_out[3]),
        .I1(Q[6]),
        .I2(\shift_reg_reg[7] [6]),
        .O(next_sum_carry__0_i_1_n_0));
  (* HLUTNM = "lutpair4" *) 
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry__0_i_2
       (.I0(data_out[2]),
        .I1(Q[5]),
        .I2(\shift_reg_reg[7] [5]),
        .O(next_sum_carry__0_i_2_n_0));
  (* HLUTNM = "lutpair3" *) 
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry__0_i_3
       (.I0(data_out[1]),
        .I1(Q[4]),
        .I2(\shift_reg_reg[7] [4]),
        .O(next_sum_carry__0_i_3_n_0));
  (* HLUTNM = "lutpair2" *) 
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry__0_i_4
       (.I0(data_out[0]),
        .I1(Q[3]),
        .I2(\shift_reg_reg[7] [3]),
        .O(next_sum_carry__0_i_4_n_0));
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_5
       (.I0(next_sum_carry__0_i_1_n_0),
        .I1(\shift_reg_reg[7] [7]),
        .I2(Q[7]),
        .I3(data_out[4]),
        .O(next_sum_carry__0_i_5_n_0));
  (* HLUTNM = "lutpair5" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_6
       (.I0(data_out[3]),
        .I1(Q[6]),
        .I2(\shift_reg_reg[7] [6]),
        .I3(next_sum_carry__0_i_2_n_0),
        .O(next_sum_carry__0_i_6_n_0));
  (* HLUTNM = "lutpair4" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_7
       (.I0(data_out[2]),
        .I1(Q[5]),
        .I2(\shift_reg_reg[7] [5]),
        .I3(next_sum_carry__0_i_3_n_0),
        .O(next_sum_carry__0_i_7_n_0));
  (* HLUTNM = "lutpair3" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry__0_i_8
       (.I0(data_out[1]),
        .I1(Q[4]),
        .I2(\shift_reg_reg[7] [4]),
        .I3(next_sum_carry__0_i_4_n_0),
        .O(next_sum_carry__0_i_8_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 next_sum_carry__1
       (.CI(next_sum_carry__0_n_0),
        .CO({NLW_next_sum_carry__1_CO_UNCONNECTED[3:2],next_sum_carry__1_n_2,next_sum_carry__1_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,data_out[5],next_sum_carry__1_i_1_n_0}),
        .O({NLW_next_sum_carry__1_O_UNCONNECTED[3],p_1_in[7:5]}),
        .S({1'b0,next_sum_carry__1_i_2_n_0,next_sum_carry__1_i_3_n_0,next_sum_carry__1_i_4_n_0}));
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry__1_i_1
       (.I0(data_out[4]),
        .I1(Q[7]),
        .I2(\shift_reg_reg[7] [7]),
        .O(next_sum_carry__1_i_1_n_0));
  LUT2 #(
    .INIT(4'h9)) 
    next_sum_carry__1_i_2
       (.I0(data_out[6]),
        .I1(data_out[7]),
        .O(next_sum_carry__1_i_2_n_0));
  LUT2 #(
    .INIT(4'h9)) 
    next_sum_carry__1_i_3
       (.I0(data_out[5]),
        .I1(data_out[6]),
        .O(next_sum_carry__1_i_3_n_0));
  LUT4 #(
    .INIT(16'hD42B)) 
    next_sum_carry__1_i_4
       (.I0(\shift_reg_reg[7] [7]),
        .I1(Q[7]),
        .I2(data_out[4]),
        .I3(data_out[5]),
        .O(next_sum_carry__1_i_4_n_0));
  (* HLUTNM = "lutpair1" *) 
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry_i_1
       (.I0(Q[2]),
        .I1(running_sum[2]),
        .I2(\shift_reg_reg[7] [2]),
        .O(next_sum_carry_i_1_n_0));
  (* HLUTNM = "lutpair0" *) 
  LUT3 #(
    .INIT(8'h8E)) 
    next_sum_carry_i_2
       (.I0(Q[1]),
        .I1(running_sum[1]),
        .I2(\shift_reg_reg[7] [1]),
        .O(next_sum_carry_i_2_n_0));
  (* HLUTNM = "lutpair12" *) 
  LUT2 #(
    .INIT(4'hB)) 
    next_sum_carry_i_3
       (.I0(Q[0]),
        .I1(\shift_reg_reg[7] [0]),
        .O(next_sum_carry_i_3_n_0));
  (* HLUTNM = "lutpair2" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry_i_4
       (.I0(data_out[0]),
        .I1(Q[3]),
        .I2(\shift_reg_reg[7] [3]),
        .I3(next_sum_carry_i_1_n_0),
        .O(next_sum_carry_i_4_n_0));
  (* HLUTNM = "lutpair1" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry_i_5
       (.I0(Q[2]),
        .I1(running_sum[2]),
        .I2(\shift_reg_reg[7] [2]),
        .I3(next_sum_carry_i_2_n_0),
        .O(next_sum_carry_i_5_n_0));
  (* HLUTNM = "lutpair0" *) 
  LUT4 #(
    .INIT(16'h9669)) 
    next_sum_carry_i_6
       (.I0(Q[1]),
        .I1(running_sum[1]),
        .I2(\shift_reg_reg[7] [1]),
        .I3(next_sum_carry_i_3_n_0),
        .O(next_sum_carry_i_6_n_0));
  (* HLUTNM = "lutpair12" *) 
  LUT3 #(
    .INIT(8'h96)) 
    next_sum_carry_i_7
       (.I0(Q[0]),
        .I1(\shift_reg_reg[7] [0]),
        .I2(running_sum[0]),
        .O(next_sum_carry_i_7_n_0));
  FDRE out_valid_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(red_sample_valid),
        .Q(red_filter_valid),
        .R(\data_out_reg[0]_0 ));
  FDRE \running_sum_reg[0] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(next_sum_carry_n_7),
        .Q(running_sum[0]),
        .R(\data_out_reg[0]_0 ));
  FDRE \running_sum_reg[1] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(next_sum_carry_n_6),
        .Q(running_sum[1]),
        .R(\data_out_reg[0]_0 ));
  FDRE \running_sum_reg[2] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(next_sum_carry_n_5),
        .Q(running_sum[2]),
        .R(\data_out_reg[0]_0 ));
  LUT6 #(
    .INIT(64'h00F000CC000000AA)) 
    \s_axi_rdata[0]_i_3 
       (.I0(Q[0]),
        .I1(data_out[0]),
        .I2(data3[0]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(\reg_red_raw_reg[0] ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[1]_i_1 
       (.I0(\s_axi_rdata[1]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[1] ),
        .I2(\s_axi_rdata_reg[7] [0]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[0]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[1]_i_2 
       (.I0(Q[1]),
        .I1(data_out[1]),
        .I2(\s_axi_rdata_reg[7]_1 [0]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[1]_i_2_n_0 ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[2]_i_1 
       (.I0(\s_axi_rdata[2]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[2] ),
        .I2(\s_axi_rdata_reg[7] [1]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[1]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[2]_i_2 
       (.I0(Q[2]),
        .I1(data_out[2]),
        .I2(\s_axi_rdata_reg[7]_1 [1]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[2]_i_2_n_0 ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[3]_i_1 
       (.I0(\s_axi_rdata[3]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[3] ),
        .I2(\s_axi_rdata_reg[7] [2]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[2]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[3]_i_2 
       (.I0(Q[3]),
        .I1(data_out[3]),
        .I2(\s_axi_rdata_reg[7]_1 [2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[3]_i_2_n_0 ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[4]_i_1 
       (.I0(\s_axi_rdata[4]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[4] ),
        .I2(\s_axi_rdata_reg[7] [3]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[3]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[4]_i_2 
       (.I0(Q[4]),
        .I1(data_out[4]),
        .I2(\s_axi_rdata_reg[7]_1 [3]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[4]_i_2_n_0 ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[5]_i_1 
       (.I0(\s_axi_rdata[5]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[5] ),
        .I2(\s_axi_rdata_reg[7] [4]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[4]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[5]_i_2 
       (.I0(Q[5]),
        .I1(data_out[5]),
        .I2(\s_axi_rdata_reg[7]_1 [4]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[5]_i_2_n_0 ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[6]_i_1 
       (.I0(\s_axi_rdata[6]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[6] ),
        .I2(\s_axi_rdata_reg[7] [5]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[5]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[6]_i_2 
       (.I0(Q[6]),
        .I1(data_out[6]),
        .I2(\s_axi_rdata_reg[7]_1 [5]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[6]_i_2_n_0 ));
  LUT6 #(
    .INIT(64'hAAAAEEAAAAFAAAAA)) 
    \s_axi_rdata[7]_i_1 
       (.I0(\s_axi_rdata[7]_i_2_n_0 ),
        .I1(\s_axi_rdata_reg[7]_0 ),
        .I2(\s_axi_rdata_reg[7] [6]),
        .I3(sel0[2]),
        .I4(sel0[1]),
        .I5(sel0[0]),
        .O(D[6]));
  LUT6 #(
    .INIT(64'h000000F000CC00AA)) 
    \s_axi_rdata[7]_i_2 
       (.I0(Q[7]),
        .I1(data_out[7]),
        .I2(\s_axi_rdata_reg[7]_1 [6]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .I5(sel0[2]),
        .O(\s_axi_rdata[7]_i_2_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[0]),
        .Q(\shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[1]),
        .Q(\shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[2]),
        .Q(\shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[3]),
        .Q(\shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[4]),
        .Q(\shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[5]),
        .Q(\shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[6]),
        .Q(\shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  (* srl_bus_name = "\\inst/u_filter_red/shift_reg_reg[5] " *) 
  (* srl_name = "\\inst/u_filter_red/shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4 " *) 
  SRL16E \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4 
       (.A0(1'b1),
        .A1(1'b0),
        .A2(1'b1),
        .A3(1'b0),
        .CE(red_sample_valid),
        .CLK(s_axi_aclk),
        .D(Q[7]),
        .Q(\shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ));
  FDRE \shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(\shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0 ),
        .Q(\shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .R(1'b0));
  FDRE \shift_reg_reg[7][0] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__6_n_0),
        .Q(\shift_reg_reg[7] [0]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][1] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__5_n_0),
        .Q(\shift_reg_reg[7] [1]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][2] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__4_n_0),
        .Q(\shift_reg_reg[7] [2]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][3] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__3_n_0),
        .Q(\shift_reg_reg[7] [3]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][4] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__2_n_0),
        .Q(\shift_reg_reg[7] [4]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][5] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__1_n_0),
        .Q(\shift_reg_reg[7] [5]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][6] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate__0_n_0),
        .Q(\shift_reg_reg[7] [6]),
        .R(\data_out_reg[0]_0 ));
  FDRE \shift_reg_reg[7][7] 
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_gate_n_0),
        .Q(\shift_reg_reg[7] [7]),
        .R(\data_out_reg[0]_0 ));
  (* SOFT_HLUTNM = "soft_lutpair4" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate
       (.I0(\shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate_n_0));
  (* SOFT_HLUTNM = "soft_lutpair4" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__0
       (.I0(\shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__0_n_0));
  (* SOFT_HLUTNM = "soft_lutpair5" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__1
       (.I0(\shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__1_n_0));
  (* SOFT_HLUTNM = "soft_lutpair5" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__2
       (.I0(\shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__2_n_0));
  (* SOFT_HLUTNM = "soft_lutpair6" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__3
       (.I0(\shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__3_n_0));
  (* SOFT_HLUTNM = "soft_lutpair6" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__4
       (.I0(\shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__4_n_0));
  (* SOFT_HLUTNM = "soft_lutpair7" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__5
       (.I0(\shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__5_n_0));
  (* SOFT_HLUTNM = "soft_lutpair7" *) 
  LUT2 #(
    .INIT(4'h8)) 
    shift_reg_reg_gate__6
       (.I0(\shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5_n_0 ),
        .I1(shift_reg_reg_r_5_n_0),
        .O(shift_reg_reg_gate__6_n_0));
  FDRE shift_reg_reg_r
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(1'b1),
        .Q(shift_reg_reg_r_n_0),
        .R(\data_out_reg[0]_0 ));
  FDRE shift_reg_reg_r_0
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_r_n_0),
        .Q(shift_reg_reg_r_0_n_0),
        .R(\data_out_reg[0]_0 ));
  FDRE shift_reg_reg_r_1
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_r_0_n_0),
        .Q(shift_reg_reg_r_1_n_0),
        .R(\data_out_reg[0]_0 ));
  FDRE shift_reg_reg_r_2
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_r_1_n_0),
        .Q(shift_reg_reg_r_2_n_0),
        .R(\data_out_reg[0]_0 ));
  FDRE shift_reg_reg_r_3
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_r_2_n_0),
        .Q(shift_reg_reg_r_3_n_0),
        .R(\data_out_reg[0]_0 ));
  FDRE shift_reg_reg_r_4
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_r_3_n_0),
        .Q(shift_reg_reg_r_4_n_0),
        .R(\data_out_reg[0]_0 ));
  FDRE shift_reg_reg_r_5
       (.C(s_axi_aclk),
        .CE(red_sample_valid),
        .D(shift_reg_reg_r_4_n_0),
        .Q(shift_reg_reg_r_5_n_0),
        .R(\data_out_reg[0]_0 ));
endmodule

(* ORIG_REF_NAME = "ppg_peak_detector" *) 
module design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector
   (beat_detected_reg_0,
    SR,
    \axi_araddr_latched_reg[2] ,
    Q,
    beat_detected_reg_1,
    s_axi_aclk,
    \FSM_sequential_current_state[1]_i_2_0 ,
    \FSM_sequential_current_state[1]_i_2_1 ,
    DI,
    S,
    D,
    red_filter_valid,
    data3,
    sel0,
    s_axi_aresetn,
    beat_flag_reg,
    p_1_in_0,
    write_execute);
  output beat_detected_reg_0;
  output [0:0]SR;
  output [23:0]\axi_araddr_latched_reg[2] ;
  output [7:0]Q;
  output beat_detected_reg_1;
  input s_axi_aclk;
  input [3:0]\FSM_sequential_current_state[1]_i_2_0 ;
  input [3:0]\FSM_sequential_current_state[1]_i_2_1 ;
  input [3:0]DI;
  input [3:0]S;
  input [7:0]D;
  input red_filter_valid;
  input [8:0]data3;
  input [2:0]sel0;
  input s_axi_aresetn;
  input [0:0]beat_flag_reg;
  input [2:0]p_1_in_0;
  input write_execute;

  wire [7:0]D;
  wire [3:0]DI;
  wire \FSM_sequential_current_state[0]_i_1_n_0 ;
  wire \FSM_sequential_current_state[1]_i_10_n_0 ;
  wire \FSM_sequential_current_state[1]_i_11_n_0 ;
  wire \FSM_sequential_current_state[1]_i_12_n_0 ;
  wire \FSM_sequential_current_state[1]_i_1_n_0 ;
  wire [3:0]\FSM_sequential_current_state[1]_i_2_0 ;
  wire [3:0]\FSM_sequential_current_state[1]_i_2_1 ;
  wire \FSM_sequential_current_state[1]_i_2_n_0 ;
  wire \FSM_sequential_current_state[1]_i_3_n_0 ;
  wire \FSM_sequential_current_state[1]_i_4_n_0 ;
  wire \FSM_sequential_current_state[1]_i_5_n_0 ;
  wire \FSM_sequential_current_state[1]_i_6_n_0 ;
  wire \FSM_sequential_current_state[1]_i_7_n_0 ;
  wire \FSM_sequential_current_state[1]_i_8_n_0 ;
  wire \FSM_sequential_current_state[1]_i_9_n_0 ;
  wire [7:0]Q;
  wire [3:0]S;
  wire [0:0]SR;
  wire [23:0]\axi_araddr_latched_reg[2] ;
  wire beat_detected_i_1_n_0;
  wire beat_detected_reg_0;
  wire beat_detected_reg_1;
  wire beat_flag_i_2_n_0;
  wire beat_flag_i_3_n_0;
  wire [0:0]beat_flag_reg;
  wire [1:0]current_state;
  wire [8:0]data3;
  wire \fall_count[0]_i_1_n_0 ;
  wire \fall_count[1]_i_1_n_0 ;
  wire \fall_count_reg_n_0_[0] ;
  wire \fall_count_reg_n_0_[1] ;
  wire first_beat_seen_i_1_n_0;
  wire first_beat_seen_reg_n_0;
  wire [31:8]ibi_cycles;
  wire [31:1]in7;
  wire [31:1]in9;
  wire [31:0]interval_cnt;
  wire interval_cnt0_carry__0_n_0;
  wire interval_cnt0_carry__0_n_1;
  wire interval_cnt0_carry__0_n_2;
  wire interval_cnt0_carry__0_n_3;
  wire interval_cnt0_carry__1_n_0;
  wire interval_cnt0_carry__1_n_1;
  wire interval_cnt0_carry__1_n_2;
  wire interval_cnt0_carry__1_n_3;
  wire interval_cnt0_carry__2_n_0;
  wire interval_cnt0_carry__2_n_1;
  wire interval_cnt0_carry__2_n_2;
  wire interval_cnt0_carry__2_n_3;
  wire interval_cnt0_carry__3_n_0;
  wire interval_cnt0_carry__3_n_1;
  wire interval_cnt0_carry__3_n_2;
  wire interval_cnt0_carry__3_n_3;
  wire interval_cnt0_carry__4_n_0;
  wire interval_cnt0_carry__4_n_1;
  wire interval_cnt0_carry__4_n_2;
  wire interval_cnt0_carry__4_n_3;
  wire interval_cnt0_carry__5_n_0;
  wire interval_cnt0_carry__5_n_1;
  wire interval_cnt0_carry__5_n_2;
  wire interval_cnt0_carry__5_n_3;
  wire interval_cnt0_carry__6_n_2;
  wire interval_cnt0_carry__6_n_3;
  wire interval_cnt0_carry_n_0;
  wire interval_cnt0_carry_n_1;
  wire interval_cnt0_carry_n_2;
  wire interval_cnt0_carry_n_3;
  wire \interval_cnt[31]_i_3_n_0 ;
  wire \interval_cnt[31]_i_4_n_0 ;
  wire \interval_cnt[31]_i_5_n_0 ;
  wire \interval_cnt[31]_i_6_n_0 ;
  wire \interval_cnt[31]_i_7_n_0 ;
  wire \interval_cnt[31]_i_8_n_0 ;
  wire [0:0]interval_cnt_1;
  wire \interval_cnt_reg_n_0_[0] ;
  wire \interval_cnt_reg_n_0_[10] ;
  wire \interval_cnt_reg_n_0_[11] ;
  wire \interval_cnt_reg_n_0_[12] ;
  wire \interval_cnt_reg_n_0_[13] ;
  wire \interval_cnt_reg_n_0_[14] ;
  wire \interval_cnt_reg_n_0_[15] ;
  wire \interval_cnt_reg_n_0_[16] ;
  wire \interval_cnt_reg_n_0_[17] ;
  wire \interval_cnt_reg_n_0_[18] ;
  wire \interval_cnt_reg_n_0_[19] ;
  wire \interval_cnt_reg_n_0_[1] ;
  wire \interval_cnt_reg_n_0_[20] ;
  wire \interval_cnt_reg_n_0_[21] ;
  wire \interval_cnt_reg_n_0_[22] ;
  wire \interval_cnt_reg_n_0_[23] ;
  wire \interval_cnt_reg_n_0_[24] ;
  wire \interval_cnt_reg_n_0_[25] ;
  wire \interval_cnt_reg_n_0_[26] ;
  wire \interval_cnt_reg_n_0_[27] ;
  wire \interval_cnt_reg_n_0_[28] ;
  wire \interval_cnt_reg_n_0_[29] ;
  wire \interval_cnt_reg_n_0_[2] ;
  wire \interval_cnt_reg_n_0_[30] ;
  wire \interval_cnt_reg_n_0_[31] ;
  wire \interval_cnt_reg_n_0_[3] ;
  wire \interval_cnt_reg_n_0_[4] ;
  wire \interval_cnt_reg_n_0_[5] ;
  wire \interval_cnt_reg_n_0_[6] ;
  wire \interval_cnt_reg_n_0_[7] ;
  wire \interval_cnt_reg_n_0_[8] ;
  wire \interval_cnt_reg_n_0_[9] ;
  wire next_state1;
  wire next_state13_in;
  wire next_state1_carry_n_1;
  wire next_state1_carry_n_2;
  wire next_state1_carry_n_3;
  wire \next_state1_inferred__0/i__carry_n_1 ;
  wire \next_state1_inferred__0/i__carry_n_2 ;
  wire \next_state1_inferred__0/i__carry_n_3 ;
  wire next_state20_in;
  wire next_state2_carry_i_1_n_0;
  wire next_state2_carry_i_2_n_0;
  wire next_state2_carry_i_3_n_0;
  wire next_state2_carry_i_4_n_0;
  wire next_state2_carry_i_5_n_0;
  wire next_state2_carry_i_6_n_0;
  wire next_state2_carry_i_7_n_0;
  wire next_state2_carry_i_8_n_0;
  wire next_state2_carry_n_1;
  wire next_state2_carry_n_2;
  wire next_state2_carry_n_3;
  wire [2:0]p_1_in_0;
  wire [7:0]prev_sample;
  wire red_filter_valid;
  wire [31:1]refractory_cnt;
  wire refractory_cnt0_carry__0_i_1_n_0;
  wire refractory_cnt0_carry__0_i_2_n_0;
  wire refractory_cnt0_carry__0_i_3_n_0;
  wire refractory_cnt0_carry__0_i_4_n_0;
  wire refractory_cnt0_carry__0_n_0;
  wire refractory_cnt0_carry__0_n_1;
  wire refractory_cnt0_carry__0_n_2;
  wire refractory_cnt0_carry__0_n_3;
  wire refractory_cnt0_carry__1_i_1_n_0;
  wire refractory_cnt0_carry__1_i_2_n_0;
  wire refractory_cnt0_carry__1_i_3_n_0;
  wire refractory_cnt0_carry__1_i_4_n_0;
  wire refractory_cnt0_carry__1_n_0;
  wire refractory_cnt0_carry__1_n_1;
  wire refractory_cnt0_carry__1_n_2;
  wire refractory_cnt0_carry__1_n_3;
  wire refractory_cnt0_carry__2_i_1_n_0;
  wire refractory_cnt0_carry__2_i_2_n_0;
  wire refractory_cnt0_carry__2_i_3_n_0;
  wire refractory_cnt0_carry__2_i_4_n_0;
  wire refractory_cnt0_carry__2_n_0;
  wire refractory_cnt0_carry__2_n_1;
  wire refractory_cnt0_carry__2_n_2;
  wire refractory_cnt0_carry__2_n_3;
  wire refractory_cnt0_carry__3_i_1_n_0;
  wire refractory_cnt0_carry__3_i_2_n_0;
  wire refractory_cnt0_carry__3_i_3_n_0;
  wire refractory_cnt0_carry__3_i_4_n_0;
  wire refractory_cnt0_carry__3_n_0;
  wire refractory_cnt0_carry__3_n_1;
  wire refractory_cnt0_carry__3_n_2;
  wire refractory_cnt0_carry__3_n_3;
  wire refractory_cnt0_carry__4_i_1_n_0;
  wire refractory_cnt0_carry__4_i_2_n_0;
  wire refractory_cnt0_carry__4_i_3_n_0;
  wire refractory_cnt0_carry__4_i_4_n_0;
  wire refractory_cnt0_carry__4_n_0;
  wire refractory_cnt0_carry__4_n_1;
  wire refractory_cnt0_carry__4_n_2;
  wire refractory_cnt0_carry__4_n_3;
  wire refractory_cnt0_carry__5_i_1_n_0;
  wire refractory_cnt0_carry__5_i_2_n_0;
  wire refractory_cnt0_carry__5_i_3_n_0;
  wire refractory_cnt0_carry__5_i_4_n_0;
  wire refractory_cnt0_carry__5_n_0;
  wire refractory_cnt0_carry__5_n_1;
  wire refractory_cnt0_carry__5_n_2;
  wire refractory_cnt0_carry__5_n_3;
  wire refractory_cnt0_carry__6_i_1_n_0;
  wire refractory_cnt0_carry__6_i_2_n_0;
  wire refractory_cnt0_carry__6_i_3_n_0;
  wire refractory_cnt0_carry__6_n_2;
  wire refractory_cnt0_carry__6_n_3;
  wire refractory_cnt0_carry_i_1_n_0;
  wire refractory_cnt0_carry_i_2_n_0;
  wire refractory_cnt0_carry_i_3_n_0;
  wire refractory_cnt0_carry_i_4_n_0;
  wire refractory_cnt0_carry_n_0;
  wire refractory_cnt0_carry_n_1;
  wire refractory_cnt0_carry_n_2;
  wire refractory_cnt0_carry_n_3;
  wire \refractory_cnt[0]_i_1_n_0 ;
  wire \refractory_cnt[10]_i_1_n_0 ;
  wire \refractory_cnt[11]_i_1_n_0 ;
  wire \refractory_cnt[12]_i_1_n_0 ;
  wire \refractory_cnt[13]_i_1_n_0 ;
  wire \refractory_cnt[15]_i_1_n_0 ;
  wire \refractory_cnt[17]_i_1_n_0 ;
  wire \refractory_cnt[18]_i_1_n_0 ;
  wire \refractory_cnt[19]_i_1_n_0 ;
  wire \refractory_cnt[20]_i_1_n_0 ;
  wire \refractory_cnt[21]_i_1_n_0 ;
  wire \refractory_cnt[23]_i_1_n_0 ;
  wire \refractory_cnt[5]_i_1_n_0 ;
  wire [0:0]refractory_cnt_0;
  wire \refractory_cnt_reg_n_0_[0] ;
  wire \refractory_cnt_reg_n_0_[10] ;
  wire \refractory_cnt_reg_n_0_[11] ;
  wire \refractory_cnt_reg_n_0_[12] ;
  wire \refractory_cnt_reg_n_0_[13] ;
  wire \refractory_cnt_reg_n_0_[14] ;
  wire \refractory_cnt_reg_n_0_[15] ;
  wire \refractory_cnt_reg_n_0_[16] ;
  wire \refractory_cnt_reg_n_0_[17] ;
  wire \refractory_cnt_reg_n_0_[18] ;
  wire \refractory_cnt_reg_n_0_[19] ;
  wire \refractory_cnt_reg_n_0_[1] ;
  wire \refractory_cnt_reg_n_0_[20] ;
  wire \refractory_cnt_reg_n_0_[21] ;
  wire \refractory_cnt_reg_n_0_[22] ;
  wire \refractory_cnt_reg_n_0_[23] ;
  wire \refractory_cnt_reg_n_0_[24] ;
  wire \refractory_cnt_reg_n_0_[25] ;
  wire \refractory_cnt_reg_n_0_[26] ;
  wire \refractory_cnt_reg_n_0_[27] ;
  wire \refractory_cnt_reg_n_0_[28] ;
  wire \refractory_cnt_reg_n_0_[29] ;
  wire \refractory_cnt_reg_n_0_[2] ;
  wire \refractory_cnt_reg_n_0_[30] ;
  wire \refractory_cnt_reg_n_0_[31] ;
  wire \refractory_cnt_reg_n_0_[3] ;
  wire \refractory_cnt_reg_n_0_[4] ;
  wire \refractory_cnt_reg_n_0_[5] ;
  wire \refractory_cnt_reg_n_0_[6] ;
  wire \refractory_cnt_reg_n_0_[7] ;
  wire \refractory_cnt_reg_n_0_[8] ;
  wire \refractory_cnt_reg_n_0_[9] ;
  wire s_axi_aclk;
  wire s_axi_aresetn;
  wire [2:0]sel0;
  wire write_execute;
  wire [3:2]NLW_interval_cnt0_carry__6_CO_UNCONNECTED;
  wire [3:3]NLW_interval_cnt0_carry__6_O_UNCONNECTED;
  wire [3:0]NLW_next_state1_carry_O_UNCONNECTED;
  wire [3:0]\NLW_next_state1_inferred__0/i__carry_O_UNCONNECTED ;
  wire [3:0]NLW_next_state2_carry_O_UNCONNECTED;
  wire [3:2]NLW_refractory_cnt0_carry__6_CO_UNCONNECTED;
  wire [3:3]NLW_refractory_cnt0_carry__6_O_UNCONNECTED;

  LUT6 #(
    .INIT(64'h0000FF7FFFFF0080)) 
    \FSM_sequential_current_state[0]_i_1 
       (.I0(next_state1),
        .I1(current_state[1]),
        .I2(red_filter_valid),
        .I3(\FSM_sequential_current_state[1]_i_3_n_0 ),
        .I4(\FSM_sequential_current_state[1]_i_2_n_0 ),
        .I5(current_state[0]),
        .O(\FSM_sequential_current_state[0]_i_1_n_0 ));
  LUT6 #(
    .INIT(64'h7577888877778888)) 
    \FSM_sequential_current_state[1]_i_1 
       (.I0(current_state[0]),
        .I1(\FSM_sequential_current_state[1]_i_2_n_0 ),
        .I2(\FSM_sequential_current_state[1]_i_3_n_0 ),
        .I3(red_filter_valid),
        .I4(current_state[1]),
        .I5(next_state1),
        .O(\FSM_sequential_current_state[1]_i_1_n_0 ));
  LUT4 #(
    .INIT(16'hFFFE)) 
    \FSM_sequential_current_state[1]_i_10 
       (.I0(\refractory_cnt_reg_n_0_[5] ),
        .I1(\refractory_cnt_reg_n_0_[4] ),
        .I2(\refractory_cnt_reg_n_0_[7] ),
        .I3(\refractory_cnt_reg_n_0_[6] ),
        .O(\FSM_sequential_current_state[1]_i_10_n_0 ));
  LUT4 #(
    .INIT(16'hFFFE)) 
    \FSM_sequential_current_state[1]_i_11 
       (.I0(\refractory_cnt_reg_n_0_[29] ),
        .I1(\refractory_cnt_reg_n_0_[28] ),
        .I2(\refractory_cnt_reg_n_0_[31] ),
        .I3(\refractory_cnt_reg_n_0_[30] ),
        .O(\FSM_sequential_current_state[1]_i_11_n_0 ));
  LUT4 #(
    .INIT(16'hFFFE)) 
    \FSM_sequential_current_state[1]_i_12 
       (.I0(\refractory_cnt_reg_n_0_[21] ),
        .I1(\refractory_cnt_reg_n_0_[20] ),
        .I2(\refractory_cnt_reg_n_0_[23] ),
        .I3(\refractory_cnt_reg_n_0_[22] ),
        .O(\FSM_sequential_current_state[1]_i_12_n_0 ));
  LUT6 #(
    .INIT(64'h00008080FFFFCC00)) 
    \FSM_sequential_current_state[1]_i_2 
       (.I0(next_state20_in),
        .I1(red_filter_valid),
        .I2(\FSM_sequential_current_state[1]_i_4_n_0 ),
        .I3(next_state13_in),
        .I4(current_state[1]),
        .I5(current_state[0]),
        .O(\FSM_sequential_current_state[1]_i_2_n_0 ));
  LUT4 #(
    .INIT(16'hFFFE)) 
    \FSM_sequential_current_state[1]_i_3 
       (.I0(\FSM_sequential_current_state[1]_i_5_n_0 ),
        .I1(\FSM_sequential_current_state[1]_i_6_n_0 ),
        .I2(\FSM_sequential_current_state[1]_i_7_n_0 ),
        .I3(\FSM_sequential_current_state[1]_i_8_n_0 ),
        .O(\FSM_sequential_current_state[1]_i_3_n_0 ));
  LUT2 #(
    .INIT(4'hE)) 
    \FSM_sequential_current_state[1]_i_4 
       (.I0(\fall_count_reg_n_0_[0] ),
        .I1(\fall_count_reg_n_0_[1] ),
        .O(\FSM_sequential_current_state[1]_i_4_n_0 ));
  LUT5 #(
    .INIT(32'hFFFFFFFE)) 
    \FSM_sequential_current_state[1]_i_5 
       (.I0(\refractory_cnt_reg_n_0_[10] ),
        .I1(\refractory_cnt_reg_n_0_[11] ),
        .I2(\refractory_cnt_reg_n_0_[8] ),
        .I3(\refractory_cnt_reg_n_0_[9] ),
        .I4(\FSM_sequential_current_state[1]_i_9_n_0 ),
        .O(\FSM_sequential_current_state[1]_i_5_n_0 ));
  LUT5 #(
    .INIT(32'hFFFFFFFE)) 
    \FSM_sequential_current_state[1]_i_6 
       (.I0(\refractory_cnt_reg_n_0_[2] ),
        .I1(\refractory_cnt_reg_n_0_[3] ),
        .I2(\refractory_cnt_reg_n_0_[0] ),
        .I3(\refractory_cnt_reg_n_0_[1] ),
        .I4(\FSM_sequential_current_state[1]_i_10_n_0 ),
        .O(\FSM_sequential_current_state[1]_i_6_n_0 ));
  LUT5 #(
    .INIT(32'hFFFFFFFE)) 
    \FSM_sequential_current_state[1]_i_7 
       (.I0(\refractory_cnt_reg_n_0_[26] ),
        .I1(\refractory_cnt_reg_n_0_[27] ),
        .I2(\refractory_cnt_reg_n_0_[24] ),
        .I3(\refractory_cnt_reg_n_0_[25] ),
        .I4(\FSM_sequential_current_state[1]_i_11_n_0 ),
        .O(\FSM_sequential_current_state[1]_i_7_n_0 ));
  LUT5 #(
    .INIT(32'hFFFFFFFE)) 
    \FSM_sequential_current_state[1]_i_8 
       (.I0(\refractory_cnt_reg_n_0_[18] ),
        .I1(\refractory_cnt_reg_n_0_[19] ),
        .I2(\refractory_cnt_reg_n_0_[16] ),
        .I3(\refractory_cnt_reg_n_0_[17] ),
        .I4(\FSM_sequential_current_state[1]_i_12_n_0 ),
        .O(\FSM_sequential_current_state[1]_i_8_n_0 ));
  LUT4 #(
    .INIT(16'hFFFE)) 
    \FSM_sequential_current_state[1]_i_9 
       (.I0(\refractory_cnt_reg_n_0_[13] ),
        .I1(\refractory_cnt_reg_n_0_[12] ),
        .I2(\refractory_cnt_reg_n_0_[15] ),
        .I3(\refractory_cnt_reg_n_0_[14] ),
        .O(\FSM_sequential_current_state[1]_i_9_n_0 ));
  (* FSM_ENCODED_STATES = "STATE_ARMED:00,STATE_RISING:01,STATE_PEAK_FOUND:10,STATE_REFRACTORY:11" *) 
  FDRE \FSM_sequential_current_state_reg[0] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\FSM_sequential_current_state[0]_i_1_n_0 ),
        .Q(current_state[0]),
        .R(SR));
  (* FSM_ENCODED_STATES = "STATE_ARMED:00,STATE_RISING:01,STATE_PEAK_FOUND:10,STATE_REFRACTORY:11" *) 
  FDRE \FSM_sequential_current_state_reg[1] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\FSM_sequential_current_state[1]_i_1_n_0 ),
        .Q(current_state[1]),
        .R(SR));
  LUT3 #(
    .INIT(8'h08)) 
    beat_detected_i_1
       (.I0(first_beat_seen_reg_n_0),
        .I1(current_state[1]),
        .I2(current_state[0]),
        .O(beat_detected_i_1_n_0));
  FDRE beat_detected_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(beat_detected_i_1_n_0),
        .Q(beat_detected_reg_0),
        .R(SR));
  LUT5 #(
    .INIT(32'hBFBF8F80)) 
    beat_flag_i_1
       (.I0(beat_flag_i_2_n_0),
        .I1(beat_flag_i_3_n_0),
        .I2(write_execute),
        .I3(beat_detected_reg_0),
        .I4(data3[0]),
        .O(beat_detected_reg_1));
  (* SOFT_HLUTNM = "soft_lutpair8" *) 
  LUT5 #(
    .INIT(32'hCCCC4CCC)) 
    beat_flag_i_2
       (.I0(beat_flag_reg),
        .I1(beat_detected_reg_0),
        .I2(p_1_in_0[0]),
        .I3(p_1_in_0[1]),
        .I4(p_1_in_0[2]),
        .O(beat_flag_i_2_n_0));
  (* SOFT_HLUTNM = "soft_lutpair8" *) 
  LUT5 #(
    .INIT(32'hCCCCECCC)) 
    beat_flag_i_3
       (.I0(beat_flag_reg),
        .I1(beat_detected_reg_0),
        .I2(p_1_in_0[0]),
        .I3(p_1_in_0[1]),
        .I4(p_1_in_0[2]),
        .O(beat_flag_i_3_n_0));
  LUT5 #(
    .INIT(32'hFF300080)) 
    \fall_count[0]_i_1 
       (.I0(next_state20_in),
        .I1(red_filter_valid),
        .I2(current_state[0]),
        .I3(current_state[1]),
        .I4(\fall_count_reg_n_0_[0] ),
        .O(\fall_count[0]_i_1_n_0 ));
  LUT6 #(
    .INIT(64'hFFFF2F0000008000)) 
    \fall_count[1]_i_1 
       (.I0(next_state20_in),
        .I1(\fall_count_reg_n_0_[0] ),
        .I2(red_filter_valid),
        .I3(current_state[0]),
        .I4(current_state[1]),
        .I5(\fall_count_reg_n_0_[1] ),
        .O(\fall_count[1]_i_1_n_0 ));
  FDRE \fall_count_reg[0] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\fall_count[0]_i_1_n_0 ),
        .Q(\fall_count_reg_n_0_[0] ),
        .R(SR));
  FDRE \fall_count_reg[1] 
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(\fall_count[1]_i_1_n_0 ),
        .Q(\fall_count_reg_n_0_[1] ),
        .R(SR));
  LUT3 #(
    .INIT(8'hF4)) 
    first_beat_seen_i_1
       (.I0(current_state[0]),
        .I1(current_state[1]),
        .I2(first_beat_seen_reg_n_0),
        .O(first_beat_seen_i_1_n_0));
  FDRE first_beat_seen_reg
       (.C(s_axi_aclk),
        .CE(1'b1),
        .D(first_beat_seen_i_1_n_0),
        .Q(first_beat_seen_reg_n_0),
        .R(SR));
  FDRE \ibi_cycles_reg[0] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[0] ),
        .Q(Q[0]),
        .R(SR));
  FDRE \ibi_cycles_reg[10] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[10] ),
        .Q(ibi_cycles[10]),
        .R(SR));
  FDRE \ibi_cycles_reg[11] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[11] ),
        .Q(ibi_cycles[11]),
        .R(SR));
  FDRE \ibi_cycles_reg[12] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[12] ),
        .Q(ibi_cycles[12]),
        .R(SR));
  FDRE \ibi_cycles_reg[13] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[13] ),
        .Q(ibi_cycles[13]),
        .R(SR));
  FDRE \ibi_cycles_reg[14] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[14] ),
        .Q(ibi_cycles[14]),
        .R(SR));
  FDRE \ibi_cycles_reg[15] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[15] ),
        .Q(ibi_cycles[15]),
        .R(SR));
  FDRE \ibi_cycles_reg[16] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[16] ),
        .Q(ibi_cycles[16]),
        .R(SR));
  FDRE \ibi_cycles_reg[17] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[17] ),
        .Q(ibi_cycles[17]),
        .R(SR));
  FDRE \ibi_cycles_reg[18] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[18] ),
        .Q(ibi_cycles[18]),
        .R(SR));
  FDRE \ibi_cycles_reg[19] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[19] ),
        .Q(ibi_cycles[19]),
        .R(SR));
  FDRE \ibi_cycles_reg[1] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[1] ),
        .Q(Q[1]),
        .R(SR));
  FDRE \ibi_cycles_reg[20] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[20] ),
        .Q(ibi_cycles[20]),
        .R(SR));
  FDRE \ibi_cycles_reg[21] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[21] ),
        .Q(ibi_cycles[21]),
        .R(SR));
  FDRE \ibi_cycles_reg[22] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[22] ),
        .Q(ibi_cycles[22]),
        .R(SR));
  FDRE \ibi_cycles_reg[23] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[23] ),
        .Q(ibi_cycles[23]),
        .R(SR));
  FDRE \ibi_cycles_reg[24] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[24] ),
        .Q(ibi_cycles[24]),
        .R(SR));
  FDRE \ibi_cycles_reg[25] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[25] ),
        .Q(ibi_cycles[25]),
        .R(SR));
  FDRE \ibi_cycles_reg[26] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[26] ),
        .Q(ibi_cycles[26]),
        .R(SR));
  FDRE \ibi_cycles_reg[27] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[27] ),
        .Q(ibi_cycles[27]),
        .R(SR));
  FDRE \ibi_cycles_reg[28] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[28] ),
        .Q(ibi_cycles[28]),
        .R(SR));
  FDRE \ibi_cycles_reg[29] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[29] ),
        .Q(ibi_cycles[29]),
        .R(SR));
  FDRE \ibi_cycles_reg[2] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[2] ),
        .Q(Q[2]),
        .R(SR));
  FDRE \ibi_cycles_reg[30] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[30] ),
        .Q(ibi_cycles[30]),
        .R(SR));
  FDRE \ibi_cycles_reg[31] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[31] ),
        .Q(ibi_cycles[31]),
        .R(SR));
  FDRE \ibi_cycles_reg[3] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[3] ),
        .Q(Q[3]),
        .R(SR));
  FDRE \ibi_cycles_reg[4] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[4] ),
        .Q(Q[4]),
        .R(SR));
  FDRE \ibi_cycles_reg[5] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[5] ),
        .Q(Q[5]),
        .R(SR));
  FDRE \ibi_cycles_reg[6] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[6] ),
        .Q(Q[6]),
        .R(SR));
  FDRE \ibi_cycles_reg[7] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[7] ),
        .Q(Q[7]),
        .R(SR));
  FDRE \ibi_cycles_reg[8] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[8] ),
        .Q(ibi_cycles[8]),
        .R(SR));
  FDRE \ibi_cycles_reg[9] 
       (.C(s_axi_aclk),
        .CE(beat_detected_i_1_n_0),
        .D(\interval_cnt_reg_n_0_[9] ),
        .Q(ibi_cycles[9]),
        .R(SR));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry
       (.CI(1'b0),
        .CO({interval_cnt0_carry_n_0,interval_cnt0_carry_n_1,interval_cnt0_carry_n_2,interval_cnt0_carry_n_3}),
        .CYINIT(\interval_cnt_reg_n_0_[0] ),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[4:1]),
        .S({\interval_cnt_reg_n_0_[4] ,\interval_cnt_reg_n_0_[3] ,\interval_cnt_reg_n_0_[2] ,\interval_cnt_reg_n_0_[1] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__0
       (.CI(interval_cnt0_carry_n_0),
        .CO({interval_cnt0_carry__0_n_0,interval_cnt0_carry__0_n_1,interval_cnt0_carry__0_n_2,interval_cnt0_carry__0_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[8:5]),
        .S({\interval_cnt_reg_n_0_[8] ,\interval_cnt_reg_n_0_[7] ,\interval_cnt_reg_n_0_[6] ,\interval_cnt_reg_n_0_[5] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__1
       (.CI(interval_cnt0_carry__0_n_0),
        .CO({interval_cnt0_carry__1_n_0,interval_cnt0_carry__1_n_1,interval_cnt0_carry__1_n_2,interval_cnt0_carry__1_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[12:9]),
        .S({\interval_cnt_reg_n_0_[12] ,\interval_cnt_reg_n_0_[11] ,\interval_cnt_reg_n_0_[10] ,\interval_cnt_reg_n_0_[9] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__2
       (.CI(interval_cnt0_carry__1_n_0),
        .CO({interval_cnt0_carry__2_n_0,interval_cnt0_carry__2_n_1,interval_cnt0_carry__2_n_2,interval_cnt0_carry__2_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[16:13]),
        .S({\interval_cnt_reg_n_0_[16] ,\interval_cnt_reg_n_0_[15] ,\interval_cnt_reg_n_0_[14] ,\interval_cnt_reg_n_0_[13] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__3
       (.CI(interval_cnt0_carry__2_n_0),
        .CO({interval_cnt0_carry__3_n_0,interval_cnt0_carry__3_n_1,interval_cnt0_carry__3_n_2,interval_cnt0_carry__3_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[20:17]),
        .S({\interval_cnt_reg_n_0_[20] ,\interval_cnt_reg_n_0_[19] ,\interval_cnt_reg_n_0_[18] ,\interval_cnt_reg_n_0_[17] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__4
       (.CI(interval_cnt0_carry__3_n_0),
        .CO({interval_cnt0_carry__4_n_0,interval_cnt0_carry__4_n_1,interval_cnt0_carry__4_n_2,interval_cnt0_carry__4_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[24:21]),
        .S({\interval_cnt_reg_n_0_[24] ,\interval_cnt_reg_n_0_[23] ,\interval_cnt_reg_n_0_[22] ,\interval_cnt_reg_n_0_[21] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__5
       (.CI(interval_cnt0_carry__4_n_0),
        .CO({interval_cnt0_carry__5_n_0,interval_cnt0_carry__5_n_1,interval_cnt0_carry__5_n_2,interval_cnt0_carry__5_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O(in7[28:25]),
        .S({\interval_cnt_reg_n_0_[28] ,\interval_cnt_reg_n_0_[27] ,\interval_cnt_reg_n_0_[26] ,\interval_cnt_reg_n_0_[25] }));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 interval_cnt0_carry__6
       (.CI(interval_cnt0_carry__5_n_0),
        .CO({NLW_interval_cnt0_carry__6_CO_UNCONNECTED[3:2],interval_cnt0_carry__6_n_2,interval_cnt0_carry__6_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,1'b0,1'b0}),
        .O({NLW_interval_cnt0_carry__6_O_UNCONNECTED[3],in7[31:29]}),
        .S({1'b0,\interval_cnt_reg_n_0_[31] ,\interval_cnt_reg_n_0_[30] ,\interval_cnt_reg_n_0_[29] }));
  (* SOFT_HLUTNM = "soft_lutpair9" *) 
  LUT3 #(
    .INIT(8'h0D)) 
    \interval_cnt[0]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(\interval_cnt_reg_n_0_[0] ),
        .O(interval_cnt[0]));
  (* SOFT_HLUTNM = "soft_lutpair22" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[10]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[10]),
        .O(interval_cnt[10]));
  (* SOFT_HLUTNM = "soft_lutpair23" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[11]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[11]),
        .O(interval_cnt[11]));
  (* SOFT_HLUTNM = "soft_lutpair23" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[12]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[12]),
        .O(interval_cnt[12]));
  (* SOFT_HLUTNM = "soft_lutpair24" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[13]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[13]),
        .O(interval_cnt[13]));
  (* SOFT_HLUTNM = "soft_lutpair24" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[14]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[14]),
        .O(interval_cnt[14]));
  (* SOFT_HLUTNM = "soft_lutpair25" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[15]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[15]),
        .O(interval_cnt[15]));
  (* SOFT_HLUTNM = "soft_lutpair25" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[16]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[16]),
        .O(interval_cnt[16]));
  (* SOFT_HLUTNM = "soft_lutpair26" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[17]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[17]),
        .O(interval_cnt[17]));
  (* SOFT_HLUTNM = "soft_lutpair26" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[18]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[18]),
        .O(interval_cnt[18]));
  (* SOFT_HLUTNM = "soft_lutpair27" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[19]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[19]),
        .O(interval_cnt[19]));
  (* SOFT_HLUTNM = "soft_lutpair18" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[1]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[1]),
        .O(interval_cnt[1]));
  (* SOFT_HLUTNM = "soft_lutpair27" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[20]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[20]),
        .O(interval_cnt[20]));
  (* SOFT_HLUTNM = "soft_lutpair28" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[21]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[21]),
        .O(interval_cnt[21]));
  (* SOFT_HLUTNM = "soft_lutpair28" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[22]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[22]),
        .O(interval_cnt[22]));
  (* SOFT_HLUTNM = "soft_lutpair29" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[23]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[23]),
        .O(interval_cnt[23]));
  (* SOFT_HLUTNM = "soft_lutpair29" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[24]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[24]),
        .O(interval_cnt[24]));
  (* SOFT_HLUTNM = "soft_lutpair30" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[25]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[25]),
        .O(interval_cnt[25]));
  (* SOFT_HLUTNM = "soft_lutpair30" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[26]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[26]),
        .O(interval_cnt[26]));
  (* SOFT_HLUTNM = "soft_lutpair31" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[27]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[27]),
        .O(interval_cnt[27]));
  (* SOFT_HLUTNM = "soft_lutpair31" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[28]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[28]),
        .O(interval_cnt[28]));
  (* SOFT_HLUTNM = "soft_lutpair32" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[29]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[29]),
        .O(interval_cnt[29]));
  (* SOFT_HLUTNM = "soft_lutpair18" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[2]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[2]),
        .O(interval_cnt[2]));
  (* SOFT_HLUTNM = "soft_lutpair32" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[30]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[30]),
        .O(interval_cnt[30]));
  LUT6 #(
    .INIT(64'hFFFFFFFFFFFFFFFE)) 
    \interval_cnt[31]_i_1 
       (.I0(\interval_cnt[31]_i_3_n_0 ),
        .I1(\interval_cnt[31]_i_4_n_0 ),
        .I2(\interval_cnt[31]_i_5_n_0 ),
        .I3(\interval_cnt[31]_i_6_n_0 ),
        .I4(\interval_cnt[31]_i_7_n_0 ),
        .I5(\interval_cnt[31]_i_8_n_0 ),
        .O(interval_cnt_1));
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[31]_i_2 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[31]),
        .O(interval_cnt[31]));
  LUT6 #(
    .INIT(64'h7FFFFFFFFFFFFFFF)) 
    \interval_cnt[31]_i_3 
       (.I0(\interval_cnt_reg_n_0_[12] ),
        .I1(\interval_cnt_reg_n_0_[13] ),
        .I2(\interval_cnt_reg_n_0_[10] ),
        .I3(\interval_cnt_reg_n_0_[11] ),
        .I4(\interval_cnt_reg_n_0_[9] ),
        .I5(\interval_cnt_reg_n_0_[8] ),
        .O(\interval_cnt[31]_i_3_n_0 ));
  LUT6 #(
    .INIT(64'h7FFFFFFFFFFFFFFF)) 
    \interval_cnt[31]_i_4 
       (.I0(\interval_cnt_reg_n_0_[6] ),
        .I1(\interval_cnt_reg_n_0_[7] ),
        .I2(\interval_cnt_reg_n_0_[4] ),
        .I3(\interval_cnt_reg_n_0_[5] ),
        .I4(\interval_cnt_reg_n_0_[3] ),
        .I5(\interval_cnt_reg_n_0_[2] ),
        .O(\interval_cnt[31]_i_4_n_0 ));
  LUT6 #(
    .INIT(64'h7FFFFFFFFFFFFFFF)) 
    \interval_cnt[31]_i_5 
       (.I0(\interval_cnt_reg_n_0_[24] ),
        .I1(\interval_cnt_reg_n_0_[25] ),
        .I2(\interval_cnt_reg_n_0_[22] ),
        .I3(\interval_cnt_reg_n_0_[23] ),
        .I4(\interval_cnt_reg_n_0_[21] ),
        .I5(\interval_cnt_reg_n_0_[20] ),
        .O(\interval_cnt[31]_i_5_n_0 ));
  LUT6 #(
    .INIT(64'h7FFFFFFFFFFFFFFF)) 
    \interval_cnt[31]_i_6 
       (.I0(\interval_cnt_reg_n_0_[30] ),
        .I1(\interval_cnt_reg_n_0_[31] ),
        .I2(\interval_cnt_reg_n_0_[28] ),
        .I3(\interval_cnt_reg_n_0_[29] ),
        .I4(\interval_cnt_reg_n_0_[27] ),
        .I5(\interval_cnt_reg_n_0_[26] ),
        .O(\interval_cnt[31]_i_6_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair9" *) 
  LUT4 #(
    .INIT(16'h77F7)) 
    \interval_cnt[31]_i_7 
       (.I0(\interval_cnt_reg_n_0_[1] ),
        .I1(\interval_cnt_reg_n_0_[0] ),
        .I2(current_state[1]),
        .I3(current_state[0]),
        .O(\interval_cnt[31]_i_7_n_0 ));
  LUT6 #(
    .INIT(64'h7FFFFFFFFFFFFFFF)) 
    \interval_cnt[31]_i_8 
       (.I0(\interval_cnt_reg_n_0_[18] ),
        .I1(\interval_cnt_reg_n_0_[19] ),
        .I2(\interval_cnt_reg_n_0_[16] ),
        .I3(\interval_cnt_reg_n_0_[17] ),
        .I4(\interval_cnt_reg_n_0_[15] ),
        .I5(\interval_cnt_reg_n_0_[14] ),
        .O(\interval_cnt[31]_i_8_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair19" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[3]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[3]),
        .O(interval_cnt[3]));
  (* SOFT_HLUTNM = "soft_lutpair19" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[4]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[4]),
        .O(interval_cnt[4]));
  (* SOFT_HLUTNM = "soft_lutpair20" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[5]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[5]),
        .O(interval_cnt[5]));
  (* SOFT_HLUTNM = "soft_lutpair20" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[6]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[6]),
        .O(interval_cnt[6]));
  (* SOFT_HLUTNM = "soft_lutpair21" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[7]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[7]),
        .O(interval_cnt[7]));
  (* SOFT_HLUTNM = "soft_lutpair21" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[8]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[8]),
        .O(interval_cnt[8]));
  (* SOFT_HLUTNM = "soft_lutpair22" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    \interval_cnt[9]_i_1 
       (.I0(current_state[1]),
        .I1(current_state[0]),
        .I2(in7[9]),
        .O(interval_cnt[9]));
  FDRE \interval_cnt_reg[0] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[0]),
        .Q(\interval_cnt_reg_n_0_[0] ),
        .R(SR));
  FDRE \interval_cnt_reg[10] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[10]),
        .Q(\interval_cnt_reg_n_0_[10] ),
        .R(SR));
  FDRE \interval_cnt_reg[11] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[11]),
        .Q(\interval_cnt_reg_n_0_[11] ),
        .R(SR));
  FDRE \interval_cnt_reg[12] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[12]),
        .Q(\interval_cnt_reg_n_0_[12] ),
        .R(SR));
  FDRE \interval_cnt_reg[13] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[13]),
        .Q(\interval_cnt_reg_n_0_[13] ),
        .R(SR));
  FDRE \interval_cnt_reg[14] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[14]),
        .Q(\interval_cnt_reg_n_0_[14] ),
        .R(SR));
  FDRE \interval_cnt_reg[15] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[15]),
        .Q(\interval_cnt_reg_n_0_[15] ),
        .R(SR));
  FDRE \interval_cnt_reg[16] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[16]),
        .Q(\interval_cnt_reg_n_0_[16] ),
        .R(SR));
  FDRE \interval_cnt_reg[17] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[17]),
        .Q(\interval_cnt_reg_n_0_[17] ),
        .R(SR));
  FDRE \interval_cnt_reg[18] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[18]),
        .Q(\interval_cnt_reg_n_0_[18] ),
        .R(SR));
  FDRE \interval_cnt_reg[19] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[19]),
        .Q(\interval_cnt_reg_n_0_[19] ),
        .R(SR));
  FDRE \interval_cnt_reg[1] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[1]),
        .Q(\interval_cnt_reg_n_0_[1] ),
        .R(SR));
  FDRE \interval_cnt_reg[20] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[20]),
        .Q(\interval_cnt_reg_n_0_[20] ),
        .R(SR));
  FDRE \interval_cnt_reg[21] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[21]),
        .Q(\interval_cnt_reg_n_0_[21] ),
        .R(SR));
  FDRE \interval_cnt_reg[22] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[22]),
        .Q(\interval_cnt_reg_n_0_[22] ),
        .R(SR));
  FDRE \interval_cnt_reg[23] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[23]),
        .Q(\interval_cnt_reg_n_0_[23] ),
        .R(SR));
  FDRE \interval_cnt_reg[24] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[24]),
        .Q(\interval_cnt_reg_n_0_[24] ),
        .R(SR));
  FDRE \interval_cnt_reg[25] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[25]),
        .Q(\interval_cnt_reg_n_0_[25] ),
        .R(SR));
  FDRE \interval_cnt_reg[26] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[26]),
        .Q(\interval_cnt_reg_n_0_[26] ),
        .R(SR));
  FDRE \interval_cnt_reg[27] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[27]),
        .Q(\interval_cnt_reg_n_0_[27] ),
        .R(SR));
  FDRE \interval_cnt_reg[28] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[28]),
        .Q(\interval_cnt_reg_n_0_[28] ),
        .R(SR));
  FDRE \interval_cnt_reg[29] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[29]),
        .Q(\interval_cnt_reg_n_0_[29] ),
        .R(SR));
  FDRE \interval_cnt_reg[2] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[2]),
        .Q(\interval_cnt_reg_n_0_[2] ),
        .R(SR));
  FDRE \interval_cnt_reg[30] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[30]),
        .Q(\interval_cnt_reg_n_0_[30] ),
        .R(SR));
  FDRE \interval_cnt_reg[31] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[31]),
        .Q(\interval_cnt_reg_n_0_[31] ),
        .R(SR));
  FDRE \interval_cnt_reg[3] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[3]),
        .Q(\interval_cnt_reg_n_0_[3] ),
        .R(SR));
  FDRE \interval_cnt_reg[4] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[4]),
        .Q(\interval_cnt_reg_n_0_[4] ),
        .R(SR));
  FDRE \interval_cnt_reg[5] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[5]),
        .Q(\interval_cnt_reg_n_0_[5] ),
        .R(SR));
  FDRE \interval_cnt_reg[6] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[6]),
        .Q(\interval_cnt_reg_n_0_[6] ),
        .R(SR));
  FDRE \interval_cnt_reg[7] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[7]),
        .Q(\interval_cnt_reg_n_0_[7] ),
        .R(SR));
  FDRE \interval_cnt_reg[8] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[8]),
        .Q(\interval_cnt_reg_n_0_[8] ),
        .R(SR));
  FDRE \interval_cnt_reg[9] 
       (.C(s_axi_aclk),
        .CE(interval_cnt_1),
        .D(interval_cnt[9]),
        .Q(\interval_cnt_reg_n_0_[9] ),
        .R(SR));
  (* COMPARATOR_THRESHOLD = "11" *) 
  CARRY4 next_state1_carry
       (.CI(1'b0),
        .CO({next_state13_in,next_state1_carry_n_1,next_state1_carry_n_2,next_state1_carry_n_3}),
        .CYINIT(1'b1),
        .DI(\FSM_sequential_current_state[1]_i_2_0 ),
        .O(NLW_next_state1_carry_O_UNCONNECTED[3:0]),
        .S(\FSM_sequential_current_state[1]_i_2_1 ));
  (* COMPARATOR_THRESHOLD = "11" *) 
  CARRY4 \next_state1_inferred__0/i__carry 
       (.CI(1'b0),
        .CO({next_state1,\next_state1_inferred__0/i__carry_n_1 ,\next_state1_inferred__0/i__carry_n_2 ,\next_state1_inferred__0/i__carry_n_3 }),
        .CYINIT(1'b0),
        .DI(DI),
        .O(\NLW_next_state1_inferred__0/i__carry_O_UNCONNECTED [3:0]),
        .S(S));
  (* COMPARATOR_THRESHOLD = "11" *) 
  CARRY4 next_state2_carry
       (.CI(1'b0),
        .CO({next_state20_in,next_state2_carry_n_1,next_state2_carry_n_2,next_state2_carry_n_3}),
        .CYINIT(1'b0),
        .DI({next_state2_carry_i_1_n_0,next_state2_carry_i_2_n_0,next_state2_carry_i_3_n_0,next_state2_carry_i_4_n_0}),
        .O(NLW_next_state2_carry_O_UNCONNECTED[3:0]),
        .S({next_state2_carry_i_5_n_0,next_state2_carry_i_6_n_0,next_state2_carry_i_7_n_0,next_state2_carry_i_8_n_0}));
  LUT4 #(
    .INIT(16'h22B2)) 
    next_state2_carry_i_1
       (.I0(prev_sample[7]),
        .I1(D[7]),
        .I2(prev_sample[6]),
        .I3(D[6]),
        .O(next_state2_carry_i_1_n_0));
  LUT4 #(
    .INIT(16'h22B2)) 
    next_state2_carry_i_2
       (.I0(prev_sample[5]),
        .I1(D[5]),
        .I2(prev_sample[4]),
        .I3(D[4]),
        .O(next_state2_carry_i_2_n_0));
  LUT4 #(
    .INIT(16'h22B2)) 
    next_state2_carry_i_3
       (.I0(prev_sample[3]),
        .I1(D[3]),
        .I2(prev_sample[2]),
        .I3(D[2]),
        .O(next_state2_carry_i_3_n_0));
  LUT4 #(
    .INIT(16'h22B2)) 
    next_state2_carry_i_4
       (.I0(prev_sample[1]),
        .I1(D[1]),
        .I2(prev_sample[0]),
        .I3(D[0]),
        .O(next_state2_carry_i_4_n_0));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state2_carry_i_5
       (.I0(prev_sample[7]),
        .I1(D[7]),
        .I2(prev_sample[6]),
        .I3(D[6]),
        .O(next_state2_carry_i_5_n_0));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state2_carry_i_6
       (.I0(prev_sample[5]),
        .I1(D[5]),
        .I2(prev_sample[4]),
        .I3(D[4]),
        .O(next_state2_carry_i_6_n_0));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state2_carry_i_7
       (.I0(prev_sample[3]),
        .I1(D[3]),
        .I2(prev_sample[2]),
        .I3(D[2]),
        .O(next_state2_carry_i_7_n_0));
  LUT4 #(
    .INIT(16'h9009)) 
    next_state2_carry_i_8
       (.I0(prev_sample[1]),
        .I1(D[1]),
        .I2(prev_sample[0]),
        .I3(D[0]),
        .O(next_state2_carry_i_8_n_0));
  FDRE \prev_sample_reg[0] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[0]),
        .Q(prev_sample[0]),
        .R(SR));
  FDRE \prev_sample_reg[1] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[1]),
        .Q(prev_sample[1]),
        .R(SR));
  FDRE \prev_sample_reg[2] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[2]),
        .Q(prev_sample[2]),
        .R(SR));
  FDRE \prev_sample_reg[3] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[3]),
        .Q(prev_sample[3]),
        .R(SR));
  FDRE \prev_sample_reg[4] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[4]),
        .Q(prev_sample[4]),
        .R(SR));
  FDRE \prev_sample_reg[5] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[5]),
        .Q(prev_sample[5]),
        .R(SR));
  FDRE \prev_sample_reg[6] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[6]),
        .Q(prev_sample[6]),
        .R(SR));
  FDRE \prev_sample_reg[7] 
       (.C(s_axi_aclk),
        .CE(red_filter_valid),
        .D(D[7]),
        .Q(prev_sample[7]),
        .R(SR));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry
       (.CI(1'b0),
        .CO({refractory_cnt0_carry_n_0,refractory_cnt0_carry_n_1,refractory_cnt0_carry_n_2,refractory_cnt0_carry_n_3}),
        .CYINIT(\refractory_cnt_reg_n_0_[0] ),
        .DI({\refractory_cnt_reg_n_0_[4] ,\refractory_cnt_reg_n_0_[3] ,\refractory_cnt_reg_n_0_[2] ,\refractory_cnt_reg_n_0_[1] }),
        .O(in9[4:1]),
        .S({refractory_cnt0_carry_i_1_n_0,refractory_cnt0_carry_i_2_n_0,refractory_cnt0_carry_i_3_n_0,refractory_cnt0_carry_i_4_n_0}));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__0
       (.CI(refractory_cnt0_carry_n_0),
        .CO({refractory_cnt0_carry__0_n_0,refractory_cnt0_carry__0_n_1,refractory_cnt0_carry__0_n_2,refractory_cnt0_carry__0_n_3}),
        .CYINIT(1'b0),
        .DI({\refractory_cnt_reg_n_0_[8] ,\refractory_cnt_reg_n_0_[7] ,\refractory_cnt_reg_n_0_[6] ,\refractory_cnt_reg_n_0_[5] }),
        .O(in9[8:5]),
        .S({refractory_cnt0_carry__0_i_1_n_0,refractory_cnt0_carry__0_i_2_n_0,refractory_cnt0_carry__0_i_3_n_0,refractory_cnt0_carry__0_i_4_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__0_i_1
       (.I0(\refractory_cnt_reg_n_0_[8] ),
        .O(refractory_cnt0_carry__0_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__0_i_2
       (.I0(\refractory_cnt_reg_n_0_[7] ),
        .O(refractory_cnt0_carry__0_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__0_i_3
       (.I0(\refractory_cnt_reg_n_0_[6] ),
        .O(refractory_cnt0_carry__0_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__0_i_4
       (.I0(\refractory_cnt_reg_n_0_[5] ),
        .O(refractory_cnt0_carry__0_i_4_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__1
       (.CI(refractory_cnt0_carry__0_n_0),
        .CO({refractory_cnt0_carry__1_n_0,refractory_cnt0_carry__1_n_1,refractory_cnt0_carry__1_n_2,refractory_cnt0_carry__1_n_3}),
        .CYINIT(1'b0),
        .DI({\refractory_cnt_reg_n_0_[12] ,\refractory_cnt_reg_n_0_[11] ,\refractory_cnt_reg_n_0_[10] ,\refractory_cnt_reg_n_0_[9] }),
        .O(in9[12:9]),
        .S({refractory_cnt0_carry__1_i_1_n_0,refractory_cnt0_carry__1_i_2_n_0,refractory_cnt0_carry__1_i_3_n_0,refractory_cnt0_carry__1_i_4_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__1_i_1
       (.I0(\refractory_cnt_reg_n_0_[12] ),
        .O(refractory_cnt0_carry__1_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__1_i_2
       (.I0(\refractory_cnt_reg_n_0_[11] ),
        .O(refractory_cnt0_carry__1_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__1_i_3
       (.I0(\refractory_cnt_reg_n_0_[10] ),
        .O(refractory_cnt0_carry__1_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__1_i_4
       (.I0(\refractory_cnt_reg_n_0_[9] ),
        .O(refractory_cnt0_carry__1_i_4_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__2
       (.CI(refractory_cnt0_carry__1_n_0),
        .CO({refractory_cnt0_carry__2_n_0,refractory_cnt0_carry__2_n_1,refractory_cnt0_carry__2_n_2,refractory_cnt0_carry__2_n_3}),
        .CYINIT(1'b0),
        .DI({\refractory_cnt_reg_n_0_[16] ,\refractory_cnt_reg_n_0_[15] ,\refractory_cnt_reg_n_0_[14] ,\refractory_cnt_reg_n_0_[13] }),
        .O(in9[16:13]),
        .S({refractory_cnt0_carry__2_i_1_n_0,refractory_cnt0_carry__2_i_2_n_0,refractory_cnt0_carry__2_i_3_n_0,refractory_cnt0_carry__2_i_4_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__2_i_1
       (.I0(\refractory_cnt_reg_n_0_[16] ),
        .O(refractory_cnt0_carry__2_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__2_i_2
       (.I0(\refractory_cnt_reg_n_0_[15] ),
        .O(refractory_cnt0_carry__2_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__2_i_3
       (.I0(\refractory_cnt_reg_n_0_[14] ),
        .O(refractory_cnt0_carry__2_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__2_i_4
       (.I0(\refractory_cnt_reg_n_0_[13] ),
        .O(refractory_cnt0_carry__2_i_4_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__3
       (.CI(refractory_cnt0_carry__2_n_0),
        .CO({refractory_cnt0_carry__3_n_0,refractory_cnt0_carry__3_n_1,refractory_cnt0_carry__3_n_2,refractory_cnt0_carry__3_n_3}),
        .CYINIT(1'b0),
        .DI({\refractory_cnt_reg_n_0_[20] ,\refractory_cnt_reg_n_0_[19] ,\refractory_cnt_reg_n_0_[18] ,\refractory_cnt_reg_n_0_[17] }),
        .O(in9[20:17]),
        .S({refractory_cnt0_carry__3_i_1_n_0,refractory_cnt0_carry__3_i_2_n_0,refractory_cnt0_carry__3_i_3_n_0,refractory_cnt0_carry__3_i_4_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__3_i_1
       (.I0(\refractory_cnt_reg_n_0_[20] ),
        .O(refractory_cnt0_carry__3_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__3_i_2
       (.I0(\refractory_cnt_reg_n_0_[19] ),
        .O(refractory_cnt0_carry__3_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__3_i_3
       (.I0(\refractory_cnt_reg_n_0_[18] ),
        .O(refractory_cnt0_carry__3_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__3_i_4
       (.I0(\refractory_cnt_reg_n_0_[17] ),
        .O(refractory_cnt0_carry__3_i_4_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__4
       (.CI(refractory_cnt0_carry__3_n_0),
        .CO({refractory_cnt0_carry__4_n_0,refractory_cnt0_carry__4_n_1,refractory_cnt0_carry__4_n_2,refractory_cnt0_carry__4_n_3}),
        .CYINIT(1'b0),
        .DI({\refractory_cnt_reg_n_0_[24] ,\refractory_cnt_reg_n_0_[23] ,\refractory_cnt_reg_n_0_[22] ,\refractory_cnt_reg_n_0_[21] }),
        .O(in9[24:21]),
        .S({refractory_cnt0_carry__4_i_1_n_0,refractory_cnt0_carry__4_i_2_n_0,refractory_cnt0_carry__4_i_3_n_0,refractory_cnt0_carry__4_i_4_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__4_i_1
       (.I0(\refractory_cnt_reg_n_0_[24] ),
        .O(refractory_cnt0_carry__4_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__4_i_2
       (.I0(\refractory_cnt_reg_n_0_[23] ),
        .O(refractory_cnt0_carry__4_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__4_i_3
       (.I0(\refractory_cnt_reg_n_0_[22] ),
        .O(refractory_cnt0_carry__4_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__4_i_4
       (.I0(\refractory_cnt_reg_n_0_[21] ),
        .O(refractory_cnt0_carry__4_i_4_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__5
       (.CI(refractory_cnt0_carry__4_n_0),
        .CO({refractory_cnt0_carry__5_n_0,refractory_cnt0_carry__5_n_1,refractory_cnt0_carry__5_n_2,refractory_cnt0_carry__5_n_3}),
        .CYINIT(1'b0),
        .DI({\refractory_cnt_reg_n_0_[28] ,\refractory_cnt_reg_n_0_[27] ,\refractory_cnt_reg_n_0_[26] ,\refractory_cnt_reg_n_0_[25] }),
        .O(in9[28:25]),
        .S({refractory_cnt0_carry__5_i_1_n_0,refractory_cnt0_carry__5_i_2_n_0,refractory_cnt0_carry__5_i_3_n_0,refractory_cnt0_carry__5_i_4_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__5_i_1
       (.I0(\refractory_cnt_reg_n_0_[28] ),
        .O(refractory_cnt0_carry__5_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__5_i_2
       (.I0(\refractory_cnt_reg_n_0_[27] ),
        .O(refractory_cnt0_carry__5_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__5_i_3
       (.I0(\refractory_cnt_reg_n_0_[26] ),
        .O(refractory_cnt0_carry__5_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__5_i_4
       (.I0(\refractory_cnt_reg_n_0_[25] ),
        .O(refractory_cnt0_carry__5_i_4_n_0));
  (* ADDER_THRESHOLD = "35" *) 
  CARRY4 refractory_cnt0_carry__6
       (.CI(refractory_cnt0_carry__5_n_0),
        .CO({NLW_refractory_cnt0_carry__6_CO_UNCONNECTED[3:2],refractory_cnt0_carry__6_n_2,refractory_cnt0_carry__6_n_3}),
        .CYINIT(1'b0),
        .DI({1'b0,1'b0,\refractory_cnt_reg_n_0_[30] ,\refractory_cnt_reg_n_0_[29] }),
        .O({NLW_refractory_cnt0_carry__6_O_UNCONNECTED[3],in9[31:29]}),
        .S({1'b0,refractory_cnt0_carry__6_i_1_n_0,refractory_cnt0_carry__6_i_2_n_0,refractory_cnt0_carry__6_i_3_n_0}));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__6_i_1
       (.I0(\refractory_cnt_reg_n_0_[31] ),
        .O(refractory_cnt0_carry__6_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__6_i_2
       (.I0(\refractory_cnt_reg_n_0_[30] ),
        .O(refractory_cnt0_carry__6_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry__6_i_3
       (.I0(\refractory_cnt_reg_n_0_[29] ),
        .O(refractory_cnt0_carry__6_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry_i_1
       (.I0(\refractory_cnt_reg_n_0_[4] ),
        .O(refractory_cnt0_carry_i_1_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry_i_2
       (.I0(\refractory_cnt_reg_n_0_[3] ),
        .O(refractory_cnt0_carry_i_2_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry_i_3
       (.I0(\refractory_cnt_reg_n_0_[2] ),
        .O(refractory_cnt0_carry_i_3_n_0));
  LUT1 #(
    .INIT(2'h1)) 
    refractory_cnt0_carry_i_4
       (.I0(\refractory_cnt_reg_n_0_[1] ),
        .O(refractory_cnt0_carry_i_4_n_0));
  (* SOFT_HLUTNM = "soft_lutpair39" *) 
  LUT2 #(
    .INIT(4'h2)) 
    \refractory_cnt[0]_i_1 
       (.I0(current_state[0]),
        .I1(\refractory_cnt_reg_n_0_[0] ),
        .O(\refractory_cnt[0]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair38" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[10]_i_1 
       (.I0(in9[10]),
        .I1(current_state[0]),
        .O(\refractory_cnt[10]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair37" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[11]_i_1 
       (.I0(in9[11]),
        .I1(current_state[0]),
        .O(\refractory_cnt[11]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair37" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[12]_i_1 
       (.I0(in9[12]),
        .I1(current_state[0]),
        .O(\refractory_cnt[12]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair36" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[13]_i_1 
       (.I0(in9[13]),
        .I1(current_state[0]),
        .O(\refractory_cnt[13]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair44" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[14]_i_1 
       (.I0(current_state[0]),
        .I1(in9[14]),
        .O(refractory_cnt[14]));
  (* SOFT_HLUTNM = "soft_lutpair36" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[15]_i_1 
       (.I0(in9[15]),
        .I1(current_state[0]),
        .O(\refractory_cnt[15]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair44" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[16]_i_1 
       (.I0(current_state[0]),
        .I1(in9[16]),
        .O(refractory_cnt[16]));
  (* SOFT_HLUTNM = "soft_lutpair35" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[17]_i_1 
       (.I0(in9[17]),
        .I1(current_state[0]),
        .O(\refractory_cnt[17]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair35" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[18]_i_1 
       (.I0(in9[18]),
        .I1(current_state[0]),
        .O(\refractory_cnt[18]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair34" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[19]_i_1 
       (.I0(in9[19]),
        .I1(current_state[0]),
        .O(\refractory_cnt[19]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair48" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[1]_i_1 
       (.I0(current_state[0]),
        .I1(in9[1]),
        .O(refractory_cnt[1]));
  (* SOFT_HLUTNM = "soft_lutpair34" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[20]_i_1 
       (.I0(in9[20]),
        .I1(current_state[0]),
        .O(\refractory_cnt[20]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair33" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[21]_i_1 
       (.I0(in9[21]),
        .I1(current_state[0]),
        .O(\refractory_cnt[21]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair43" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[22]_i_1 
       (.I0(current_state[0]),
        .I1(in9[22]),
        .O(refractory_cnt[22]));
  (* SOFT_HLUTNM = "soft_lutpair33" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[23]_i_1 
       (.I0(in9[23]),
        .I1(current_state[0]),
        .O(\refractory_cnt[23]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair43" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[24]_i_1 
       (.I0(current_state[0]),
        .I1(in9[24]),
        .O(refractory_cnt[24]));
  (* SOFT_HLUTNM = "soft_lutpair42" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[25]_i_1 
       (.I0(current_state[0]),
        .I1(in9[25]),
        .O(refractory_cnt[25]));
  (* SOFT_HLUTNM = "soft_lutpair42" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[26]_i_1 
       (.I0(current_state[0]),
        .I1(in9[26]),
        .O(refractory_cnt[26]));
  (* SOFT_HLUTNM = "soft_lutpair41" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[27]_i_1 
       (.I0(current_state[0]),
        .I1(in9[27]),
        .O(refractory_cnt[27]));
  (* SOFT_HLUTNM = "soft_lutpair41" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[28]_i_1 
       (.I0(current_state[0]),
        .I1(in9[28]),
        .O(refractory_cnt[28]));
  (* SOFT_HLUTNM = "soft_lutpair40" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[29]_i_1 
       (.I0(current_state[0]),
        .I1(in9[29]),
        .O(refractory_cnt[29]));
  (* SOFT_HLUTNM = "soft_lutpair48" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[2]_i_1 
       (.I0(current_state[0]),
        .I1(in9[2]),
        .O(refractory_cnt[2]));
  (* SOFT_HLUTNM = "soft_lutpair40" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[30]_i_1 
       (.I0(current_state[0]),
        .I1(in9[30]),
        .O(refractory_cnt[30]));
  LUT3 #(
    .INIT(8'h8C)) 
    \refractory_cnt[31]_i_1 
       (.I0(\FSM_sequential_current_state[1]_i_3_n_0 ),
        .I1(current_state[1]),
        .I2(current_state[0]),
        .O(refractory_cnt_0));
  (* SOFT_HLUTNM = "soft_lutpair39" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[31]_i_2 
       (.I0(current_state[0]),
        .I1(in9[31]),
        .O(refractory_cnt[31]));
  (* SOFT_HLUTNM = "soft_lutpair47" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[3]_i_1 
       (.I0(current_state[0]),
        .I1(in9[3]),
        .O(refractory_cnt[3]));
  (* SOFT_HLUTNM = "soft_lutpair47" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[4]_i_1 
       (.I0(current_state[0]),
        .I1(in9[4]),
        .O(refractory_cnt[4]));
  (* SOFT_HLUTNM = "soft_lutpair38" *) 
  LUT2 #(
    .INIT(4'hB)) 
    \refractory_cnt[5]_i_1 
       (.I0(in9[5]),
        .I1(current_state[0]),
        .O(\refractory_cnt[5]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair46" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[6]_i_1 
       (.I0(current_state[0]),
        .I1(in9[6]),
        .O(refractory_cnt[6]));
  (* SOFT_HLUTNM = "soft_lutpair46" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[7]_i_1 
       (.I0(current_state[0]),
        .I1(in9[7]),
        .O(refractory_cnt[7]));
  (* SOFT_HLUTNM = "soft_lutpair45" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[8]_i_1 
       (.I0(current_state[0]),
        .I1(in9[8]),
        .O(refractory_cnt[8]));
  (* SOFT_HLUTNM = "soft_lutpair45" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \refractory_cnt[9]_i_1 
       (.I0(current_state[0]),
        .I1(in9[9]),
        .O(refractory_cnt[9]));
  FDRE \refractory_cnt_reg[0] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[0]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[0] ),
        .R(SR));
  FDRE \refractory_cnt_reg[10] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[10]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[10] ),
        .R(SR));
  FDRE \refractory_cnt_reg[11] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[11]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[11] ),
        .R(SR));
  FDRE \refractory_cnt_reg[12] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[12]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[12] ),
        .R(SR));
  FDRE \refractory_cnt_reg[13] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[13]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[13] ),
        .R(SR));
  FDRE \refractory_cnt_reg[14] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[14]),
        .Q(\refractory_cnt_reg_n_0_[14] ),
        .R(SR));
  FDRE \refractory_cnt_reg[15] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[15]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[15] ),
        .R(SR));
  FDRE \refractory_cnt_reg[16] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[16]),
        .Q(\refractory_cnt_reg_n_0_[16] ),
        .R(SR));
  FDRE \refractory_cnt_reg[17] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[17]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[17] ),
        .R(SR));
  FDRE \refractory_cnt_reg[18] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[18]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[18] ),
        .R(SR));
  FDRE \refractory_cnt_reg[19] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[19]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[19] ),
        .R(SR));
  FDRE \refractory_cnt_reg[1] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[1]),
        .Q(\refractory_cnt_reg_n_0_[1] ),
        .R(SR));
  FDRE \refractory_cnt_reg[20] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[20]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[20] ),
        .R(SR));
  FDRE \refractory_cnt_reg[21] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[21]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[21] ),
        .R(SR));
  FDRE \refractory_cnt_reg[22] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[22]),
        .Q(\refractory_cnt_reg_n_0_[22] ),
        .R(SR));
  FDRE \refractory_cnt_reg[23] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[23]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[23] ),
        .R(SR));
  FDRE \refractory_cnt_reg[24] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[24]),
        .Q(\refractory_cnt_reg_n_0_[24] ),
        .R(SR));
  FDRE \refractory_cnt_reg[25] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[25]),
        .Q(\refractory_cnt_reg_n_0_[25] ),
        .R(SR));
  FDRE \refractory_cnt_reg[26] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[26]),
        .Q(\refractory_cnt_reg_n_0_[26] ),
        .R(SR));
  FDRE \refractory_cnt_reg[27] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[27]),
        .Q(\refractory_cnt_reg_n_0_[27] ),
        .R(SR));
  FDRE \refractory_cnt_reg[28] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[28]),
        .Q(\refractory_cnt_reg_n_0_[28] ),
        .R(SR));
  FDRE \refractory_cnt_reg[29] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[29]),
        .Q(\refractory_cnt_reg_n_0_[29] ),
        .R(SR));
  FDRE \refractory_cnt_reg[2] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[2]),
        .Q(\refractory_cnt_reg_n_0_[2] ),
        .R(SR));
  FDRE \refractory_cnt_reg[30] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[30]),
        .Q(\refractory_cnt_reg_n_0_[30] ),
        .R(SR));
  FDRE \refractory_cnt_reg[31] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[31]),
        .Q(\refractory_cnt_reg_n_0_[31] ),
        .R(SR));
  FDRE \refractory_cnt_reg[3] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[3]),
        .Q(\refractory_cnt_reg_n_0_[3] ),
        .R(SR));
  FDRE \refractory_cnt_reg[4] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[4]),
        .Q(\refractory_cnt_reg_n_0_[4] ),
        .R(SR));
  FDRE \refractory_cnt_reg[5] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(\refractory_cnt[5]_i_1_n_0 ),
        .Q(\refractory_cnt_reg_n_0_[5] ),
        .R(SR));
  FDRE \refractory_cnt_reg[6] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[6]),
        .Q(\refractory_cnt_reg_n_0_[6] ),
        .R(SR));
  FDRE \refractory_cnt_reg[7] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[7]),
        .Q(\refractory_cnt_reg_n_0_[7] ),
        .R(SR));
  FDRE \refractory_cnt_reg[8] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[8]),
        .Q(\refractory_cnt_reg_n_0_[8] ),
        .R(SR));
  FDRE \refractory_cnt_reg[9] 
       (.C(s_axi_aclk),
        .CE(refractory_cnt_0),
        .D(refractory_cnt[9]),
        .Q(\refractory_cnt_reg_n_0_[9] ),
        .R(SR));
  LUT1 #(
    .INIT(2'h1)) 
    s_axi_awready_i_1
       (.I0(s_axi_aresetn),
        .O(SR));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[10]_i_1 
       (.I0(ibi_cycles[10]),
        .I1(data3[3]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [2]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[11]_i_1 
       (.I0(ibi_cycles[11]),
        .I1(data3[4]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [3]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[12]_i_1 
       (.I0(ibi_cycles[12]),
        .I1(data3[5]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [4]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[13]_i_1 
       (.I0(ibi_cycles[13]),
        .I1(data3[6]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [5]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[14]_i_1 
       (.I0(ibi_cycles[14]),
        .I1(data3[7]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [6]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[15]_i_1 
       (.I0(ibi_cycles[15]),
        .I1(data3[8]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [7]));
  (* SOFT_HLUTNM = "soft_lutpair10" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[16]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[16]),
        .O(\axi_araddr_latched_reg[2] [8]));
  (* SOFT_HLUTNM = "soft_lutpair10" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[17]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[17]),
        .O(\axi_araddr_latched_reg[2] [9]));
  (* SOFT_HLUTNM = "soft_lutpair11" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[18]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[18]),
        .O(\axi_araddr_latched_reg[2] [10]));
  (* SOFT_HLUTNM = "soft_lutpair11" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[19]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[19]),
        .O(\axi_araddr_latched_reg[2] [11]));
  (* SOFT_HLUTNM = "soft_lutpair12" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[20]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[20]),
        .O(\axi_araddr_latched_reg[2] [12]));
  (* SOFT_HLUTNM = "soft_lutpair12" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[21]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[21]),
        .O(\axi_araddr_latched_reg[2] [13]));
  (* SOFT_HLUTNM = "soft_lutpair13" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[22]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[22]),
        .O(\axi_araddr_latched_reg[2] [14]));
  (* SOFT_HLUTNM = "soft_lutpair13" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[23]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[23]),
        .O(\axi_araddr_latched_reg[2] [15]));
  (* SOFT_HLUTNM = "soft_lutpair14" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[24]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[24]),
        .O(\axi_araddr_latched_reg[2] [16]));
  (* SOFT_HLUTNM = "soft_lutpair14" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[25]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[25]),
        .O(\axi_araddr_latched_reg[2] [17]));
  (* SOFT_HLUTNM = "soft_lutpair15" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[26]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[26]),
        .O(\axi_araddr_latched_reg[2] [18]));
  (* SOFT_HLUTNM = "soft_lutpair15" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[27]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[27]),
        .O(\axi_araddr_latched_reg[2] [19]));
  (* SOFT_HLUTNM = "soft_lutpair16" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[28]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[28]),
        .O(\axi_araddr_latched_reg[2] [20]));
  (* SOFT_HLUTNM = "soft_lutpair16" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[29]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[29]),
        .O(\axi_araddr_latched_reg[2] [21]));
  (* SOFT_HLUTNM = "soft_lutpair17" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[30]_i_1 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[30]),
        .O(\axi_araddr_latched_reg[2] [22]));
  (* SOFT_HLUTNM = "soft_lutpair17" *) 
  LUT4 #(
    .INIT(16'h0400)) 
    \s_axi_rdata[31]_i_2 
       (.I0(sel0[0]),
        .I1(sel0[1]),
        .I2(sel0[2]),
        .I3(ibi_cycles[31]),
        .O(\axi_araddr_latched_reg[2] [23]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[8]_i_1 
       (.I0(ibi_cycles[8]),
        .I1(data3[1]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [0]));
  LUT5 #(
    .INIT(32'h0C000A00)) 
    \s_axi_rdata[9]_i_1 
       (.I0(ibi_cycles[9]),
        .I1(data3[2]),
        .I2(sel0[2]),
        .I3(sel0[1]),
        .I4(sel0[0]),
        .O(\axi_araddr_latched_reg[2] [1]));
endmodule
`ifndef GLBL
`define GLBL
`timescale  1 ps / 1 ps

module glbl ();

    parameter ROC_WIDTH = 100000;
    parameter TOC_WIDTH = 0;
    parameter GRES_WIDTH = 10000;
    parameter GRES_START = 10000;

//--------   STARTUP Globals --------------
    wire GSR;
    wire GTS;
    wire GWE;
    wire PRLD;
    wire GRESTORE;
    tri1 p_up_tmp;
    tri (weak1, strong0) PLL_LOCKG = p_up_tmp;

    wire PROGB_GLBL;
    wire CCLKO_GLBL;
    wire FCSBO_GLBL;
    wire [3:0] DO_GLBL;
    wire [3:0] DI_GLBL;
   
    reg GSR_int;
    reg GTS_int;
    reg PRLD_int;
    reg GRESTORE_int;

//--------   JTAG Globals --------------
    wire JTAG_TDO_GLBL;
    wire JTAG_TCK_GLBL;
    wire JTAG_TDI_GLBL;
    wire JTAG_TMS_GLBL;
    wire JTAG_TRST_GLBL;

    reg JTAG_CAPTURE_GLBL;
    reg JTAG_RESET_GLBL;
    reg JTAG_SHIFT_GLBL;
    reg JTAG_UPDATE_GLBL;
    reg JTAG_RUNTEST_GLBL;

    reg JTAG_SEL1_GLBL = 0;
    reg JTAG_SEL2_GLBL = 0 ;
    reg JTAG_SEL3_GLBL = 0;
    reg JTAG_SEL4_GLBL = 0;

    reg JTAG_USER_TDO1_GLBL = 1'bz;
    reg JTAG_USER_TDO2_GLBL = 1'bz;
    reg JTAG_USER_TDO3_GLBL = 1'bz;
    reg JTAG_USER_TDO4_GLBL = 1'bz;

    assign (strong1, weak0) GSR = GSR_int;
    assign (strong1, weak0) GTS = GTS_int;
    assign (weak1, weak0) PRLD = PRLD_int;
    assign (strong1, weak0) GRESTORE = GRESTORE_int;

    initial begin
	GSR_int = 1'b1;
	PRLD_int = 1'b1;
	#(ROC_WIDTH)
	GSR_int = 1'b0;
	PRLD_int = 1'b0;
    end

    initial begin
	GTS_int = 1'b1;
	#(TOC_WIDTH)
	GTS_int = 1'b0;
    end

    initial begin 
	GRESTORE_int = 1'b0;
	#(GRES_START);
	GRESTORE_int = 1'b1;
	#(GRES_WIDTH);
	GRESTORE_int = 1'b0;
    end

endmodule
`endif
