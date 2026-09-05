-- Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
-- Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2026.1 (win64) Build 6511674 Tue Jun 16 11:02:23 MDT 2026
-- Date        : Fri Sep  4 13:54:12 2026
-- Host        : ABHINAV running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode funcsim {c:/Users/abhin/OneDrive/Desktop/verilog/FPGA MEDTECH DEVICE
--               ZYNQ-7000/FPGA MEDTECH DEVICE
--               ZYNQ-7000.gen/sources_1/bd/design_ZYNQ/ip/design_ZYNQ_axi_ppg_accelerator_0_0/design_ZYNQ_axi_ppg_accelerator_0_0_sim_netlist.vhdl}
-- Design      : design_ZYNQ_axi_ppg_accelerator_0_0
-- Purpose     : This VHDL netlist is a functional simulation representation of the design and should not be modified or
--               synthesized. This netlist cannot be used for SDF annotated simulation.
-- Device      : xc7z020clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap is
  port (
    \data_out_reg[7]_0\ : out STD_LOGIC;
    \data_out_reg[1]_0\ : out STD_LOGIC;
    \data_out_reg[2]_0\ : out STD_LOGIC;
    \data_out_reg[3]_0\ : out STD_LOGIC;
    \data_out_reg[4]_0\ : out STD_LOGIC;
    \data_out_reg[5]_0\ : out STD_LOGIC;
    \data_out_reg[6]_0\ : out STD_LOGIC;
    D : out STD_LOGIC_VECTOR ( 0 to 0 );
    \running_sum_reg[0]_0\ : in STD_LOGIC;
    ir_sample_valid : in STD_LOGIC;
    s_axi_aclk : in STD_LOGIC;
    Q : in STD_LOGIC_VECTOR ( 7 downto 0 );
    \s_axi_rdata_reg[0]\ : in STD_LOGIC;
    \s_axi_rdata_reg[0]_0\ : in STD_LOGIC_VECTOR ( 0 to 0 );
    sel0 : in STD_LOGIC_VECTOR ( 2 downto 0 )
  );
  attribute ORIG_REF_NAME : string;
  attribute ORIG_REF_NAME of design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap : entity is "moving_average_8tap";
end design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap;

architecture STRUCTURE of design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap is
  signal \^data_out_reg[1]_0\ : STD_LOGIC;
  signal \^data_out_reg[2]_0\ : STD_LOGIC;
  signal \^data_out_reg[3]_0\ : STD_LOGIC;
  signal \^data_out_reg[4]_0\ : STD_LOGIC;
  signal \^data_out_reg[5]_0\ : STD_LOGIC;
  signal \^data_out_reg[6]_0\ : STD_LOGIC;
  signal \^data_out_reg[7]_0\ : STD_LOGIC;
  signal \data_out_reg_n_0_[0]\ : STD_LOGIC;
  signal \next_sum_carry__0_i_1__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_2__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_3__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_4__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_5__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_6__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_7__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_8__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_n_1\ : STD_LOGIC;
  signal \next_sum_carry__0_n_2\ : STD_LOGIC;
  signal \next_sum_carry__0_n_3\ : STD_LOGIC;
  signal \next_sum_carry__0_n_4\ : STD_LOGIC;
  signal \next_sum_carry__0_n_5\ : STD_LOGIC;
  signal \next_sum_carry__0_n_6\ : STD_LOGIC;
  signal \next_sum_carry__0_n_7\ : STD_LOGIC;
  signal \next_sum_carry__1_i_1__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_i_2__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_i_3__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_i_4__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_n_2\ : STD_LOGIC;
  signal \next_sum_carry__1_n_3\ : STD_LOGIC;
  signal \next_sum_carry__1_n_5\ : STD_LOGIC;
  signal \next_sum_carry__1_n_6\ : STD_LOGIC;
  signal \next_sum_carry__1_n_7\ : STD_LOGIC;
  signal \next_sum_carry_i_1__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry_i_2__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry_i_3__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry_i_4__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry_i_5__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry_i_6__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry_i_7__0_n_0\ : STD_LOGIC;
  signal next_sum_carry_n_0 : STD_LOGIC;
  signal next_sum_carry_n_1 : STD_LOGIC;
  signal next_sum_carry_n_2 : STD_LOGIC;
  signal next_sum_carry_n_3 : STD_LOGIC;
  signal next_sum_carry_n_4 : STD_LOGIC;
  signal next_sum_carry_n_5 : STD_LOGIC;
  signal next_sum_carry_n_6 : STD_LOGIC;
  signal next_sum_carry_n_7 : STD_LOGIC;
  signal \running_sum_reg_n_0_[0]\ : STD_LOGIC;
  signal \running_sum_reg_n_0_[1]\ : STD_LOGIC;
  signal \running_sum_reg_n_0_[2]\ : STD_LOGIC;
  signal \s_axi_rdata[0]_i_2_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__0_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__1_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__2_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__3_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__6_n_0\ : STD_LOGIC;
  signal shift_reg_reg_gate_n_0 : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][0]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][1]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][2]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][3]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][4]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][5]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][6]\ : STD_LOGIC;
  signal \shift_reg_reg_n_0_[7][7]\ : STD_LOGIC;
  signal shift_reg_reg_r_10_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_11_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_6_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_7_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_8_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_9_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_n_0 : STD_LOGIC;
  signal \NLW_next_sum_carry__1_CO_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 downto 2 );
  signal \NLW_next_sum_carry__1_O_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 to 3 );
  attribute ADDER_THRESHOLD : integer;
  attribute ADDER_THRESHOLD of next_sum_carry : label is 35;
  attribute ADDER_THRESHOLD of \next_sum_carry__0\ : label is 35;
  attribute HLUTNM : string;
  attribute HLUTNM of \next_sum_carry__0_i_1__0\ : label is "lutpair11";
  attribute HLUTNM of \next_sum_carry__0_i_2__0\ : label is "lutpair10";
  attribute HLUTNM of \next_sum_carry__0_i_3__0\ : label is "lutpair9";
  attribute HLUTNM of \next_sum_carry__0_i_4__0\ : label is "lutpair8";
  attribute HLUTNM of \next_sum_carry__0_i_6__0\ : label is "lutpair11";
  attribute HLUTNM of \next_sum_carry__0_i_7__0\ : label is "lutpair10";
  attribute HLUTNM of \next_sum_carry__0_i_8__0\ : label is "lutpair9";
  attribute ADDER_THRESHOLD of \next_sum_carry__1\ : label is 35;
  attribute HLUTNM of \next_sum_carry_i_1__0\ : label is "lutpair7";
  attribute HLUTNM of \next_sum_carry_i_2__0\ : label is "lutpair6";
  attribute HLUTNM of \next_sum_carry_i_3__0\ : label is "lutpair13";
  attribute HLUTNM of \next_sum_carry_i_4__0\ : label is "lutpair8";
  attribute HLUTNM of \next_sum_carry_i_5__0\ : label is "lutpair7";
  attribute HLUTNM of \next_sum_carry_i_6__0\ : label is "lutpair6";
  attribute HLUTNM of \next_sum_carry_i_7__0\ : label is "lutpair13";
  attribute srl_bus_name : string;
  attribute srl_bus_name of \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name : string;
  attribute srl_name of \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute srl_bus_name of \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\ : label is "\inst/u_filter_ir/shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10 ";
  attribute SOFT_HLUTNM : string;
  attribute SOFT_HLUTNM of shift_reg_reg_gate : label is "soft_lutpair0";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__0\ : label is "soft_lutpair0";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__1\ : label is "soft_lutpair1";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__2\ : label is "soft_lutpair1";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__3\ : label is "soft_lutpair2";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__4\ : label is "soft_lutpair2";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__5\ : label is "soft_lutpair3";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__6\ : label is "soft_lutpair3";
begin
  \data_out_reg[1]_0\ <= \^data_out_reg[1]_0\;
  \data_out_reg[2]_0\ <= \^data_out_reg[2]_0\;
  \data_out_reg[3]_0\ <= \^data_out_reg[3]_0\;
  \data_out_reg[4]_0\ <= \^data_out_reg[4]_0\;
  \data_out_reg[5]_0\ <= \^data_out_reg[5]_0\;
  \data_out_reg[6]_0\ <= \^data_out_reg[6]_0\;
  \data_out_reg[7]_0\ <= \^data_out_reg[7]_0\;
\data_out_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => next_sum_carry_n_4,
      Q => \data_out_reg_n_0_[0]\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__0_n_7\,
      Q => \^data_out_reg[1]_0\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__0_n_6\,
      Q => \^data_out_reg[2]_0\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__0_n_5\,
      Q => \^data_out_reg[3]_0\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__0_n_4\,
      Q => \^data_out_reg[4]_0\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__1_n_7\,
      Q => \^data_out_reg[5]_0\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__1_n_6\,
      Q => \^data_out_reg[6]_0\,
      R => \running_sum_reg[0]_0\
    );
\data_out_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \next_sum_carry__1_n_5\,
      Q => \^data_out_reg[7]_0\,
      R => \running_sum_reg[0]_0\
    );
next_sum_carry: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => next_sum_carry_n_0,
      CO(2) => next_sum_carry_n_1,
      CO(1) => next_sum_carry_n_2,
      CO(0) => next_sum_carry_n_3,
      CYINIT => '0',
      DI(3) => \next_sum_carry_i_1__0_n_0\,
      DI(2) => \next_sum_carry_i_2__0_n_0\,
      DI(1) => \next_sum_carry_i_3__0_n_0\,
      DI(0) => \running_sum_reg_n_0_[0]\,
      O(3) => next_sum_carry_n_4,
      O(2) => next_sum_carry_n_5,
      O(1) => next_sum_carry_n_6,
      O(0) => next_sum_carry_n_7,
      S(3) => \next_sum_carry_i_4__0_n_0\,
      S(2) => \next_sum_carry_i_5__0_n_0\,
      S(1) => \next_sum_carry_i_6__0_n_0\,
      S(0) => \next_sum_carry_i_7__0_n_0\
    );
\next_sum_carry__0\: unisim.vcomponents.CARRY4
     port map (
      CI => next_sum_carry_n_0,
      CO(3) => \next_sum_carry__0_n_0\,
      CO(2) => \next_sum_carry__0_n_1\,
      CO(1) => \next_sum_carry__0_n_2\,
      CO(0) => \next_sum_carry__0_n_3\,
      CYINIT => '0',
      DI(3) => \next_sum_carry__0_i_1__0_n_0\,
      DI(2) => \next_sum_carry__0_i_2__0_n_0\,
      DI(1) => \next_sum_carry__0_i_3__0_n_0\,
      DI(0) => \next_sum_carry__0_i_4__0_n_0\,
      O(3) => \next_sum_carry__0_n_4\,
      O(2) => \next_sum_carry__0_n_5\,
      O(1) => \next_sum_carry__0_n_6\,
      O(0) => \next_sum_carry__0_n_7\,
      S(3) => \next_sum_carry__0_i_5__0_n_0\,
      S(2) => \next_sum_carry__0_i_6__0_n_0\,
      S(1) => \next_sum_carry__0_i_7__0_n_0\,
      S(0) => \next_sum_carry__0_i_8__0_n_0\
    );
\next_sum_carry__0_i_1__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][6]\,
      I1 => Q(6),
      I2 => \^data_out_reg[3]_0\,
      O => \next_sum_carry__0_i_1__0_n_0\
    );
\next_sum_carry__0_i_2__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][5]\,
      I1 => Q(5),
      I2 => \^data_out_reg[2]_0\,
      O => \next_sum_carry__0_i_2__0_n_0\
    );
\next_sum_carry__0_i_3__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][4]\,
      I1 => Q(4),
      I2 => \^data_out_reg[1]_0\,
      O => \next_sum_carry__0_i_3__0_n_0\
    );
\next_sum_carry__0_i_4__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][3]\,
      I1 => Q(3),
      I2 => \data_out_reg_n_0_[0]\,
      O => \next_sum_carry__0_i_4__0_n_0\
    );
\next_sum_carry__0_i_5__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \next_sum_carry__0_i_1__0_n_0\,
      I1 => \shift_reg_reg_n_0_[7][7]\,
      I2 => Q(7),
      I3 => \^data_out_reg[4]_0\,
      O => \next_sum_carry__0_i_5__0_n_0\
    );
\next_sum_carry__0_i_6__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][6]\,
      I1 => Q(6),
      I2 => \^data_out_reg[3]_0\,
      I3 => \next_sum_carry__0_i_2__0_n_0\,
      O => \next_sum_carry__0_i_6__0_n_0\
    );
\next_sum_carry__0_i_7__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][5]\,
      I1 => Q(5),
      I2 => \^data_out_reg[2]_0\,
      I3 => \next_sum_carry__0_i_3__0_n_0\,
      O => \next_sum_carry__0_i_7__0_n_0\
    );
\next_sum_carry__0_i_8__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][4]\,
      I1 => Q(4),
      I2 => \^data_out_reg[1]_0\,
      I3 => \next_sum_carry__0_i_4__0_n_0\,
      O => \next_sum_carry__0_i_8__0_n_0\
    );
\next_sum_carry__1\: unisim.vcomponents.CARRY4
     port map (
      CI => \next_sum_carry__0_n_0\,
      CO(3 downto 2) => \NLW_next_sum_carry__1_CO_UNCONNECTED\(3 downto 2),
      CO(1) => \next_sum_carry__1_n_2\,
      CO(0) => \next_sum_carry__1_n_3\,
      CYINIT => '0',
      DI(3 downto 2) => B"00",
      DI(1) => \^data_out_reg[5]_0\,
      DI(0) => \next_sum_carry__1_i_1__0_n_0\,
      O(3) => \NLW_next_sum_carry__1_O_UNCONNECTED\(3),
      O(2) => \next_sum_carry__1_n_5\,
      O(1) => \next_sum_carry__1_n_6\,
      O(0) => \next_sum_carry__1_n_7\,
      S(3) => '0',
      S(2) => \next_sum_carry__1_i_2__0_n_0\,
      S(1) => \next_sum_carry__1_i_3__0_n_0\,
      S(0) => \next_sum_carry__1_i_4__0_n_0\
    );
\next_sum_carry__1_i_1__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][7]\,
      I1 => Q(7),
      I2 => \^data_out_reg[4]_0\,
      O => \next_sum_carry__1_i_1__0_n_0\
    );
\next_sum_carry__1_i_2__0\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"9"
    )
        port map (
      I0 => \^data_out_reg[6]_0\,
      I1 => \^data_out_reg[7]_0\,
      O => \next_sum_carry__1_i_2__0_n_0\
    );
\next_sum_carry__1_i_3__0\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"9"
    )
        port map (
      I0 => \^data_out_reg[5]_0\,
      I1 => \^data_out_reg[6]_0\,
      O => \next_sum_carry__1_i_3__0_n_0\
    );
\next_sum_carry__1_i_4__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"8E71"
    )
        port map (
      I0 => \^data_out_reg[4]_0\,
      I1 => Q(7),
      I2 => \shift_reg_reg_n_0_[7][7]\,
      I3 => \^data_out_reg[5]_0\,
      O => \next_sum_carry__1_i_4__0_n_0\
    );
\next_sum_carry_i_1__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][2]\,
      I1 => \running_sum_reg_n_0_[2]\,
      I2 => Q(2),
      O => \next_sum_carry_i_1__0_n_0\
    );
\next_sum_carry_i_2__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D4"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][1]\,
      I1 => \running_sum_reg_n_0_[1]\,
      I2 => Q(1),
      O => \next_sum_carry_i_2__0_n_0\
    );
\next_sum_carry_i_3__0\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => Q(0),
      I1 => \shift_reg_reg_n_0_[7][0]\,
      O => \next_sum_carry_i_3__0_n_0\
    );
\next_sum_carry_i_4__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][3]\,
      I1 => Q(3),
      I2 => \data_out_reg_n_0_[0]\,
      I3 => \next_sum_carry_i_1__0_n_0\,
      O => \next_sum_carry_i_4__0_n_0\
    );
\next_sum_carry_i_5__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][2]\,
      I1 => \running_sum_reg_n_0_[2]\,
      I2 => Q(2),
      I3 => \next_sum_carry_i_2__0_n_0\,
      O => \next_sum_carry_i_5__0_n_0\
    );
\next_sum_carry_i_6__0\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \shift_reg_reg_n_0_[7][1]\,
      I1 => \running_sum_reg_n_0_[1]\,
      I2 => Q(1),
      I3 => \next_sum_carry_i_3__0_n_0\,
      O => \next_sum_carry_i_6__0_n_0\
    );
\next_sum_carry_i_7__0\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"96"
    )
        port map (
      I0 => Q(0),
      I1 => \shift_reg_reg_n_0_[7][0]\,
      I2 => \running_sum_reg_n_0_[0]\,
      O => \next_sum_carry_i_7__0_n_0\
    );
\running_sum_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => next_sum_carry_n_7,
      Q => \running_sum_reg_n_0_[0]\,
      R => \running_sum_reg[0]_0\
    );
\running_sum_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => next_sum_carry_n_6,
      Q => \running_sum_reg_n_0_[1]\,
      R => \running_sum_reg[0]_0\
    );
\running_sum_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => next_sum_carry_n_5,
      Q => \running_sum_reg_n_0_[2]\,
      R => \running_sum_reg[0]_0\
    );
\s_axi_rdata[0]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"E"
    )
        port map (
      I0 => \s_axi_rdata[0]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[0]\,
      O => D(0)
    );
\s_axi_rdata[0]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"0000CC0000F0AA00"
    )
        port map (
      I0 => Q(0),
      I1 => \data_out_reg_n_0_[0]\,
      I2 => \s_axi_rdata_reg[0]_0\(0),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => \s_axi_rdata[0]_i_2_n_0\
    );
\shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(0),
      Q => \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(1),
      Q => \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(2),
      Q => \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(3),
      Q => \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(4),
      Q => \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(5),
      Q => \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(6),
      Q => \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => ir_sample_valid,
      CLK => s_axi_aclk,
      D => Q(7),
      Q => \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\
    );
\shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][0]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][1]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][2]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][3]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][4]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][5]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][6]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg[5][7]_srl6___inst_u_filter_ir_shift_reg_reg_r_10_n_0\,
      Q => \shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      R => '0'
    );
\shift_reg_reg[7][0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__6_n_0\,
      Q => \shift_reg_reg_n_0_[7][0]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__5_n_0\,
      Q => \shift_reg_reg_n_0_[7][1]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__4_n_0\,
      Q => \shift_reg_reg_n_0_[7][2]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__3_n_0\,
      Q => \shift_reg_reg_n_0_[7][3]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__2_n_0\,
      Q => \shift_reg_reg_n_0_[7][4]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__1_n_0\,
      Q => \shift_reg_reg_n_0_[7][5]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => \shift_reg_reg_gate__0_n_0\,
      Q => \shift_reg_reg_n_0_[7][6]\,
      R => \running_sum_reg[0]_0\
    );
\shift_reg_reg[7][7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_gate_n_0,
      Q => \shift_reg_reg_n_0_[7][7]\,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_gate: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][7]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => shift_reg_reg_gate_n_0
    );
\shift_reg_reg_gate__0\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][6]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__0_n_0\
    );
\shift_reg_reg_gate__1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][5]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__1_n_0\
    );
\shift_reg_reg_gate__2\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][4]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__2_n_0\
    );
\shift_reg_reg_gate__3\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][3]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__3_n_0\
    );
\shift_reg_reg_gate__4\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][2]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__4_n_0\
    );
\shift_reg_reg_gate__5\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][1]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__5_n_0\
    );
\shift_reg_reg_gate__6\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][0]_inst_u_filter_ir_shift_reg_reg_r_11_n_0\,
      I1 => shift_reg_reg_r_11_n_0,
      O => \shift_reg_reg_gate__6_n_0\
    );
shift_reg_reg_r: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => '1',
      Q => shift_reg_reg_r_n_0,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_r_10: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_r_9_n_0,
      Q => shift_reg_reg_r_10_n_0,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_r_11: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_r_10_n_0,
      Q => shift_reg_reg_r_11_n_0,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_r_6: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_r_n_0,
      Q => shift_reg_reg_r_6_n_0,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_r_7: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_r_6_n_0,
      Q => shift_reg_reg_r_7_n_0,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_r_8: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_r_7_n_0,
      Q => shift_reg_reg_r_8_n_0,
      R => \running_sum_reg[0]_0\
    );
shift_reg_reg_r_9: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => ir_sample_valid,
      D => shift_reg_reg_r_8_n_0,
      Q => shift_reg_reg_r_9_n_0,
      R => \running_sum_reg[0]_0\
    );
end STRUCTURE;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0 is
  port (
    red_filter_valid : out STD_LOGIC;
    data_out : out STD_LOGIC_VECTOR ( 7 downto 0 );
    DI : out STD_LOGIC_VECTOR ( 3 downto 0 );
    \data_out_reg[6]_0\ : out STD_LOGIC_VECTOR ( 3 downto 0 );
    S : out STD_LOGIC_VECTOR ( 3 downto 0 );
    \reg_red_raw_reg[0]\ : out STD_LOGIC;
    D : out STD_LOGIC_VECTOR ( 6 downto 0 );
    \data_out_reg[6]_1\ : out STD_LOGIC_VECTOR ( 3 downto 0 );
    \data_out_reg[0]_0\ : in STD_LOGIC;
    red_sample_valid : in STD_LOGIC;
    s_axi_aclk : in STD_LOGIC;
    Q : in STD_LOGIC_VECTOR ( 7 downto 0 );
    data3 : in STD_LOGIC_VECTOR ( 8 downto 0 );
    sel0 : in STD_LOGIC_VECTOR ( 2 downto 0 );
    \s_axi_rdata_reg[1]\ : in STD_LOGIC;
    \s_axi_rdata_reg[7]\ : in STD_LOGIC_VECTOR ( 6 downto 0 );
    \s_axi_rdata_reg[2]\ : in STD_LOGIC;
    \s_axi_rdata_reg[3]\ : in STD_LOGIC;
    \s_axi_rdata_reg[4]\ : in STD_LOGIC;
    \s_axi_rdata_reg[5]\ : in STD_LOGIC;
    \s_axi_rdata_reg[6]\ : in STD_LOGIC;
    \s_axi_rdata_reg[7]_0\ : in STD_LOGIC;
    \s_axi_rdata_reg[7]_1\ : in STD_LOGIC_VECTOR ( 6 downto 0 )
  );
  attribute ORIG_REF_NAME : string;
  attribute ORIG_REF_NAME of design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0 : entity is "moving_average_8tap";
end design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0;

architecture STRUCTURE of design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0 is
  signal \^data_out\ : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal \next_sum_carry__0_i_1_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_2_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_3_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_4_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_5_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_6_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_7_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_i_8_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_n_0\ : STD_LOGIC;
  signal \next_sum_carry__0_n_1\ : STD_LOGIC;
  signal \next_sum_carry__0_n_2\ : STD_LOGIC;
  signal \next_sum_carry__0_n_3\ : STD_LOGIC;
  signal \next_sum_carry__1_i_1_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_i_2_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_i_3_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_i_4_n_0\ : STD_LOGIC;
  signal \next_sum_carry__1_n_2\ : STD_LOGIC;
  signal \next_sum_carry__1_n_3\ : STD_LOGIC;
  signal next_sum_carry_i_1_n_0 : STD_LOGIC;
  signal next_sum_carry_i_2_n_0 : STD_LOGIC;
  signal next_sum_carry_i_3_n_0 : STD_LOGIC;
  signal next_sum_carry_i_4_n_0 : STD_LOGIC;
  signal next_sum_carry_i_5_n_0 : STD_LOGIC;
  signal next_sum_carry_i_6_n_0 : STD_LOGIC;
  signal next_sum_carry_i_7_n_0 : STD_LOGIC;
  signal next_sum_carry_n_0 : STD_LOGIC;
  signal next_sum_carry_n_1 : STD_LOGIC;
  signal next_sum_carry_n_2 : STD_LOGIC;
  signal next_sum_carry_n_3 : STD_LOGIC;
  signal next_sum_carry_n_5 : STD_LOGIC;
  signal next_sum_carry_n_6 : STD_LOGIC;
  signal next_sum_carry_n_7 : STD_LOGIC;
  signal p_1_in : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal running_sum : STD_LOGIC_VECTOR ( 2 downto 0 );
  signal \s_axi_rdata[1]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[2]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[3]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[4]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[5]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[6]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[7]_i_2_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg[7]\ : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal \shift_reg_reg_gate__0_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__1_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__2_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__3_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__4_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__5_n_0\ : STD_LOGIC;
  signal \shift_reg_reg_gate__6_n_0\ : STD_LOGIC;
  signal shift_reg_reg_gate_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_0_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_1_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_2_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_3_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_4_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_5_n_0 : STD_LOGIC;
  signal shift_reg_reg_r_n_0 : STD_LOGIC;
  signal \NLW_next_sum_carry__1_CO_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 downto 2 );
  signal \NLW_next_sum_carry__1_O_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 to 3 );
  attribute ADDER_THRESHOLD : integer;
  attribute ADDER_THRESHOLD of next_sum_carry : label is 35;
  attribute ADDER_THRESHOLD of \next_sum_carry__0\ : label is 35;
  attribute HLUTNM : string;
  attribute HLUTNM of \next_sum_carry__0_i_1\ : label is "lutpair5";
  attribute HLUTNM of \next_sum_carry__0_i_2\ : label is "lutpair4";
  attribute HLUTNM of \next_sum_carry__0_i_3\ : label is "lutpair3";
  attribute HLUTNM of \next_sum_carry__0_i_4\ : label is "lutpair2";
  attribute HLUTNM of \next_sum_carry__0_i_6\ : label is "lutpair5";
  attribute HLUTNM of \next_sum_carry__0_i_7\ : label is "lutpair4";
  attribute HLUTNM of \next_sum_carry__0_i_8\ : label is "lutpair3";
  attribute ADDER_THRESHOLD of \next_sum_carry__1\ : label is 35;
  attribute HLUTNM of next_sum_carry_i_1 : label is "lutpair1";
  attribute HLUTNM of next_sum_carry_i_2 : label is "lutpair0";
  attribute HLUTNM of next_sum_carry_i_3 : label is "lutpair12";
  attribute HLUTNM of next_sum_carry_i_4 : label is "lutpair2";
  attribute HLUTNM of next_sum_carry_i_5 : label is "lutpair1";
  attribute HLUTNM of next_sum_carry_i_6 : label is "lutpair0";
  attribute HLUTNM of next_sum_carry_i_7 : label is "lutpair12";
  attribute srl_bus_name : string;
  attribute srl_bus_name of \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name : string;
  attribute srl_name of \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute srl_bus_name of \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5] ";
  attribute srl_name of \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4\ : label is "\inst/u_filter_red/shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4 ";
  attribute SOFT_HLUTNM : string;
  attribute SOFT_HLUTNM of shift_reg_reg_gate : label is "soft_lutpair4";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__0\ : label is "soft_lutpair4";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__1\ : label is "soft_lutpair5";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__2\ : label is "soft_lutpair5";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__3\ : label is "soft_lutpair6";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__4\ : label is "soft_lutpair6";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__5\ : label is "soft_lutpair7";
  attribute SOFT_HLUTNM of \shift_reg_reg_gate__6\ : label is "soft_lutpair7";
begin
  data_out(7 downto 0) <= \^data_out\(7 downto 0);
\data_out_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(0),
      Q => \^data_out\(0),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(1),
      Q => \^data_out\(1),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(2),
      Q => \^data_out\(2),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(3),
      Q => \^data_out\(3),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(4),
      Q => \^data_out\(4),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(5),
      Q => \^data_out\(5),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(6),
      Q => \^data_out\(6),
      R => \data_out_reg[0]_0\
    );
\data_out_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => p_1_in(7),
      Q => \^data_out\(7),
      R => \data_out_reg[0]_0\
    );
\i__carry_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => data3(7),
      I1 => \^data_out\(6),
      I2 => data3(8),
      I3 => \^data_out\(7),
      O => DI(3)
    );
\i__carry_i_2\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => data3(5),
      I1 => \^data_out\(4),
      I2 => data3(6),
      I3 => \^data_out\(5),
      O => DI(2)
    );
\i__carry_i_3\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => data3(3),
      I1 => \^data_out\(2),
      I2 => data3(4),
      I3 => \^data_out\(3),
      O => DI(1)
    );
\i__carry_i_4\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => data3(1),
      I1 => \^data_out\(0),
      I2 => data3(2),
      I3 => \^data_out\(1),
      O => DI(0)
    );
\i__carry_i_5\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(6),
      I1 => data3(7),
      I2 => \^data_out\(7),
      I3 => data3(8),
      O => S(3)
    );
\i__carry_i_6\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(4),
      I1 => data3(5),
      I2 => \^data_out\(5),
      I3 => data3(6),
      O => S(2)
    );
\i__carry_i_7\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(2),
      I1 => data3(3),
      I2 => \^data_out\(3),
      I3 => data3(4),
      O => S(1)
    );
\i__carry_i_8\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(0),
      I1 => data3(1),
      I2 => \^data_out\(1),
      I3 => data3(2),
      O => S(0)
    );
next_state1_carry_i_1: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => \^data_out\(6),
      I1 => data3(7),
      I2 => \^data_out\(7),
      I3 => data3(8),
      O => \data_out_reg[6]_0\(3)
    );
next_state1_carry_i_2: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => \^data_out\(4),
      I1 => data3(5),
      I2 => \^data_out\(5),
      I3 => data3(6),
      O => \data_out_reg[6]_0\(2)
    );
next_state1_carry_i_3: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => \^data_out\(2),
      I1 => data3(3),
      I2 => \^data_out\(3),
      I3 => data3(4),
      O => \data_out_reg[6]_0\(1)
    );
next_state1_carry_i_4: unisim.vcomponents.LUT4
    generic map(
      INIT => X"20F2"
    )
        port map (
      I0 => \^data_out\(0),
      I1 => data3(1),
      I2 => \^data_out\(1),
      I3 => data3(2),
      O => \data_out_reg[6]_0\(0)
    );
next_state1_carry_i_5: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(6),
      I1 => data3(7),
      I2 => \^data_out\(7),
      I3 => data3(8),
      O => \data_out_reg[6]_1\(3)
    );
next_state1_carry_i_6: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(4),
      I1 => data3(5),
      I2 => \^data_out\(5),
      I3 => data3(6),
      O => \data_out_reg[6]_1\(2)
    );
next_state1_carry_i_7: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(2),
      I1 => data3(3),
      I2 => \^data_out\(3),
      I3 => data3(4),
      O => \data_out_reg[6]_1\(1)
    );
next_state1_carry_i_8: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => \^data_out\(0),
      I1 => data3(1),
      I2 => \^data_out\(1),
      I3 => data3(2),
      O => \data_out_reg[6]_1\(0)
    );
next_sum_carry: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => next_sum_carry_n_0,
      CO(2) => next_sum_carry_n_1,
      CO(1) => next_sum_carry_n_2,
      CO(0) => next_sum_carry_n_3,
      CYINIT => '0',
      DI(3) => next_sum_carry_i_1_n_0,
      DI(2) => next_sum_carry_i_2_n_0,
      DI(1) => next_sum_carry_i_3_n_0,
      DI(0) => running_sum(0),
      O(3) => p_1_in(0),
      O(2) => next_sum_carry_n_5,
      O(1) => next_sum_carry_n_6,
      O(0) => next_sum_carry_n_7,
      S(3) => next_sum_carry_i_4_n_0,
      S(2) => next_sum_carry_i_5_n_0,
      S(1) => next_sum_carry_i_6_n_0,
      S(0) => next_sum_carry_i_7_n_0
    );
\next_sum_carry__0\: unisim.vcomponents.CARRY4
     port map (
      CI => next_sum_carry_n_0,
      CO(3) => \next_sum_carry__0_n_0\,
      CO(2) => \next_sum_carry__0_n_1\,
      CO(1) => \next_sum_carry__0_n_2\,
      CO(0) => \next_sum_carry__0_n_3\,
      CYINIT => '0',
      DI(3) => \next_sum_carry__0_i_1_n_0\,
      DI(2) => \next_sum_carry__0_i_2_n_0\,
      DI(1) => \next_sum_carry__0_i_3_n_0\,
      DI(0) => \next_sum_carry__0_i_4_n_0\,
      O(3 downto 0) => p_1_in(4 downto 1),
      S(3) => \next_sum_carry__0_i_5_n_0\,
      S(2) => \next_sum_carry__0_i_6_n_0\,
      S(1) => \next_sum_carry__0_i_7_n_0\,
      S(0) => \next_sum_carry__0_i_8_n_0\
    );
\next_sum_carry__0_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => \^data_out\(3),
      I1 => Q(6),
      I2 => \shift_reg_reg[7]\(6),
      O => \next_sum_carry__0_i_1_n_0\
    );
\next_sum_carry__0_i_2\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => \^data_out\(2),
      I1 => Q(5),
      I2 => \shift_reg_reg[7]\(5),
      O => \next_sum_carry__0_i_2_n_0\
    );
\next_sum_carry__0_i_3\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => \^data_out\(1),
      I1 => Q(4),
      I2 => \shift_reg_reg[7]\(4),
      O => \next_sum_carry__0_i_3_n_0\
    );
\next_sum_carry__0_i_4\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => \^data_out\(0),
      I1 => Q(3),
      I2 => \shift_reg_reg[7]\(3),
      O => \next_sum_carry__0_i_4_n_0\
    );
\next_sum_carry__0_i_5\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \next_sum_carry__0_i_1_n_0\,
      I1 => \shift_reg_reg[7]\(7),
      I2 => Q(7),
      I3 => \^data_out\(4),
      O => \next_sum_carry__0_i_5_n_0\
    );
\next_sum_carry__0_i_6\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \^data_out\(3),
      I1 => Q(6),
      I2 => \shift_reg_reg[7]\(6),
      I3 => \next_sum_carry__0_i_2_n_0\,
      O => \next_sum_carry__0_i_6_n_0\
    );
\next_sum_carry__0_i_7\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \^data_out\(2),
      I1 => Q(5),
      I2 => \shift_reg_reg[7]\(5),
      I3 => \next_sum_carry__0_i_3_n_0\,
      O => \next_sum_carry__0_i_7_n_0\
    );
\next_sum_carry__0_i_8\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \^data_out\(1),
      I1 => Q(4),
      I2 => \shift_reg_reg[7]\(4),
      I3 => \next_sum_carry__0_i_4_n_0\,
      O => \next_sum_carry__0_i_8_n_0\
    );
\next_sum_carry__1\: unisim.vcomponents.CARRY4
     port map (
      CI => \next_sum_carry__0_n_0\,
      CO(3 downto 2) => \NLW_next_sum_carry__1_CO_UNCONNECTED\(3 downto 2),
      CO(1) => \next_sum_carry__1_n_2\,
      CO(0) => \next_sum_carry__1_n_3\,
      CYINIT => '0',
      DI(3 downto 2) => B"00",
      DI(1) => \^data_out\(5),
      DI(0) => \next_sum_carry__1_i_1_n_0\,
      O(3) => \NLW_next_sum_carry__1_O_UNCONNECTED\(3),
      O(2 downto 0) => p_1_in(7 downto 5),
      S(3) => '0',
      S(2) => \next_sum_carry__1_i_2_n_0\,
      S(1) => \next_sum_carry__1_i_3_n_0\,
      S(0) => \next_sum_carry__1_i_4_n_0\
    );
\next_sum_carry__1_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => \^data_out\(4),
      I1 => Q(7),
      I2 => \shift_reg_reg[7]\(7),
      O => \next_sum_carry__1_i_1_n_0\
    );
\next_sum_carry__1_i_2\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"9"
    )
        port map (
      I0 => \^data_out\(6),
      I1 => \^data_out\(7),
      O => \next_sum_carry__1_i_2_n_0\
    );
\next_sum_carry__1_i_3\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"9"
    )
        port map (
      I0 => \^data_out\(5),
      I1 => \^data_out\(6),
      O => \next_sum_carry__1_i_3_n_0\
    );
\next_sum_carry__1_i_4\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"D42B"
    )
        port map (
      I0 => \shift_reg_reg[7]\(7),
      I1 => Q(7),
      I2 => \^data_out\(4),
      I3 => \^data_out\(5),
      O => \next_sum_carry__1_i_4_n_0\
    );
next_sum_carry_i_1: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => Q(2),
      I1 => running_sum(2),
      I2 => \shift_reg_reg[7]\(2),
      O => next_sum_carry_i_1_n_0
    );
next_sum_carry_i_2: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8E"
    )
        port map (
      I0 => Q(1),
      I1 => running_sum(1),
      I2 => \shift_reg_reg[7]\(1),
      O => next_sum_carry_i_2_n_0
    );
next_sum_carry_i_3: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => Q(0),
      I1 => \shift_reg_reg[7]\(0),
      O => next_sum_carry_i_3_n_0
    );
next_sum_carry_i_4: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => \^data_out\(0),
      I1 => Q(3),
      I2 => \shift_reg_reg[7]\(3),
      I3 => next_sum_carry_i_1_n_0,
      O => next_sum_carry_i_4_n_0
    );
next_sum_carry_i_5: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => Q(2),
      I1 => running_sum(2),
      I2 => \shift_reg_reg[7]\(2),
      I3 => next_sum_carry_i_2_n_0,
      O => next_sum_carry_i_5_n_0
    );
next_sum_carry_i_6: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9669"
    )
        port map (
      I0 => Q(1),
      I1 => running_sum(1),
      I2 => \shift_reg_reg[7]\(1),
      I3 => next_sum_carry_i_3_n_0,
      O => next_sum_carry_i_6_n_0
    );
next_sum_carry_i_7: unisim.vcomponents.LUT3
    generic map(
      INIT => X"96"
    )
        port map (
      I0 => Q(0),
      I1 => \shift_reg_reg[7]\(0),
      I2 => running_sum(0),
      O => next_sum_carry_i_7_n_0
    );
out_valid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => red_sample_valid,
      Q => red_filter_valid,
      R => \data_out_reg[0]_0\
    );
\running_sum_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => next_sum_carry_n_7,
      Q => running_sum(0),
      R => \data_out_reg[0]_0\
    );
\running_sum_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => next_sum_carry_n_6,
      Q => running_sum(1),
      R => \data_out_reg[0]_0\
    );
\running_sum_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => next_sum_carry_n_5,
      Q => running_sum(2),
      R => \data_out_reg[0]_0\
    );
\s_axi_rdata[0]_i_3\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"00F000CC000000AA"
    )
        port map (
      I0 => Q(0),
      I1 => \^data_out\(0),
      I2 => data3(0),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => \reg_red_raw_reg[0]\
    );
\s_axi_rdata[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[1]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[1]\,
      I2 => \s_axi_rdata_reg[7]\(0),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(0)
    );
\s_axi_rdata[1]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(1),
      I1 => \^data_out\(1),
      I2 => \s_axi_rdata_reg[7]_1\(0),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[1]_i_2_n_0\
    );
\s_axi_rdata[2]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[2]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[2]\,
      I2 => \s_axi_rdata_reg[7]\(1),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(1)
    );
\s_axi_rdata[2]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(2),
      I1 => \^data_out\(2),
      I2 => \s_axi_rdata_reg[7]_1\(1),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[2]_i_2_n_0\
    );
\s_axi_rdata[3]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[3]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[3]\,
      I2 => \s_axi_rdata_reg[7]\(2),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(2)
    );
\s_axi_rdata[3]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(3),
      I1 => \^data_out\(3),
      I2 => \s_axi_rdata_reg[7]_1\(2),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[3]_i_2_n_0\
    );
\s_axi_rdata[4]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[4]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[4]\,
      I2 => \s_axi_rdata_reg[7]\(3),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(3)
    );
\s_axi_rdata[4]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(4),
      I1 => \^data_out\(4),
      I2 => \s_axi_rdata_reg[7]_1\(3),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[4]_i_2_n_0\
    );
\s_axi_rdata[5]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[5]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[5]\,
      I2 => \s_axi_rdata_reg[7]\(4),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(4)
    );
\s_axi_rdata[5]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(5),
      I1 => \^data_out\(5),
      I2 => \s_axi_rdata_reg[7]_1\(4),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[5]_i_2_n_0\
    );
\s_axi_rdata[6]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[6]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[6]\,
      I2 => \s_axi_rdata_reg[7]\(5),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(5)
    );
\s_axi_rdata[6]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(6),
      I1 => \^data_out\(6),
      I2 => \s_axi_rdata_reg[7]_1\(5),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[6]_i_2_n_0\
    );
\s_axi_rdata[7]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"AAAAEEAAAAFAAAAA"
    )
        port map (
      I0 => \s_axi_rdata[7]_i_2_n_0\,
      I1 => \s_axi_rdata_reg[7]_0\,
      I2 => \s_axi_rdata_reg[7]\(6),
      I3 => sel0(2),
      I4 => sel0(1),
      I5 => sel0(0),
      O => D(6)
    );
\s_axi_rdata[7]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"000000F000CC00AA"
    )
        port map (
      I0 => Q(7),
      I1 => \^data_out\(7),
      I2 => \s_axi_rdata_reg[7]_1\(6),
      I3 => sel0(1),
      I4 => sel0(0),
      I5 => sel0(2),
      O => \s_axi_rdata[7]_i_2_n_0\
    );
\shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(0),
      Q => \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(1),
      Q => \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(2),
      Q => \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(3),
      Q => \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(4),
      Q => \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(5),
      Q => \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(6),
      Q => \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4\: unisim.vcomponents.SRL16E
     port map (
      A0 => '1',
      A1 => '0',
      A2 => '1',
      A3 => '0',
      CE => red_sample_valid,
      CLK => s_axi_aclk,
      D => Q(7),
      Q => \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\
    );
\shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][0]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][1]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][2]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][3]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][4]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][5]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][6]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg[5][7]_srl6___inst_u_filter_red_shift_reg_reg_r_4_n_0\,
      Q => \shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      R => '0'
    );
\shift_reg_reg[7][0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__6_n_0\,
      Q => \shift_reg_reg[7]\(0),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__5_n_0\,
      Q => \shift_reg_reg[7]\(1),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__4_n_0\,
      Q => \shift_reg_reg[7]\(2),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__3_n_0\,
      Q => \shift_reg_reg[7]\(3),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__2_n_0\,
      Q => \shift_reg_reg[7]\(4),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__1_n_0\,
      Q => \shift_reg_reg[7]\(5),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => \shift_reg_reg_gate__0_n_0\,
      Q => \shift_reg_reg[7]\(6),
      R => \data_out_reg[0]_0\
    );
\shift_reg_reg[7][7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_gate_n_0,
      Q => \shift_reg_reg[7]\(7),
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_gate: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][7]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => shift_reg_reg_gate_n_0
    );
\shift_reg_reg_gate__0\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][6]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__0_n_0\
    );
\shift_reg_reg_gate__1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][5]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__1_n_0\
    );
\shift_reg_reg_gate__2\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][4]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__2_n_0\
    );
\shift_reg_reg_gate__3\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][3]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__3_n_0\
    );
\shift_reg_reg_gate__4\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][2]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__4_n_0\
    );
\shift_reg_reg_gate__5\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][1]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__5_n_0\
    );
\shift_reg_reg_gate__6\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \shift_reg_reg[6][0]_inst_u_filter_red_shift_reg_reg_r_5_n_0\,
      I1 => shift_reg_reg_r_5_n_0,
      O => \shift_reg_reg_gate__6_n_0\
    );
shift_reg_reg_r: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => '1',
      Q => shift_reg_reg_r_n_0,
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_r_0: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_r_n_0,
      Q => shift_reg_reg_r_0_n_0,
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_r_1: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_r_0_n_0,
      Q => shift_reg_reg_r_1_n_0,
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_r_2: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_r_1_n_0,
      Q => shift_reg_reg_r_2_n_0,
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_r_3: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_r_2_n_0,
      Q => shift_reg_reg_r_3_n_0,
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_r_4: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_r_3_n_0,
      Q => shift_reg_reg_r_4_n_0,
      R => \data_out_reg[0]_0\
    );
shift_reg_reg_r_5: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_sample_valid,
      D => shift_reg_reg_r_4_n_0,
      Q => shift_reg_reg_r_5_n_0,
      R => \data_out_reg[0]_0\
    );
end STRUCTURE;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector is
  port (
    beat_detected_reg_0 : out STD_LOGIC;
    SR : out STD_LOGIC_VECTOR ( 0 to 0 );
    \axi_araddr_latched_reg[2]\ : out STD_LOGIC_VECTOR ( 23 downto 0 );
    Q : out STD_LOGIC_VECTOR ( 7 downto 0 );
    beat_detected_reg_1 : out STD_LOGIC;
    s_axi_aclk : in STD_LOGIC;
    \FSM_sequential_current_state[1]_i_2_0\ : in STD_LOGIC_VECTOR ( 3 downto 0 );
    \FSM_sequential_current_state[1]_i_2_1\ : in STD_LOGIC_VECTOR ( 3 downto 0 );
    DI : in STD_LOGIC_VECTOR ( 3 downto 0 );
    S : in STD_LOGIC_VECTOR ( 3 downto 0 );
    D : in STD_LOGIC_VECTOR ( 7 downto 0 );
    red_filter_valid : in STD_LOGIC;
    data3 : in STD_LOGIC_VECTOR ( 8 downto 0 );
    sel0 : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_aresetn : in STD_LOGIC;
    beat_flag_reg : in STD_LOGIC_VECTOR ( 0 to 0 );
    p_1_in_0 : in STD_LOGIC_VECTOR ( 2 downto 0 );
    write_execute : in STD_LOGIC
  );
  attribute ORIG_REF_NAME : string;
  attribute ORIG_REF_NAME of design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector : entity is "ppg_peak_detector";
end design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector;

architecture STRUCTURE of design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector is
  signal \FSM_sequential_current_state[0]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_10_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_11_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_12_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_2_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_3_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_4_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_5_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_6_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_7_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_8_n_0\ : STD_LOGIC;
  signal \FSM_sequential_current_state[1]_i_9_n_0\ : STD_LOGIC;
  signal \^sr\ : STD_LOGIC_VECTOR ( 0 to 0 );
  signal beat_detected_i_1_n_0 : STD_LOGIC;
  signal \^beat_detected_reg_0\ : STD_LOGIC;
  signal beat_flag_i_2_n_0 : STD_LOGIC;
  signal beat_flag_i_3_n_0 : STD_LOGIC;
  signal current_state : STD_LOGIC_VECTOR ( 1 downto 0 );
  signal \fall_count[0]_i_1_n_0\ : STD_LOGIC;
  signal \fall_count[1]_i_1_n_0\ : STD_LOGIC;
  signal \fall_count_reg_n_0_[0]\ : STD_LOGIC;
  signal \fall_count_reg_n_0_[1]\ : STD_LOGIC;
  signal first_beat_seen_i_1_n_0 : STD_LOGIC;
  signal first_beat_seen_reg_n_0 : STD_LOGIC;
  signal ibi_cycles : STD_LOGIC_VECTOR ( 31 downto 8 );
  signal in7 : STD_LOGIC_VECTOR ( 31 downto 1 );
  signal in9 : STD_LOGIC_VECTOR ( 31 downto 1 );
  signal interval_cnt : STD_LOGIC_VECTOR ( 31 downto 0 );
  signal \interval_cnt0_carry__0_n_0\ : STD_LOGIC;
  signal \interval_cnt0_carry__0_n_1\ : STD_LOGIC;
  signal \interval_cnt0_carry__0_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__0_n_3\ : STD_LOGIC;
  signal \interval_cnt0_carry__1_n_0\ : STD_LOGIC;
  signal \interval_cnt0_carry__1_n_1\ : STD_LOGIC;
  signal \interval_cnt0_carry__1_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__1_n_3\ : STD_LOGIC;
  signal \interval_cnt0_carry__2_n_0\ : STD_LOGIC;
  signal \interval_cnt0_carry__2_n_1\ : STD_LOGIC;
  signal \interval_cnt0_carry__2_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__2_n_3\ : STD_LOGIC;
  signal \interval_cnt0_carry__3_n_0\ : STD_LOGIC;
  signal \interval_cnt0_carry__3_n_1\ : STD_LOGIC;
  signal \interval_cnt0_carry__3_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__3_n_3\ : STD_LOGIC;
  signal \interval_cnt0_carry__4_n_0\ : STD_LOGIC;
  signal \interval_cnt0_carry__4_n_1\ : STD_LOGIC;
  signal \interval_cnt0_carry__4_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__4_n_3\ : STD_LOGIC;
  signal \interval_cnt0_carry__5_n_0\ : STD_LOGIC;
  signal \interval_cnt0_carry__5_n_1\ : STD_LOGIC;
  signal \interval_cnt0_carry__5_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__5_n_3\ : STD_LOGIC;
  signal \interval_cnt0_carry__6_n_2\ : STD_LOGIC;
  signal \interval_cnt0_carry__6_n_3\ : STD_LOGIC;
  signal interval_cnt0_carry_n_0 : STD_LOGIC;
  signal interval_cnt0_carry_n_1 : STD_LOGIC;
  signal interval_cnt0_carry_n_2 : STD_LOGIC;
  signal interval_cnt0_carry_n_3 : STD_LOGIC;
  signal \interval_cnt[31]_i_3_n_0\ : STD_LOGIC;
  signal \interval_cnt[31]_i_4_n_0\ : STD_LOGIC;
  signal \interval_cnt[31]_i_5_n_0\ : STD_LOGIC;
  signal \interval_cnt[31]_i_6_n_0\ : STD_LOGIC;
  signal \interval_cnt[31]_i_7_n_0\ : STD_LOGIC;
  signal \interval_cnt[31]_i_8_n_0\ : STD_LOGIC;
  signal interval_cnt_1 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal \interval_cnt_reg_n_0_[0]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[10]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[11]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[12]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[13]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[14]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[15]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[16]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[17]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[18]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[19]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[1]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[20]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[21]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[22]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[23]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[24]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[25]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[26]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[27]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[28]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[29]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[2]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[30]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[31]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[3]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[4]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[5]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[6]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[7]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[8]\ : STD_LOGIC;
  signal \interval_cnt_reg_n_0_[9]\ : STD_LOGIC;
  signal next_state1 : STD_LOGIC;
  signal next_state13_in : STD_LOGIC;
  signal next_state1_carry_n_1 : STD_LOGIC;
  signal next_state1_carry_n_2 : STD_LOGIC;
  signal next_state1_carry_n_3 : STD_LOGIC;
  signal \next_state1_inferred__0/i__carry_n_1\ : STD_LOGIC;
  signal \next_state1_inferred__0/i__carry_n_2\ : STD_LOGIC;
  signal \next_state1_inferred__0/i__carry_n_3\ : STD_LOGIC;
  signal next_state20_in : STD_LOGIC;
  signal next_state2_carry_i_1_n_0 : STD_LOGIC;
  signal next_state2_carry_i_2_n_0 : STD_LOGIC;
  signal next_state2_carry_i_3_n_0 : STD_LOGIC;
  signal next_state2_carry_i_4_n_0 : STD_LOGIC;
  signal next_state2_carry_i_5_n_0 : STD_LOGIC;
  signal next_state2_carry_i_6_n_0 : STD_LOGIC;
  signal next_state2_carry_i_7_n_0 : STD_LOGIC;
  signal next_state2_carry_i_8_n_0 : STD_LOGIC;
  signal next_state2_carry_n_1 : STD_LOGIC;
  signal next_state2_carry_n_2 : STD_LOGIC;
  signal next_state2_carry_n_3 : STD_LOGIC;
  signal prev_sample : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal refractory_cnt : STD_LOGIC_VECTOR ( 31 downto 1 );
  signal \refractory_cnt0_carry__0_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_i_4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_n_1\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__0_n_3\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_i_4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_n_1\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__1_n_3\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_i_4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_n_1\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__2_n_3\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_i_4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_n_1\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__3_n_3\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_i_4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_n_1\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__4_n_3\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_i_4_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_n_1\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__5_n_3\ : STD_LOGIC;
  signal \refractory_cnt0_carry__6_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__6_i_2_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__6_i_3_n_0\ : STD_LOGIC;
  signal \refractory_cnt0_carry__6_n_2\ : STD_LOGIC;
  signal \refractory_cnt0_carry__6_n_3\ : STD_LOGIC;
  signal refractory_cnt0_carry_i_1_n_0 : STD_LOGIC;
  signal refractory_cnt0_carry_i_2_n_0 : STD_LOGIC;
  signal refractory_cnt0_carry_i_3_n_0 : STD_LOGIC;
  signal refractory_cnt0_carry_i_4_n_0 : STD_LOGIC;
  signal refractory_cnt0_carry_n_0 : STD_LOGIC;
  signal refractory_cnt0_carry_n_1 : STD_LOGIC;
  signal refractory_cnt0_carry_n_2 : STD_LOGIC;
  signal refractory_cnt0_carry_n_3 : STD_LOGIC;
  signal \refractory_cnt[0]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[10]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[11]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[12]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[13]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[15]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[17]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[18]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[19]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[20]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[21]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[23]_i_1_n_0\ : STD_LOGIC;
  signal \refractory_cnt[5]_i_1_n_0\ : STD_LOGIC;
  signal refractory_cnt_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal \refractory_cnt_reg_n_0_[0]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[10]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[11]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[12]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[13]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[14]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[15]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[16]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[17]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[18]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[19]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[1]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[20]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[21]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[22]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[23]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[24]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[25]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[26]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[27]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[28]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[29]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[2]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[30]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[31]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[3]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[4]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[5]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[6]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[7]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[8]\ : STD_LOGIC;
  signal \refractory_cnt_reg_n_0_[9]\ : STD_LOGIC;
  signal \NLW_interval_cnt0_carry__6_CO_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 downto 2 );
  signal \NLW_interval_cnt0_carry__6_O_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 to 3 );
  signal NLW_next_state1_carry_O_UNCONNECTED : STD_LOGIC_VECTOR ( 3 downto 0 );
  signal \NLW_next_state1_inferred__0/i__carry_O_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 downto 0 );
  signal NLW_next_state2_carry_O_UNCONNECTED : STD_LOGIC_VECTOR ( 3 downto 0 );
  signal \NLW_refractory_cnt0_carry__6_CO_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 downto 2 );
  signal \NLW_refractory_cnt0_carry__6_O_UNCONNECTED\ : STD_LOGIC_VECTOR ( 3 to 3 );
  attribute FSM_ENCODED_STATES : string;
  attribute FSM_ENCODED_STATES of \FSM_sequential_current_state_reg[0]\ : label is "STATE_ARMED:00,STATE_RISING:01,STATE_PEAK_FOUND:10,STATE_REFRACTORY:11";
  attribute FSM_ENCODED_STATES of \FSM_sequential_current_state_reg[1]\ : label is "STATE_ARMED:00,STATE_RISING:01,STATE_PEAK_FOUND:10,STATE_REFRACTORY:11";
  attribute SOFT_HLUTNM : string;
  attribute SOFT_HLUTNM of beat_flag_i_2 : label is "soft_lutpair8";
  attribute SOFT_HLUTNM of beat_flag_i_3 : label is "soft_lutpair8";
  attribute ADDER_THRESHOLD : integer;
  attribute ADDER_THRESHOLD of interval_cnt0_carry : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__0\ : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__1\ : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__2\ : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__3\ : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__4\ : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__5\ : label is 35;
  attribute ADDER_THRESHOLD of \interval_cnt0_carry__6\ : label is 35;
  attribute SOFT_HLUTNM of \interval_cnt[0]_i_1\ : label is "soft_lutpair9";
  attribute SOFT_HLUTNM of \interval_cnt[10]_i_1\ : label is "soft_lutpair22";
  attribute SOFT_HLUTNM of \interval_cnt[11]_i_1\ : label is "soft_lutpair23";
  attribute SOFT_HLUTNM of \interval_cnt[12]_i_1\ : label is "soft_lutpair23";
  attribute SOFT_HLUTNM of \interval_cnt[13]_i_1\ : label is "soft_lutpair24";
  attribute SOFT_HLUTNM of \interval_cnt[14]_i_1\ : label is "soft_lutpair24";
  attribute SOFT_HLUTNM of \interval_cnt[15]_i_1\ : label is "soft_lutpair25";
  attribute SOFT_HLUTNM of \interval_cnt[16]_i_1\ : label is "soft_lutpair25";
  attribute SOFT_HLUTNM of \interval_cnt[17]_i_1\ : label is "soft_lutpair26";
  attribute SOFT_HLUTNM of \interval_cnt[18]_i_1\ : label is "soft_lutpair26";
  attribute SOFT_HLUTNM of \interval_cnt[19]_i_1\ : label is "soft_lutpair27";
  attribute SOFT_HLUTNM of \interval_cnt[1]_i_1\ : label is "soft_lutpair18";
  attribute SOFT_HLUTNM of \interval_cnt[20]_i_1\ : label is "soft_lutpair27";
  attribute SOFT_HLUTNM of \interval_cnt[21]_i_1\ : label is "soft_lutpair28";
  attribute SOFT_HLUTNM of \interval_cnt[22]_i_1\ : label is "soft_lutpair28";
  attribute SOFT_HLUTNM of \interval_cnt[23]_i_1\ : label is "soft_lutpair29";
  attribute SOFT_HLUTNM of \interval_cnt[24]_i_1\ : label is "soft_lutpair29";
  attribute SOFT_HLUTNM of \interval_cnt[25]_i_1\ : label is "soft_lutpair30";
  attribute SOFT_HLUTNM of \interval_cnt[26]_i_1\ : label is "soft_lutpair30";
  attribute SOFT_HLUTNM of \interval_cnt[27]_i_1\ : label is "soft_lutpair31";
  attribute SOFT_HLUTNM of \interval_cnt[28]_i_1\ : label is "soft_lutpair31";
  attribute SOFT_HLUTNM of \interval_cnt[29]_i_1\ : label is "soft_lutpair32";
  attribute SOFT_HLUTNM of \interval_cnt[2]_i_1\ : label is "soft_lutpair18";
  attribute SOFT_HLUTNM of \interval_cnt[30]_i_1\ : label is "soft_lutpair32";
  attribute SOFT_HLUTNM of \interval_cnt[31]_i_7\ : label is "soft_lutpair9";
  attribute SOFT_HLUTNM of \interval_cnt[3]_i_1\ : label is "soft_lutpair19";
  attribute SOFT_HLUTNM of \interval_cnt[4]_i_1\ : label is "soft_lutpair19";
  attribute SOFT_HLUTNM of \interval_cnt[5]_i_1\ : label is "soft_lutpair20";
  attribute SOFT_HLUTNM of \interval_cnt[6]_i_1\ : label is "soft_lutpair20";
  attribute SOFT_HLUTNM of \interval_cnt[7]_i_1\ : label is "soft_lutpair21";
  attribute SOFT_HLUTNM of \interval_cnt[8]_i_1\ : label is "soft_lutpair21";
  attribute SOFT_HLUTNM of \interval_cnt[9]_i_1\ : label is "soft_lutpair22";
  attribute COMPARATOR_THRESHOLD : integer;
  attribute COMPARATOR_THRESHOLD of next_state1_carry : label is 11;
  attribute COMPARATOR_THRESHOLD of \next_state1_inferred__0/i__carry\ : label is 11;
  attribute COMPARATOR_THRESHOLD of next_state2_carry : label is 11;
  attribute ADDER_THRESHOLD of refractory_cnt0_carry : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__0\ : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__1\ : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__2\ : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__3\ : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__4\ : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__5\ : label is 35;
  attribute ADDER_THRESHOLD of \refractory_cnt0_carry__6\ : label is 35;
  attribute SOFT_HLUTNM of \refractory_cnt[0]_i_1\ : label is "soft_lutpair39";
  attribute SOFT_HLUTNM of \refractory_cnt[10]_i_1\ : label is "soft_lutpair38";
  attribute SOFT_HLUTNM of \refractory_cnt[11]_i_1\ : label is "soft_lutpair37";
  attribute SOFT_HLUTNM of \refractory_cnt[12]_i_1\ : label is "soft_lutpair37";
  attribute SOFT_HLUTNM of \refractory_cnt[13]_i_1\ : label is "soft_lutpair36";
  attribute SOFT_HLUTNM of \refractory_cnt[14]_i_1\ : label is "soft_lutpair44";
  attribute SOFT_HLUTNM of \refractory_cnt[15]_i_1\ : label is "soft_lutpair36";
  attribute SOFT_HLUTNM of \refractory_cnt[16]_i_1\ : label is "soft_lutpair44";
  attribute SOFT_HLUTNM of \refractory_cnt[17]_i_1\ : label is "soft_lutpair35";
  attribute SOFT_HLUTNM of \refractory_cnt[18]_i_1\ : label is "soft_lutpair35";
  attribute SOFT_HLUTNM of \refractory_cnt[19]_i_1\ : label is "soft_lutpair34";
  attribute SOFT_HLUTNM of \refractory_cnt[1]_i_1\ : label is "soft_lutpair48";
  attribute SOFT_HLUTNM of \refractory_cnt[20]_i_1\ : label is "soft_lutpair34";
  attribute SOFT_HLUTNM of \refractory_cnt[21]_i_1\ : label is "soft_lutpair33";
  attribute SOFT_HLUTNM of \refractory_cnt[22]_i_1\ : label is "soft_lutpair43";
  attribute SOFT_HLUTNM of \refractory_cnt[23]_i_1\ : label is "soft_lutpair33";
  attribute SOFT_HLUTNM of \refractory_cnt[24]_i_1\ : label is "soft_lutpair43";
  attribute SOFT_HLUTNM of \refractory_cnt[25]_i_1\ : label is "soft_lutpair42";
  attribute SOFT_HLUTNM of \refractory_cnt[26]_i_1\ : label is "soft_lutpair42";
  attribute SOFT_HLUTNM of \refractory_cnt[27]_i_1\ : label is "soft_lutpair41";
  attribute SOFT_HLUTNM of \refractory_cnt[28]_i_1\ : label is "soft_lutpair41";
  attribute SOFT_HLUTNM of \refractory_cnt[29]_i_1\ : label is "soft_lutpair40";
  attribute SOFT_HLUTNM of \refractory_cnt[2]_i_1\ : label is "soft_lutpair48";
  attribute SOFT_HLUTNM of \refractory_cnt[30]_i_1\ : label is "soft_lutpair40";
  attribute SOFT_HLUTNM of \refractory_cnt[31]_i_2\ : label is "soft_lutpair39";
  attribute SOFT_HLUTNM of \refractory_cnt[3]_i_1\ : label is "soft_lutpair47";
  attribute SOFT_HLUTNM of \refractory_cnt[4]_i_1\ : label is "soft_lutpair47";
  attribute SOFT_HLUTNM of \refractory_cnt[5]_i_1\ : label is "soft_lutpair38";
  attribute SOFT_HLUTNM of \refractory_cnt[6]_i_1\ : label is "soft_lutpair46";
  attribute SOFT_HLUTNM of \refractory_cnt[7]_i_1\ : label is "soft_lutpair46";
  attribute SOFT_HLUTNM of \refractory_cnt[8]_i_1\ : label is "soft_lutpair45";
  attribute SOFT_HLUTNM of \refractory_cnt[9]_i_1\ : label is "soft_lutpair45";
  attribute SOFT_HLUTNM of \s_axi_rdata[16]_i_1\ : label is "soft_lutpair10";
  attribute SOFT_HLUTNM of \s_axi_rdata[17]_i_1\ : label is "soft_lutpair10";
  attribute SOFT_HLUTNM of \s_axi_rdata[18]_i_1\ : label is "soft_lutpair11";
  attribute SOFT_HLUTNM of \s_axi_rdata[19]_i_1\ : label is "soft_lutpair11";
  attribute SOFT_HLUTNM of \s_axi_rdata[20]_i_1\ : label is "soft_lutpair12";
  attribute SOFT_HLUTNM of \s_axi_rdata[21]_i_1\ : label is "soft_lutpair12";
  attribute SOFT_HLUTNM of \s_axi_rdata[22]_i_1\ : label is "soft_lutpair13";
  attribute SOFT_HLUTNM of \s_axi_rdata[23]_i_1\ : label is "soft_lutpair13";
  attribute SOFT_HLUTNM of \s_axi_rdata[24]_i_1\ : label is "soft_lutpair14";
  attribute SOFT_HLUTNM of \s_axi_rdata[25]_i_1\ : label is "soft_lutpair14";
  attribute SOFT_HLUTNM of \s_axi_rdata[26]_i_1\ : label is "soft_lutpair15";
  attribute SOFT_HLUTNM of \s_axi_rdata[27]_i_1\ : label is "soft_lutpair15";
  attribute SOFT_HLUTNM of \s_axi_rdata[28]_i_1\ : label is "soft_lutpair16";
  attribute SOFT_HLUTNM of \s_axi_rdata[29]_i_1\ : label is "soft_lutpair16";
  attribute SOFT_HLUTNM of \s_axi_rdata[30]_i_1\ : label is "soft_lutpair17";
  attribute SOFT_HLUTNM of \s_axi_rdata[31]_i_2\ : label is "soft_lutpair17";
begin
  SR(0) <= \^sr\(0);
  beat_detected_reg_0 <= \^beat_detected_reg_0\;
\FSM_sequential_current_state[0]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"0000FF7FFFFF0080"
    )
        port map (
      I0 => next_state1,
      I1 => current_state(1),
      I2 => red_filter_valid,
      I3 => \FSM_sequential_current_state[1]_i_3_n_0\,
      I4 => \FSM_sequential_current_state[1]_i_2_n_0\,
      I5 => current_state(0),
      O => \FSM_sequential_current_state[0]_i_1_n_0\
    );
\FSM_sequential_current_state[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"7577888877778888"
    )
        port map (
      I0 => current_state(0),
      I1 => \FSM_sequential_current_state[1]_i_2_n_0\,
      I2 => \FSM_sequential_current_state[1]_i_3_n_0\,
      I3 => red_filter_valid,
      I4 => current_state(1),
      I5 => next_state1,
      O => \FSM_sequential_current_state[1]_i_1_n_0\
    );
\FSM_sequential_current_state[1]_i_10\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[5]\,
      I1 => \refractory_cnt_reg_n_0_[4]\,
      I2 => \refractory_cnt_reg_n_0_[7]\,
      I3 => \refractory_cnt_reg_n_0_[6]\,
      O => \FSM_sequential_current_state[1]_i_10_n_0\
    );
\FSM_sequential_current_state[1]_i_11\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[29]\,
      I1 => \refractory_cnt_reg_n_0_[28]\,
      I2 => \refractory_cnt_reg_n_0_[31]\,
      I3 => \refractory_cnt_reg_n_0_[30]\,
      O => \FSM_sequential_current_state[1]_i_11_n_0\
    );
\FSM_sequential_current_state[1]_i_12\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[21]\,
      I1 => \refractory_cnt_reg_n_0_[20]\,
      I2 => \refractory_cnt_reg_n_0_[23]\,
      I3 => \refractory_cnt_reg_n_0_[22]\,
      O => \FSM_sequential_current_state[1]_i_12_n_0\
    );
\FSM_sequential_current_state[1]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"00008080FFFFCC00"
    )
        port map (
      I0 => next_state20_in,
      I1 => red_filter_valid,
      I2 => \FSM_sequential_current_state[1]_i_4_n_0\,
      I3 => next_state13_in,
      I4 => current_state(1),
      I5 => current_state(0),
      O => \FSM_sequential_current_state[1]_i_2_n_0\
    );
\FSM_sequential_current_state[1]_i_3\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => \FSM_sequential_current_state[1]_i_5_n_0\,
      I1 => \FSM_sequential_current_state[1]_i_6_n_0\,
      I2 => \FSM_sequential_current_state[1]_i_7_n_0\,
      I3 => \FSM_sequential_current_state[1]_i_8_n_0\,
      O => \FSM_sequential_current_state[1]_i_3_n_0\
    );
\FSM_sequential_current_state[1]_i_4\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"E"
    )
        port map (
      I0 => \fall_count_reg_n_0_[0]\,
      I1 => \fall_count_reg_n_0_[1]\,
      O => \FSM_sequential_current_state[1]_i_4_n_0\
    );
\FSM_sequential_current_state[1]_i_5\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FFFFFFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[10]\,
      I1 => \refractory_cnt_reg_n_0_[11]\,
      I2 => \refractory_cnt_reg_n_0_[8]\,
      I3 => \refractory_cnt_reg_n_0_[9]\,
      I4 => \FSM_sequential_current_state[1]_i_9_n_0\,
      O => \FSM_sequential_current_state[1]_i_5_n_0\
    );
\FSM_sequential_current_state[1]_i_6\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FFFFFFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[2]\,
      I1 => \refractory_cnt_reg_n_0_[3]\,
      I2 => \refractory_cnt_reg_n_0_[0]\,
      I3 => \refractory_cnt_reg_n_0_[1]\,
      I4 => \FSM_sequential_current_state[1]_i_10_n_0\,
      O => \FSM_sequential_current_state[1]_i_6_n_0\
    );
\FSM_sequential_current_state[1]_i_7\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FFFFFFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[26]\,
      I1 => \refractory_cnt_reg_n_0_[27]\,
      I2 => \refractory_cnt_reg_n_0_[24]\,
      I3 => \refractory_cnt_reg_n_0_[25]\,
      I4 => \FSM_sequential_current_state[1]_i_11_n_0\,
      O => \FSM_sequential_current_state[1]_i_7_n_0\
    );
\FSM_sequential_current_state[1]_i_8\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FFFFFFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[18]\,
      I1 => \refractory_cnt_reg_n_0_[19]\,
      I2 => \refractory_cnt_reg_n_0_[16]\,
      I3 => \refractory_cnt_reg_n_0_[17]\,
      I4 => \FSM_sequential_current_state[1]_i_12_n_0\,
      O => \FSM_sequential_current_state[1]_i_8_n_0\
    );
\FSM_sequential_current_state[1]_i_9\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[13]\,
      I1 => \refractory_cnt_reg_n_0_[12]\,
      I2 => \refractory_cnt_reg_n_0_[15]\,
      I3 => \refractory_cnt_reg_n_0_[14]\,
      O => \FSM_sequential_current_state[1]_i_9_n_0\
    );
\FSM_sequential_current_state_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_sequential_current_state[0]_i_1_n_0\,
      Q => current_state(0),
      R => \^sr\(0)
    );
\FSM_sequential_current_state_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_sequential_current_state[1]_i_1_n_0\,
      Q => current_state(1),
      R => \^sr\(0)
    );
beat_detected_i_1: unisim.vcomponents.LUT3
    generic map(
      INIT => X"08"
    )
        port map (
      I0 => first_beat_seen_reg_n_0,
      I1 => current_state(1),
      I2 => current_state(0),
      O => beat_detected_i_1_n_0
    );
beat_detected_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => beat_detected_i_1_n_0,
      Q => \^beat_detected_reg_0\,
      R => \^sr\(0)
    );
beat_flag_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"BFBF8F80"
    )
        port map (
      I0 => beat_flag_i_2_n_0,
      I1 => beat_flag_i_3_n_0,
      I2 => write_execute,
      I3 => \^beat_detected_reg_0\,
      I4 => data3(0),
      O => beat_detected_reg_1
    );
beat_flag_i_2: unisim.vcomponents.LUT5
    generic map(
      INIT => X"CCCC4CCC"
    )
        port map (
      I0 => beat_flag_reg(0),
      I1 => \^beat_detected_reg_0\,
      I2 => p_1_in_0(0),
      I3 => p_1_in_0(1),
      I4 => p_1_in_0(2),
      O => beat_flag_i_2_n_0
    );
beat_flag_i_3: unisim.vcomponents.LUT5
    generic map(
      INIT => X"CCCCECCC"
    )
        port map (
      I0 => beat_flag_reg(0),
      I1 => \^beat_detected_reg_0\,
      I2 => p_1_in_0(0),
      I3 => p_1_in_0(1),
      I4 => p_1_in_0(2),
      O => beat_flag_i_3_n_0
    );
\fall_count[0]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FF300080"
    )
        port map (
      I0 => next_state20_in,
      I1 => red_filter_valid,
      I2 => current_state(0),
      I3 => current_state(1),
      I4 => \fall_count_reg_n_0_[0]\,
      O => \fall_count[0]_i_1_n_0\
    );
\fall_count[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFF2F0000008000"
    )
        port map (
      I0 => next_state20_in,
      I1 => \fall_count_reg_n_0_[0]\,
      I2 => red_filter_valid,
      I3 => current_state(0),
      I4 => current_state(1),
      I5 => \fall_count_reg_n_0_[1]\,
      O => \fall_count[1]_i_1_n_0\
    );
\fall_count_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \fall_count[0]_i_1_n_0\,
      Q => \fall_count_reg_n_0_[0]\,
      R => \^sr\(0)
    );
\fall_count_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \fall_count[1]_i_1_n_0\,
      Q => \fall_count_reg_n_0_[1]\,
      R => \^sr\(0)
    );
first_beat_seen_i_1: unisim.vcomponents.LUT3
    generic map(
      INIT => X"F4"
    )
        port map (
      I0 => current_state(0),
      I1 => current_state(1),
      I2 => first_beat_seen_reg_n_0,
      O => first_beat_seen_i_1_n_0
    );
first_beat_seen_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => first_beat_seen_i_1_n_0,
      Q => first_beat_seen_reg_n_0,
      R => \^sr\(0)
    );
\ibi_cycles_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[0]\,
      Q => Q(0),
      R => \^sr\(0)
    );
\ibi_cycles_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[10]\,
      Q => ibi_cycles(10),
      R => \^sr\(0)
    );
\ibi_cycles_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[11]\,
      Q => ibi_cycles(11),
      R => \^sr\(0)
    );
\ibi_cycles_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[12]\,
      Q => ibi_cycles(12),
      R => \^sr\(0)
    );
\ibi_cycles_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[13]\,
      Q => ibi_cycles(13),
      R => \^sr\(0)
    );
\ibi_cycles_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[14]\,
      Q => ibi_cycles(14),
      R => \^sr\(0)
    );
\ibi_cycles_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[15]\,
      Q => ibi_cycles(15),
      R => \^sr\(0)
    );
\ibi_cycles_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[16]\,
      Q => ibi_cycles(16),
      R => \^sr\(0)
    );
\ibi_cycles_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[17]\,
      Q => ibi_cycles(17),
      R => \^sr\(0)
    );
\ibi_cycles_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[18]\,
      Q => ibi_cycles(18),
      R => \^sr\(0)
    );
\ibi_cycles_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[19]\,
      Q => ibi_cycles(19),
      R => \^sr\(0)
    );
\ibi_cycles_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[1]\,
      Q => Q(1),
      R => \^sr\(0)
    );
\ibi_cycles_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[20]\,
      Q => ibi_cycles(20),
      R => \^sr\(0)
    );
\ibi_cycles_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[21]\,
      Q => ibi_cycles(21),
      R => \^sr\(0)
    );
\ibi_cycles_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[22]\,
      Q => ibi_cycles(22),
      R => \^sr\(0)
    );
\ibi_cycles_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[23]\,
      Q => ibi_cycles(23),
      R => \^sr\(0)
    );
\ibi_cycles_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[24]\,
      Q => ibi_cycles(24),
      R => \^sr\(0)
    );
\ibi_cycles_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[25]\,
      Q => ibi_cycles(25),
      R => \^sr\(0)
    );
\ibi_cycles_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[26]\,
      Q => ibi_cycles(26),
      R => \^sr\(0)
    );
\ibi_cycles_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[27]\,
      Q => ibi_cycles(27),
      R => \^sr\(0)
    );
\ibi_cycles_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[28]\,
      Q => ibi_cycles(28),
      R => \^sr\(0)
    );
\ibi_cycles_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[29]\,
      Q => ibi_cycles(29),
      R => \^sr\(0)
    );
\ibi_cycles_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[2]\,
      Q => Q(2),
      R => \^sr\(0)
    );
\ibi_cycles_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[30]\,
      Q => ibi_cycles(30),
      R => \^sr\(0)
    );
\ibi_cycles_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[31]\,
      Q => ibi_cycles(31),
      R => \^sr\(0)
    );
\ibi_cycles_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[3]\,
      Q => Q(3),
      R => \^sr\(0)
    );
\ibi_cycles_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[4]\,
      Q => Q(4),
      R => \^sr\(0)
    );
\ibi_cycles_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[5]\,
      Q => Q(5),
      R => \^sr\(0)
    );
\ibi_cycles_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[6]\,
      Q => Q(6),
      R => \^sr\(0)
    );
\ibi_cycles_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[7]\,
      Q => Q(7),
      R => \^sr\(0)
    );
\ibi_cycles_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[8]\,
      Q => ibi_cycles(8),
      R => \^sr\(0)
    );
\ibi_cycles_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => beat_detected_i_1_n_0,
      D => \interval_cnt_reg_n_0_[9]\,
      Q => ibi_cycles(9),
      R => \^sr\(0)
    );
interval_cnt0_carry: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => interval_cnt0_carry_n_0,
      CO(2) => interval_cnt0_carry_n_1,
      CO(1) => interval_cnt0_carry_n_2,
      CO(0) => interval_cnt0_carry_n_3,
      CYINIT => \interval_cnt_reg_n_0_[0]\,
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(4 downto 1),
      S(3) => \interval_cnt_reg_n_0_[4]\,
      S(2) => \interval_cnt_reg_n_0_[3]\,
      S(1) => \interval_cnt_reg_n_0_[2]\,
      S(0) => \interval_cnt_reg_n_0_[1]\
    );
\interval_cnt0_carry__0\: unisim.vcomponents.CARRY4
     port map (
      CI => interval_cnt0_carry_n_0,
      CO(3) => \interval_cnt0_carry__0_n_0\,
      CO(2) => \interval_cnt0_carry__0_n_1\,
      CO(1) => \interval_cnt0_carry__0_n_2\,
      CO(0) => \interval_cnt0_carry__0_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(8 downto 5),
      S(3) => \interval_cnt_reg_n_0_[8]\,
      S(2) => \interval_cnt_reg_n_0_[7]\,
      S(1) => \interval_cnt_reg_n_0_[6]\,
      S(0) => \interval_cnt_reg_n_0_[5]\
    );
\interval_cnt0_carry__1\: unisim.vcomponents.CARRY4
     port map (
      CI => \interval_cnt0_carry__0_n_0\,
      CO(3) => \interval_cnt0_carry__1_n_0\,
      CO(2) => \interval_cnt0_carry__1_n_1\,
      CO(1) => \interval_cnt0_carry__1_n_2\,
      CO(0) => \interval_cnt0_carry__1_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(12 downto 9),
      S(3) => \interval_cnt_reg_n_0_[12]\,
      S(2) => \interval_cnt_reg_n_0_[11]\,
      S(1) => \interval_cnt_reg_n_0_[10]\,
      S(0) => \interval_cnt_reg_n_0_[9]\
    );
\interval_cnt0_carry__2\: unisim.vcomponents.CARRY4
     port map (
      CI => \interval_cnt0_carry__1_n_0\,
      CO(3) => \interval_cnt0_carry__2_n_0\,
      CO(2) => \interval_cnt0_carry__2_n_1\,
      CO(1) => \interval_cnt0_carry__2_n_2\,
      CO(0) => \interval_cnt0_carry__2_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(16 downto 13),
      S(3) => \interval_cnt_reg_n_0_[16]\,
      S(2) => \interval_cnt_reg_n_0_[15]\,
      S(1) => \interval_cnt_reg_n_0_[14]\,
      S(0) => \interval_cnt_reg_n_0_[13]\
    );
\interval_cnt0_carry__3\: unisim.vcomponents.CARRY4
     port map (
      CI => \interval_cnt0_carry__2_n_0\,
      CO(3) => \interval_cnt0_carry__3_n_0\,
      CO(2) => \interval_cnt0_carry__3_n_1\,
      CO(1) => \interval_cnt0_carry__3_n_2\,
      CO(0) => \interval_cnt0_carry__3_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(20 downto 17),
      S(3) => \interval_cnt_reg_n_0_[20]\,
      S(2) => \interval_cnt_reg_n_0_[19]\,
      S(1) => \interval_cnt_reg_n_0_[18]\,
      S(0) => \interval_cnt_reg_n_0_[17]\
    );
\interval_cnt0_carry__4\: unisim.vcomponents.CARRY4
     port map (
      CI => \interval_cnt0_carry__3_n_0\,
      CO(3) => \interval_cnt0_carry__4_n_0\,
      CO(2) => \interval_cnt0_carry__4_n_1\,
      CO(1) => \interval_cnt0_carry__4_n_2\,
      CO(0) => \interval_cnt0_carry__4_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(24 downto 21),
      S(3) => \interval_cnt_reg_n_0_[24]\,
      S(2) => \interval_cnt_reg_n_0_[23]\,
      S(1) => \interval_cnt_reg_n_0_[22]\,
      S(0) => \interval_cnt_reg_n_0_[21]\
    );
\interval_cnt0_carry__5\: unisim.vcomponents.CARRY4
     port map (
      CI => \interval_cnt0_carry__4_n_0\,
      CO(3) => \interval_cnt0_carry__5_n_0\,
      CO(2) => \interval_cnt0_carry__5_n_1\,
      CO(1) => \interval_cnt0_carry__5_n_2\,
      CO(0) => \interval_cnt0_carry__5_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3 downto 0) => in7(28 downto 25),
      S(3) => \interval_cnt_reg_n_0_[28]\,
      S(2) => \interval_cnt_reg_n_0_[27]\,
      S(1) => \interval_cnt_reg_n_0_[26]\,
      S(0) => \interval_cnt_reg_n_0_[25]\
    );
\interval_cnt0_carry__6\: unisim.vcomponents.CARRY4
     port map (
      CI => \interval_cnt0_carry__5_n_0\,
      CO(3 downto 2) => \NLW_interval_cnt0_carry__6_CO_UNCONNECTED\(3 downto 2),
      CO(1) => \interval_cnt0_carry__6_n_2\,
      CO(0) => \interval_cnt0_carry__6_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => B"0000",
      O(3) => \NLW_interval_cnt0_carry__6_O_UNCONNECTED\(3),
      O(2 downto 0) => in7(31 downto 29),
      S(3) => '0',
      S(2) => \interval_cnt_reg_n_0_[31]\,
      S(1) => \interval_cnt_reg_n_0_[30]\,
      S(0) => \interval_cnt_reg_n_0_[29]\
    );
\interval_cnt[0]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"0D"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => \interval_cnt_reg_n_0_[0]\,
      O => interval_cnt(0)
    );
\interval_cnt[10]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(10),
      O => interval_cnt(10)
    );
\interval_cnt[11]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(11),
      O => interval_cnt(11)
    );
\interval_cnt[12]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(12),
      O => interval_cnt(12)
    );
\interval_cnt[13]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(13),
      O => interval_cnt(13)
    );
\interval_cnt[14]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(14),
      O => interval_cnt(14)
    );
\interval_cnt[15]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(15),
      O => interval_cnt(15)
    );
\interval_cnt[16]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(16),
      O => interval_cnt(16)
    );
\interval_cnt[17]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(17),
      O => interval_cnt(17)
    );
\interval_cnt[18]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(18),
      O => interval_cnt(18)
    );
\interval_cnt[19]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(19),
      O => interval_cnt(19)
    );
\interval_cnt[1]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(1),
      O => interval_cnt(1)
    );
\interval_cnt[20]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(20),
      O => interval_cnt(20)
    );
\interval_cnt[21]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(21),
      O => interval_cnt(21)
    );
\interval_cnt[22]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(22),
      O => interval_cnt(22)
    );
\interval_cnt[23]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(23),
      O => interval_cnt(23)
    );
\interval_cnt[24]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(24),
      O => interval_cnt(24)
    );
\interval_cnt[25]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(25),
      O => interval_cnt(25)
    );
\interval_cnt[26]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(26),
      O => interval_cnt(26)
    );
\interval_cnt[27]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(27),
      O => interval_cnt(27)
    );
\interval_cnt[28]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(28),
      O => interval_cnt(28)
    );
\interval_cnt[29]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(29),
      O => interval_cnt(29)
    );
\interval_cnt[2]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(2),
      O => interval_cnt(2)
    );
\interval_cnt[30]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(30),
      O => interval_cnt(30)
    );
\interval_cnt[31]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFFFFFFFFFE"
    )
        port map (
      I0 => \interval_cnt[31]_i_3_n_0\,
      I1 => \interval_cnt[31]_i_4_n_0\,
      I2 => \interval_cnt[31]_i_5_n_0\,
      I3 => \interval_cnt[31]_i_6_n_0\,
      I4 => \interval_cnt[31]_i_7_n_0\,
      I5 => \interval_cnt[31]_i_8_n_0\,
      O => interval_cnt_1(0)
    );
\interval_cnt[31]_i_2\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(31),
      O => interval_cnt(31)
    );
\interval_cnt[31]_i_3\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"7FFFFFFFFFFFFFFF"
    )
        port map (
      I0 => \interval_cnt_reg_n_0_[12]\,
      I1 => \interval_cnt_reg_n_0_[13]\,
      I2 => \interval_cnt_reg_n_0_[10]\,
      I3 => \interval_cnt_reg_n_0_[11]\,
      I4 => \interval_cnt_reg_n_0_[9]\,
      I5 => \interval_cnt_reg_n_0_[8]\,
      O => \interval_cnt[31]_i_3_n_0\
    );
\interval_cnt[31]_i_4\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"7FFFFFFFFFFFFFFF"
    )
        port map (
      I0 => \interval_cnt_reg_n_0_[6]\,
      I1 => \interval_cnt_reg_n_0_[7]\,
      I2 => \interval_cnt_reg_n_0_[4]\,
      I3 => \interval_cnt_reg_n_0_[5]\,
      I4 => \interval_cnt_reg_n_0_[3]\,
      I5 => \interval_cnt_reg_n_0_[2]\,
      O => \interval_cnt[31]_i_4_n_0\
    );
\interval_cnt[31]_i_5\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"7FFFFFFFFFFFFFFF"
    )
        port map (
      I0 => \interval_cnt_reg_n_0_[24]\,
      I1 => \interval_cnt_reg_n_0_[25]\,
      I2 => \interval_cnt_reg_n_0_[22]\,
      I3 => \interval_cnt_reg_n_0_[23]\,
      I4 => \interval_cnt_reg_n_0_[21]\,
      I5 => \interval_cnt_reg_n_0_[20]\,
      O => \interval_cnt[31]_i_5_n_0\
    );
\interval_cnt[31]_i_6\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"7FFFFFFFFFFFFFFF"
    )
        port map (
      I0 => \interval_cnt_reg_n_0_[30]\,
      I1 => \interval_cnt_reg_n_0_[31]\,
      I2 => \interval_cnt_reg_n_0_[28]\,
      I3 => \interval_cnt_reg_n_0_[29]\,
      I4 => \interval_cnt_reg_n_0_[27]\,
      I5 => \interval_cnt_reg_n_0_[26]\,
      O => \interval_cnt[31]_i_6_n_0\
    );
\interval_cnt[31]_i_7\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"77F7"
    )
        port map (
      I0 => \interval_cnt_reg_n_0_[1]\,
      I1 => \interval_cnt_reg_n_0_[0]\,
      I2 => current_state(1),
      I3 => current_state(0),
      O => \interval_cnt[31]_i_7_n_0\
    );
\interval_cnt[31]_i_8\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"7FFFFFFFFFFFFFFF"
    )
        port map (
      I0 => \interval_cnt_reg_n_0_[18]\,
      I1 => \interval_cnt_reg_n_0_[19]\,
      I2 => \interval_cnt_reg_n_0_[16]\,
      I3 => \interval_cnt_reg_n_0_[17]\,
      I4 => \interval_cnt_reg_n_0_[15]\,
      I5 => \interval_cnt_reg_n_0_[14]\,
      O => \interval_cnt[31]_i_8_n_0\
    );
\interval_cnt[3]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(3),
      O => interval_cnt(3)
    );
\interval_cnt[4]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(4),
      O => interval_cnt(4)
    );
\interval_cnt[5]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(5),
      O => interval_cnt(5)
    );
\interval_cnt[6]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(6),
      O => interval_cnt(6)
    );
\interval_cnt[7]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(7),
      O => interval_cnt(7)
    );
\interval_cnt[8]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(8),
      O => interval_cnt(8)
    );
\interval_cnt[9]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"D0"
    )
        port map (
      I0 => current_state(1),
      I1 => current_state(0),
      I2 => in7(9),
      O => interval_cnt(9)
    );
\interval_cnt_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(0),
      Q => \interval_cnt_reg_n_0_[0]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(10),
      Q => \interval_cnt_reg_n_0_[10]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(11),
      Q => \interval_cnt_reg_n_0_[11]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(12),
      Q => \interval_cnt_reg_n_0_[12]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(13),
      Q => \interval_cnt_reg_n_0_[13]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(14),
      Q => \interval_cnt_reg_n_0_[14]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(15),
      Q => \interval_cnt_reg_n_0_[15]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(16),
      Q => \interval_cnt_reg_n_0_[16]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(17),
      Q => \interval_cnt_reg_n_0_[17]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(18),
      Q => \interval_cnt_reg_n_0_[18]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(19),
      Q => \interval_cnt_reg_n_0_[19]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(1),
      Q => \interval_cnt_reg_n_0_[1]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(20),
      Q => \interval_cnt_reg_n_0_[20]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(21),
      Q => \interval_cnt_reg_n_0_[21]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(22),
      Q => \interval_cnt_reg_n_0_[22]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(23),
      Q => \interval_cnt_reg_n_0_[23]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(24),
      Q => \interval_cnt_reg_n_0_[24]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(25),
      Q => \interval_cnt_reg_n_0_[25]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(26),
      Q => \interval_cnt_reg_n_0_[26]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(27),
      Q => \interval_cnt_reg_n_0_[27]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(28),
      Q => \interval_cnt_reg_n_0_[28]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(29),
      Q => \interval_cnt_reg_n_0_[29]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(2),
      Q => \interval_cnt_reg_n_0_[2]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(30),
      Q => \interval_cnt_reg_n_0_[30]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(31),
      Q => \interval_cnt_reg_n_0_[31]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(3),
      Q => \interval_cnt_reg_n_0_[3]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(4),
      Q => \interval_cnt_reg_n_0_[4]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(5),
      Q => \interval_cnt_reg_n_0_[5]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(6),
      Q => \interval_cnt_reg_n_0_[6]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(7),
      Q => \interval_cnt_reg_n_0_[7]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(8),
      Q => \interval_cnt_reg_n_0_[8]\,
      R => \^sr\(0)
    );
\interval_cnt_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => interval_cnt_1(0),
      D => interval_cnt(9),
      Q => \interval_cnt_reg_n_0_[9]\,
      R => \^sr\(0)
    );
next_state1_carry: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => next_state13_in,
      CO(2) => next_state1_carry_n_1,
      CO(1) => next_state1_carry_n_2,
      CO(0) => next_state1_carry_n_3,
      CYINIT => '1',
      DI(3 downto 0) => \FSM_sequential_current_state[1]_i_2_0\(3 downto 0),
      O(3 downto 0) => NLW_next_state1_carry_O_UNCONNECTED(3 downto 0),
      S(3 downto 0) => \FSM_sequential_current_state[1]_i_2_1\(3 downto 0)
    );
\next_state1_inferred__0/i__carry\: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => next_state1,
      CO(2) => \next_state1_inferred__0/i__carry_n_1\,
      CO(1) => \next_state1_inferred__0/i__carry_n_2\,
      CO(0) => \next_state1_inferred__0/i__carry_n_3\,
      CYINIT => '0',
      DI(3 downto 0) => DI(3 downto 0),
      O(3 downto 0) => \NLW_next_state1_inferred__0/i__carry_O_UNCONNECTED\(3 downto 0),
      S(3 downto 0) => S(3 downto 0)
    );
next_state2_carry: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => next_state20_in,
      CO(2) => next_state2_carry_n_1,
      CO(1) => next_state2_carry_n_2,
      CO(0) => next_state2_carry_n_3,
      CYINIT => '0',
      DI(3) => next_state2_carry_i_1_n_0,
      DI(2) => next_state2_carry_i_2_n_0,
      DI(1) => next_state2_carry_i_3_n_0,
      DI(0) => next_state2_carry_i_4_n_0,
      O(3 downto 0) => NLW_next_state2_carry_O_UNCONNECTED(3 downto 0),
      S(3) => next_state2_carry_i_5_n_0,
      S(2) => next_state2_carry_i_6_n_0,
      S(1) => next_state2_carry_i_7_n_0,
      S(0) => next_state2_carry_i_8_n_0
    );
next_state2_carry_i_1: unisim.vcomponents.LUT4
    generic map(
      INIT => X"22B2"
    )
        port map (
      I0 => prev_sample(7),
      I1 => D(7),
      I2 => prev_sample(6),
      I3 => D(6),
      O => next_state2_carry_i_1_n_0
    );
next_state2_carry_i_2: unisim.vcomponents.LUT4
    generic map(
      INIT => X"22B2"
    )
        port map (
      I0 => prev_sample(5),
      I1 => D(5),
      I2 => prev_sample(4),
      I3 => D(4),
      O => next_state2_carry_i_2_n_0
    );
next_state2_carry_i_3: unisim.vcomponents.LUT4
    generic map(
      INIT => X"22B2"
    )
        port map (
      I0 => prev_sample(3),
      I1 => D(3),
      I2 => prev_sample(2),
      I3 => D(2),
      O => next_state2_carry_i_3_n_0
    );
next_state2_carry_i_4: unisim.vcomponents.LUT4
    generic map(
      INIT => X"22B2"
    )
        port map (
      I0 => prev_sample(1),
      I1 => D(1),
      I2 => prev_sample(0),
      I3 => D(0),
      O => next_state2_carry_i_4_n_0
    );
next_state2_carry_i_5: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => prev_sample(7),
      I1 => D(7),
      I2 => prev_sample(6),
      I3 => D(6),
      O => next_state2_carry_i_5_n_0
    );
next_state2_carry_i_6: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => prev_sample(5),
      I1 => D(5),
      I2 => prev_sample(4),
      I3 => D(4),
      O => next_state2_carry_i_6_n_0
    );
next_state2_carry_i_7: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => prev_sample(3),
      I1 => D(3),
      I2 => prev_sample(2),
      I3 => D(2),
      O => next_state2_carry_i_7_n_0
    );
next_state2_carry_i_8: unisim.vcomponents.LUT4
    generic map(
      INIT => X"9009"
    )
        port map (
      I0 => prev_sample(1),
      I1 => D(1),
      I2 => prev_sample(0),
      I3 => D(0),
      O => next_state2_carry_i_8_n_0
    );
\prev_sample_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(0),
      Q => prev_sample(0),
      R => \^sr\(0)
    );
\prev_sample_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(1),
      Q => prev_sample(1),
      R => \^sr\(0)
    );
\prev_sample_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(2),
      Q => prev_sample(2),
      R => \^sr\(0)
    );
\prev_sample_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(3),
      Q => prev_sample(3),
      R => \^sr\(0)
    );
\prev_sample_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(4),
      Q => prev_sample(4),
      R => \^sr\(0)
    );
\prev_sample_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(5),
      Q => prev_sample(5),
      R => \^sr\(0)
    );
\prev_sample_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(6),
      Q => prev_sample(6),
      R => \^sr\(0)
    );
\prev_sample_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => red_filter_valid,
      D => D(7),
      Q => prev_sample(7),
      R => \^sr\(0)
    );
refractory_cnt0_carry: unisim.vcomponents.CARRY4
     port map (
      CI => '0',
      CO(3) => refractory_cnt0_carry_n_0,
      CO(2) => refractory_cnt0_carry_n_1,
      CO(1) => refractory_cnt0_carry_n_2,
      CO(0) => refractory_cnt0_carry_n_3,
      CYINIT => \refractory_cnt_reg_n_0_[0]\,
      DI(3) => \refractory_cnt_reg_n_0_[4]\,
      DI(2) => \refractory_cnt_reg_n_0_[3]\,
      DI(1) => \refractory_cnt_reg_n_0_[2]\,
      DI(0) => \refractory_cnt_reg_n_0_[1]\,
      O(3 downto 0) => in9(4 downto 1),
      S(3) => refractory_cnt0_carry_i_1_n_0,
      S(2) => refractory_cnt0_carry_i_2_n_0,
      S(1) => refractory_cnt0_carry_i_3_n_0,
      S(0) => refractory_cnt0_carry_i_4_n_0
    );
\refractory_cnt0_carry__0\: unisim.vcomponents.CARRY4
     port map (
      CI => refractory_cnt0_carry_n_0,
      CO(3) => \refractory_cnt0_carry__0_n_0\,
      CO(2) => \refractory_cnt0_carry__0_n_1\,
      CO(1) => \refractory_cnt0_carry__0_n_2\,
      CO(0) => \refractory_cnt0_carry__0_n_3\,
      CYINIT => '0',
      DI(3) => \refractory_cnt_reg_n_0_[8]\,
      DI(2) => \refractory_cnt_reg_n_0_[7]\,
      DI(1) => \refractory_cnt_reg_n_0_[6]\,
      DI(0) => \refractory_cnt_reg_n_0_[5]\,
      O(3 downto 0) => in9(8 downto 5),
      S(3) => \refractory_cnt0_carry__0_i_1_n_0\,
      S(2) => \refractory_cnt0_carry__0_i_2_n_0\,
      S(1) => \refractory_cnt0_carry__0_i_3_n_0\,
      S(0) => \refractory_cnt0_carry__0_i_4_n_0\
    );
\refractory_cnt0_carry__0_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[8]\,
      O => \refractory_cnt0_carry__0_i_1_n_0\
    );
\refractory_cnt0_carry__0_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[7]\,
      O => \refractory_cnt0_carry__0_i_2_n_0\
    );
\refractory_cnt0_carry__0_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[6]\,
      O => \refractory_cnt0_carry__0_i_3_n_0\
    );
\refractory_cnt0_carry__0_i_4\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[5]\,
      O => \refractory_cnt0_carry__0_i_4_n_0\
    );
\refractory_cnt0_carry__1\: unisim.vcomponents.CARRY4
     port map (
      CI => \refractory_cnt0_carry__0_n_0\,
      CO(3) => \refractory_cnt0_carry__1_n_0\,
      CO(2) => \refractory_cnt0_carry__1_n_1\,
      CO(1) => \refractory_cnt0_carry__1_n_2\,
      CO(0) => \refractory_cnt0_carry__1_n_3\,
      CYINIT => '0',
      DI(3) => \refractory_cnt_reg_n_0_[12]\,
      DI(2) => \refractory_cnt_reg_n_0_[11]\,
      DI(1) => \refractory_cnt_reg_n_0_[10]\,
      DI(0) => \refractory_cnt_reg_n_0_[9]\,
      O(3 downto 0) => in9(12 downto 9),
      S(3) => \refractory_cnt0_carry__1_i_1_n_0\,
      S(2) => \refractory_cnt0_carry__1_i_2_n_0\,
      S(1) => \refractory_cnt0_carry__1_i_3_n_0\,
      S(0) => \refractory_cnt0_carry__1_i_4_n_0\
    );
\refractory_cnt0_carry__1_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[12]\,
      O => \refractory_cnt0_carry__1_i_1_n_0\
    );
\refractory_cnt0_carry__1_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[11]\,
      O => \refractory_cnt0_carry__1_i_2_n_0\
    );
\refractory_cnt0_carry__1_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[10]\,
      O => \refractory_cnt0_carry__1_i_3_n_0\
    );
\refractory_cnt0_carry__1_i_4\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[9]\,
      O => \refractory_cnt0_carry__1_i_4_n_0\
    );
\refractory_cnt0_carry__2\: unisim.vcomponents.CARRY4
     port map (
      CI => \refractory_cnt0_carry__1_n_0\,
      CO(3) => \refractory_cnt0_carry__2_n_0\,
      CO(2) => \refractory_cnt0_carry__2_n_1\,
      CO(1) => \refractory_cnt0_carry__2_n_2\,
      CO(0) => \refractory_cnt0_carry__2_n_3\,
      CYINIT => '0',
      DI(3) => \refractory_cnt_reg_n_0_[16]\,
      DI(2) => \refractory_cnt_reg_n_0_[15]\,
      DI(1) => \refractory_cnt_reg_n_0_[14]\,
      DI(0) => \refractory_cnt_reg_n_0_[13]\,
      O(3 downto 0) => in9(16 downto 13),
      S(3) => \refractory_cnt0_carry__2_i_1_n_0\,
      S(2) => \refractory_cnt0_carry__2_i_2_n_0\,
      S(1) => \refractory_cnt0_carry__2_i_3_n_0\,
      S(0) => \refractory_cnt0_carry__2_i_4_n_0\
    );
\refractory_cnt0_carry__2_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[16]\,
      O => \refractory_cnt0_carry__2_i_1_n_0\
    );
\refractory_cnt0_carry__2_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[15]\,
      O => \refractory_cnt0_carry__2_i_2_n_0\
    );
\refractory_cnt0_carry__2_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[14]\,
      O => \refractory_cnt0_carry__2_i_3_n_0\
    );
\refractory_cnt0_carry__2_i_4\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[13]\,
      O => \refractory_cnt0_carry__2_i_4_n_0\
    );
\refractory_cnt0_carry__3\: unisim.vcomponents.CARRY4
     port map (
      CI => \refractory_cnt0_carry__2_n_0\,
      CO(3) => \refractory_cnt0_carry__3_n_0\,
      CO(2) => \refractory_cnt0_carry__3_n_1\,
      CO(1) => \refractory_cnt0_carry__3_n_2\,
      CO(0) => \refractory_cnt0_carry__3_n_3\,
      CYINIT => '0',
      DI(3) => \refractory_cnt_reg_n_0_[20]\,
      DI(2) => \refractory_cnt_reg_n_0_[19]\,
      DI(1) => \refractory_cnt_reg_n_0_[18]\,
      DI(0) => \refractory_cnt_reg_n_0_[17]\,
      O(3 downto 0) => in9(20 downto 17),
      S(3) => \refractory_cnt0_carry__3_i_1_n_0\,
      S(2) => \refractory_cnt0_carry__3_i_2_n_0\,
      S(1) => \refractory_cnt0_carry__3_i_3_n_0\,
      S(0) => \refractory_cnt0_carry__3_i_4_n_0\
    );
\refractory_cnt0_carry__3_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[20]\,
      O => \refractory_cnt0_carry__3_i_1_n_0\
    );
\refractory_cnt0_carry__3_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[19]\,
      O => \refractory_cnt0_carry__3_i_2_n_0\
    );
\refractory_cnt0_carry__3_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[18]\,
      O => \refractory_cnt0_carry__3_i_3_n_0\
    );
\refractory_cnt0_carry__3_i_4\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[17]\,
      O => \refractory_cnt0_carry__3_i_4_n_0\
    );
\refractory_cnt0_carry__4\: unisim.vcomponents.CARRY4
     port map (
      CI => \refractory_cnt0_carry__3_n_0\,
      CO(3) => \refractory_cnt0_carry__4_n_0\,
      CO(2) => \refractory_cnt0_carry__4_n_1\,
      CO(1) => \refractory_cnt0_carry__4_n_2\,
      CO(0) => \refractory_cnt0_carry__4_n_3\,
      CYINIT => '0',
      DI(3) => \refractory_cnt_reg_n_0_[24]\,
      DI(2) => \refractory_cnt_reg_n_0_[23]\,
      DI(1) => \refractory_cnt_reg_n_0_[22]\,
      DI(0) => \refractory_cnt_reg_n_0_[21]\,
      O(3 downto 0) => in9(24 downto 21),
      S(3) => \refractory_cnt0_carry__4_i_1_n_0\,
      S(2) => \refractory_cnt0_carry__4_i_2_n_0\,
      S(1) => \refractory_cnt0_carry__4_i_3_n_0\,
      S(0) => \refractory_cnt0_carry__4_i_4_n_0\
    );
\refractory_cnt0_carry__4_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[24]\,
      O => \refractory_cnt0_carry__4_i_1_n_0\
    );
\refractory_cnt0_carry__4_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[23]\,
      O => \refractory_cnt0_carry__4_i_2_n_0\
    );
\refractory_cnt0_carry__4_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[22]\,
      O => \refractory_cnt0_carry__4_i_3_n_0\
    );
\refractory_cnt0_carry__4_i_4\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[21]\,
      O => \refractory_cnt0_carry__4_i_4_n_0\
    );
\refractory_cnt0_carry__5\: unisim.vcomponents.CARRY4
     port map (
      CI => \refractory_cnt0_carry__4_n_0\,
      CO(3) => \refractory_cnt0_carry__5_n_0\,
      CO(2) => \refractory_cnt0_carry__5_n_1\,
      CO(1) => \refractory_cnt0_carry__5_n_2\,
      CO(0) => \refractory_cnt0_carry__5_n_3\,
      CYINIT => '0',
      DI(3) => \refractory_cnt_reg_n_0_[28]\,
      DI(2) => \refractory_cnt_reg_n_0_[27]\,
      DI(1) => \refractory_cnt_reg_n_0_[26]\,
      DI(0) => \refractory_cnt_reg_n_0_[25]\,
      O(3 downto 0) => in9(28 downto 25),
      S(3) => \refractory_cnt0_carry__5_i_1_n_0\,
      S(2) => \refractory_cnt0_carry__5_i_2_n_0\,
      S(1) => \refractory_cnt0_carry__5_i_3_n_0\,
      S(0) => \refractory_cnt0_carry__5_i_4_n_0\
    );
\refractory_cnt0_carry__5_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[28]\,
      O => \refractory_cnt0_carry__5_i_1_n_0\
    );
\refractory_cnt0_carry__5_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[27]\,
      O => \refractory_cnt0_carry__5_i_2_n_0\
    );
\refractory_cnt0_carry__5_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[26]\,
      O => \refractory_cnt0_carry__5_i_3_n_0\
    );
\refractory_cnt0_carry__5_i_4\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[25]\,
      O => \refractory_cnt0_carry__5_i_4_n_0\
    );
\refractory_cnt0_carry__6\: unisim.vcomponents.CARRY4
     port map (
      CI => \refractory_cnt0_carry__5_n_0\,
      CO(3 downto 2) => \NLW_refractory_cnt0_carry__6_CO_UNCONNECTED\(3 downto 2),
      CO(1) => \refractory_cnt0_carry__6_n_2\,
      CO(0) => \refractory_cnt0_carry__6_n_3\,
      CYINIT => '0',
      DI(3 downto 2) => B"00",
      DI(1) => \refractory_cnt_reg_n_0_[30]\,
      DI(0) => \refractory_cnt_reg_n_0_[29]\,
      O(3) => \NLW_refractory_cnt0_carry__6_O_UNCONNECTED\(3),
      O(2 downto 0) => in9(31 downto 29),
      S(3) => '0',
      S(2) => \refractory_cnt0_carry__6_i_1_n_0\,
      S(1) => \refractory_cnt0_carry__6_i_2_n_0\,
      S(0) => \refractory_cnt0_carry__6_i_3_n_0\
    );
\refractory_cnt0_carry__6_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[31]\,
      O => \refractory_cnt0_carry__6_i_1_n_0\
    );
\refractory_cnt0_carry__6_i_2\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[30]\,
      O => \refractory_cnt0_carry__6_i_2_n_0\
    );
\refractory_cnt0_carry__6_i_3\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[29]\,
      O => \refractory_cnt0_carry__6_i_3_n_0\
    );
refractory_cnt0_carry_i_1: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[4]\,
      O => refractory_cnt0_carry_i_1_n_0
    );
refractory_cnt0_carry_i_2: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[3]\,
      O => refractory_cnt0_carry_i_2_n_0
    );
refractory_cnt0_carry_i_3: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[2]\,
      O => refractory_cnt0_carry_i_3_n_0
    );
refractory_cnt0_carry_i_4: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \refractory_cnt_reg_n_0_[1]\,
      O => refractory_cnt0_carry_i_4_n_0
    );
\refractory_cnt[0]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"2"
    )
        port map (
      I0 => current_state(0),
      I1 => \refractory_cnt_reg_n_0_[0]\,
      O => \refractory_cnt[0]_i_1_n_0\
    );
\refractory_cnt[10]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(10),
      I1 => current_state(0),
      O => \refractory_cnt[10]_i_1_n_0\
    );
\refractory_cnt[11]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(11),
      I1 => current_state(0),
      O => \refractory_cnt[11]_i_1_n_0\
    );
\refractory_cnt[12]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(12),
      I1 => current_state(0),
      O => \refractory_cnt[12]_i_1_n_0\
    );
\refractory_cnt[13]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(13),
      I1 => current_state(0),
      O => \refractory_cnt[13]_i_1_n_0\
    );
\refractory_cnt[14]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(14),
      O => refractory_cnt(14)
    );
\refractory_cnt[15]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(15),
      I1 => current_state(0),
      O => \refractory_cnt[15]_i_1_n_0\
    );
\refractory_cnt[16]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(16),
      O => refractory_cnt(16)
    );
\refractory_cnt[17]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(17),
      I1 => current_state(0),
      O => \refractory_cnt[17]_i_1_n_0\
    );
\refractory_cnt[18]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(18),
      I1 => current_state(0),
      O => \refractory_cnt[18]_i_1_n_0\
    );
\refractory_cnt[19]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(19),
      I1 => current_state(0),
      O => \refractory_cnt[19]_i_1_n_0\
    );
\refractory_cnt[1]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(1),
      O => refractory_cnt(1)
    );
\refractory_cnt[20]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(20),
      I1 => current_state(0),
      O => \refractory_cnt[20]_i_1_n_0\
    );
\refractory_cnt[21]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(21),
      I1 => current_state(0),
      O => \refractory_cnt[21]_i_1_n_0\
    );
\refractory_cnt[22]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(22),
      O => refractory_cnt(22)
    );
\refractory_cnt[23]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(23),
      I1 => current_state(0),
      O => \refractory_cnt[23]_i_1_n_0\
    );
\refractory_cnt[24]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(24),
      O => refractory_cnt(24)
    );
\refractory_cnt[25]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(25),
      O => refractory_cnt(25)
    );
\refractory_cnt[26]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(26),
      O => refractory_cnt(26)
    );
\refractory_cnt[27]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(27),
      O => refractory_cnt(27)
    );
\refractory_cnt[28]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(28),
      O => refractory_cnt(28)
    );
\refractory_cnt[29]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(29),
      O => refractory_cnt(29)
    );
\refractory_cnt[2]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(2),
      O => refractory_cnt(2)
    );
\refractory_cnt[30]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(30),
      O => refractory_cnt(30)
    );
\refractory_cnt[31]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8C"
    )
        port map (
      I0 => \FSM_sequential_current_state[1]_i_3_n_0\,
      I1 => current_state(1),
      I2 => current_state(0),
      O => refractory_cnt_0(0)
    );
\refractory_cnt[31]_i_2\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(31),
      O => refractory_cnt(31)
    );
\refractory_cnt[3]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(3),
      O => refractory_cnt(3)
    );
\refractory_cnt[4]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(4),
      O => refractory_cnt(4)
    );
\refractory_cnt[5]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"B"
    )
        port map (
      I0 => in9(5),
      I1 => current_state(0),
      O => \refractory_cnt[5]_i_1_n_0\
    );
\refractory_cnt[6]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(6),
      O => refractory_cnt(6)
    );
\refractory_cnt[7]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(7),
      O => refractory_cnt(7)
    );
\refractory_cnt[8]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(8),
      O => refractory_cnt(8)
    );
\refractory_cnt[9]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => current_state(0),
      I1 => in9(9),
      O => refractory_cnt(9)
    );
\refractory_cnt_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[0]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[0]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[10]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[10]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[11]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[11]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[12]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[12]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[13]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[13]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(14),
      Q => \refractory_cnt_reg_n_0_[14]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[15]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[15]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(16),
      Q => \refractory_cnt_reg_n_0_[16]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[17]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[17]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[18]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[18]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[19]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[19]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(1),
      Q => \refractory_cnt_reg_n_0_[1]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[20]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[20]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[21]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[21]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(22),
      Q => \refractory_cnt_reg_n_0_[22]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[23]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[23]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(24),
      Q => \refractory_cnt_reg_n_0_[24]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(25),
      Q => \refractory_cnt_reg_n_0_[25]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(26),
      Q => \refractory_cnt_reg_n_0_[26]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(27),
      Q => \refractory_cnt_reg_n_0_[27]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(28),
      Q => \refractory_cnt_reg_n_0_[28]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(29),
      Q => \refractory_cnt_reg_n_0_[29]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(2),
      Q => \refractory_cnt_reg_n_0_[2]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(30),
      Q => \refractory_cnt_reg_n_0_[30]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(31),
      Q => \refractory_cnt_reg_n_0_[31]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(3),
      Q => \refractory_cnt_reg_n_0_[3]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(4),
      Q => \refractory_cnt_reg_n_0_[4]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => \refractory_cnt[5]_i_1_n_0\,
      Q => \refractory_cnt_reg_n_0_[5]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(6),
      Q => \refractory_cnt_reg_n_0_[6]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(7),
      Q => \refractory_cnt_reg_n_0_[7]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(8),
      Q => \refractory_cnt_reg_n_0_[8]\,
      R => \^sr\(0)
    );
\refractory_cnt_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => refractory_cnt_0(0),
      D => refractory_cnt(9),
      Q => \refractory_cnt_reg_n_0_[9]\,
      R => \^sr\(0)
    );
s_axi_awready_i_1: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => s_axi_aresetn,
      O => \^sr\(0)
    );
\s_axi_rdata[10]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(10),
      I1 => data3(3),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(2)
    );
\s_axi_rdata[11]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(11),
      I1 => data3(4),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(3)
    );
\s_axi_rdata[12]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(12),
      I1 => data3(5),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(4)
    );
\s_axi_rdata[13]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(13),
      I1 => data3(6),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(5)
    );
\s_axi_rdata[14]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(14),
      I1 => data3(7),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(6)
    );
\s_axi_rdata[15]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(15),
      I1 => data3(8),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(7)
    );
\s_axi_rdata[16]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(16),
      O => \axi_araddr_latched_reg[2]\(8)
    );
\s_axi_rdata[17]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(17),
      O => \axi_araddr_latched_reg[2]\(9)
    );
\s_axi_rdata[18]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(18),
      O => \axi_araddr_latched_reg[2]\(10)
    );
\s_axi_rdata[19]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(19),
      O => \axi_araddr_latched_reg[2]\(11)
    );
\s_axi_rdata[20]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(20),
      O => \axi_araddr_latched_reg[2]\(12)
    );
\s_axi_rdata[21]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(21),
      O => \axi_araddr_latched_reg[2]\(13)
    );
\s_axi_rdata[22]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(22),
      O => \axi_araddr_latched_reg[2]\(14)
    );
\s_axi_rdata[23]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(23),
      O => \axi_araddr_latched_reg[2]\(15)
    );
\s_axi_rdata[24]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(24),
      O => \axi_araddr_latched_reg[2]\(16)
    );
\s_axi_rdata[25]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(25),
      O => \axi_araddr_latched_reg[2]\(17)
    );
\s_axi_rdata[26]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(26),
      O => \axi_araddr_latched_reg[2]\(18)
    );
\s_axi_rdata[27]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(27),
      O => \axi_araddr_latched_reg[2]\(19)
    );
\s_axi_rdata[28]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(28),
      O => \axi_araddr_latched_reg[2]\(20)
    );
\s_axi_rdata[29]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(29),
      O => \axi_araddr_latched_reg[2]\(21)
    );
\s_axi_rdata[30]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(30),
      O => \axi_araddr_latched_reg[2]\(22)
    );
\s_axi_rdata[31]_i_2\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"0400"
    )
        port map (
      I0 => sel0(0),
      I1 => sel0(1),
      I2 => sel0(2),
      I3 => ibi_cycles(31),
      O => \axi_araddr_latched_reg[2]\(23)
    );
\s_axi_rdata[8]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(8),
      I1 => data3(1),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(0)
    );
\s_axi_rdata[9]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0C000A00"
    )
        port map (
      I0 => ibi_cycles(9),
      I1 => data3(2),
      I2 => sel0(2),
      I3 => sel0(1),
      I4 => sel0(0),
      O => \axi_araddr_latched_reg[2]\(1)
    );
end STRUCTURE;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator is
  port (
    irq_beat : out STD_LOGIC;
    s_axi_awready : out STD_LOGIC;
    s_axi_wready : out STD_LOGIC;
    s_axi_arready : out STD_LOGIC;
    s_axi_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_bvalid_reg_0 : out STD_LOGIC;
    s_axi_rvalid : out STD_LOGIC;
    s_axi_aclk : in STD_LOGIC;
    s_axi_wdata : in STD_LOGIC_VECTOR ( 15 downto 0 );
    s_axi_awaddr : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_awvalid : in STD_LOGIC;
    s_axi_araddr : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_arvalid : in STD_LOGIC;
    s_axi_aresetn : in STD_LOGIC;
    s_axi_bready : in STD_LOGIC;
    s_axi_wvalid : in STD_LOGIC;
    s_axi_rready : in STD_LOGIC
  );
  attribute ORIG_REF_NAME : string;
  attribute ORIG_REF_NAME of design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator : entity is "axi_ppg_accelerator";
end design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator;

architecture STRUCTURE of design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator is
  signal \aw_addr_latched[2]_i_1_n_0\ : STD_LOGIC;
  signal \aw_addr_latched[3]_i_1_n_0\ : STD_LOGIC;
  signal \aw_addr_latched[4]_i_1_n_0\ : STD_LOGIC;
  signal aw_done : STD_LOGIC;
  signal aw_done_i_1_n_0 : STD_LOGIC;
  signal \axi_araddr_latched[2]_i_1_n_0\ : STD_LOGIC;
  signal \axi_araddr_latched[3]_i_1_n_0\ : STD_LOGIC;
  signal \axi_araddr_latched[4]_i_1_n_0\ : STD_LOGIC;
  signal data3 : STD_LOGIC_VECTOR ( 15 downto 0 );
  signal data_out : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal ibi_cycles : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal ir_sample_valid : STD_LOGIC;
  signal ir_sample_valid_i_1_n_0 : STD_LOGIC;
  signal p_1_in_0 : STD_LOGIC_VECTOR ( 2 downto 0 );
  signal red_filter_valid : STD_LOGIC;
  signal red_sample_valid : STD_LOGIC;
  signal red_sample_valid_i_1_n_0 : STD_LOGIC;
  signal reg_ir_raw : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal reg_ir_raw_3 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal reg_red_raw : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal reg_red_raw_2 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal reg_threshold : STD_LOGIC_VECTOR ( 0 to 0 );
  signal \^s_axi_arready\ : STD_LOGIC;
  signal s_axi_arready0 : STD_LOGIC;
  signal s_axi_awready0 : STD_LOGIC;
  signal s_axi_bvalid_i_1_n_0 : STD_LOGIC;
  signal \^s_axi_bvalid_reg_0\ : STD_LOGIC;
  signal s_axi_rdata_1 : STD_LOGIC_VECTOR ( 31 downto 0 );
  signal \^s_axi_rvalid\ : STD_LOGIC;
  signal s_axi_rvalid01_out : STD_LOGIC;
  signal s_axi_rvalid_i_1_n_0 : STD_LOGIC;
  signal s_axi_wready0 : STD_LOGIC;
  signal sel0 : STD_LOGIC_VECTOR ( 2 downto 0 );
  signal u_filter_ir_n_0 : STD_LOGIC;
  signal u_filter_ir_n_1 : STD_LOGIC;
  signal u_filter_ir_n_2 : STD_LOGIC;
  signal u_filter_ir_n_3 : STD_LOGIC;
  signal u_filter_ir_n_4 : STD_LOGIC;
  signal u_filter_ir_n_5 : STD_LOGIC;
  signal u_filter_ir_n_6 : STD_LOGIC;
  signal u_filter_red_n_10 : STD_LOGIC;
  signal u_filter_red_n_11 : STD_LOGIC;
  signal u_filter_red_n_12 : STD_LOGIC;
  signal u_filter_red_n_13 : STD_LOGIC;
  signal u_filter_red_n_14 : STD_LOGIC;
  signal u_filter_red_n_15 : STD_LOGIC;
  signal u_filter_red_n_16 : STD_LOGIC;
  signal u_filter_red_n_17 : STD_LOGIC;
  signal u_filter_red_n_18 : STD_LOGIC;
  signal u_filter_red_n_19 : STD_LOGIC;
  signal u_filter_red_n_20 : STD_LOGIC;
  signal u_filter_red_n_21 : STD_LOGIC;
  signal u_filter_red_n_29 : STD_LOGIC;
  signal u_filter_red_n_30 : STD_LOGIC;
  signal u_filter_red_n_31 : STD_LOGIC;
  signal u_filter_red_n_32 : STD_LOGIC;
  signal u_filter_red_n_9 : STD_LOGIC;
  signal u_peak_det_n_1 : STD_LOGIC;
  signal u_peak_det_n_34 : STD_LOGIC;
  signal w_data_latched : STD_LOGIC_VECTOR ( 0 to 0 );
  signal \w_data_latched_reg_n_0_[10]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[11]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[12]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[13]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[14]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[15]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[1]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[2]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[3]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[4]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[5]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[6]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[7]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[8]\ : STD_LOGIC;
  signal \w_data_latched_reg_n_0_[9]\ : STD_LOGIC;
  signal w_done_i_1_n_0 : STD_LOGIC;
  signal w_done_reg_n_0 : STD_LOGIC;
  signal write_execute : STD_LOGIC;
  attribute SOFT_HLUTNM : string;
  attribute SOFT_HLUTNM of \aw_addr_latched[4]_i_1\ : label is "soft_lutpair51";
  attribute SOFT_HLUTNM of \axi_araddr_latched[4]_i_1\ : label is "soft_lutpair52";
  attribute SOFT_HLUTNM of ir_sample_valid_i_1 : label is "soft_lutpair50";
  attribute SOFT_HLUTNM of red_sample_valid_i_1 : label is "soft_lutpair50";
  attribute SOFT_HLUTNM of red_sample_valid_i_2 : label is "soft_lutpair49";
  attribute SOFT_HLUTNM of s_axi_arready_i_1 : label is "soft_lutpair52";
  attribute SOFT_HLUTNM of s_axi_awready_i_2 : label is "soft_lutpair51";
  attribute SOFT_HLUTNM of s_axi_bvalid_i_1 : label is "soft_lutpair49";
begin
  s_axi_arready <= \^s_axi_arready\;
  s_axi_bvalid_reg_0 <= \^s_axi_bvalid_reg_0\;
  s_axi_rvalid <= \^s_axi_rvalid\;
\aw_addr_latched[2]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FB08"
    )
        port map (
      I0 => s_axi_awaddr(0),
      I1 => s_axi_awvalid,
      I2 => aw_done,
      I3 => p_1_in_0(0),
      O => \aw_addr_latched[2]_i_1_n_0\
    );
\aw_addr_latched[3]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FB08"
    )
        port map (
      I0 => s_axi_awaddr(1),
      I1 => s_axi_awvalid,
      I2 => aw_done,
      I3 => p_1_in_0(1),
      O => \aw_addr_latched[3]_i_1_n_0\
    );
\aw_addr_latched[4]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FB08"
    )
        port map (
      I0 => s_axi_awaddr(2),
      I1 => s_axi_awvalid,
      I2 => aw_done,
      I3 => p_1_in_0(2),
      O => \aw_addr_latched[4]_i_1_n_0\
    );
\aw_addr_latched_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \aw_addr_latched[2]_i_1_n_0\,
      Q => p_1_in_0(0),
      R => u_peak_det_n_1
    );
\aw_addr_latched_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \aw_addr_latched[3]_i_1_n_0\,
      Q => p_1_in_0(1),
      R => u_peak_det_n_1
    );
\aw_addr_latched_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \aw_addr_latched[4]_i_1_n_0\,
      Q => p_1_in_0(2),
      R => u_peak_det_n_1
    );
aw_done_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"C808C8C8"
    )
        port map (
      I0 => s_axi_awvalid,
      I1 => s_axi_aresetn,
      I2 => aw_done,
      I3 => \^s_axi_bvalid_reg_0\,
      I4 => w_done_reg_n_0,
      O => aw_done_i_1_n_0
    );
aw_done_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => aw_done_i_1_n_0,
      Q => aw_done,
      R => '0'
    );
\axi_araddr_latched[2]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FB08"
    )
        port map (
      I0 => s_axi_araddr(0),
      I1 => s_axi_arvalid,
      I2 => \^s_axi_arready\,
      I3 => sel0(0),
      O => \axi_araddr_latched[2]_i_1_n_0\
    );
\axi_araddr_latched[3]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FB08"
    )
        port map (
      I0 => s_axi_araddr(1),
      I1 => s_axi_arvalid,
      I2 => \^s_axi_arready\,
      I3 => sel0(1),
      O => \axi_araddr_latched[3]_i_1_n_0\
    );
\axi_araddr_latched[4]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FB08"
    )
        port map (
      I0 => s_axi_araddr(2),
      I1 => s_axi_arvalid,
      I2 => \^s_axi_arready\,
      I3 => sel0(2),
      O => \axi_araddr_latched[4]_i_1_n_0\
    );
\axi_araddr_latched_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \axi_araddr_latched[2]_i_1_n_0\,
      Q => sel0(0),
      R => u_peak_det_n_1
    );
\axi_araddr_latched_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \axi_araddr_latched[3]_i_1_n_0\,
      Q => sel0(1),
      R => u_peak_det_n_1
    );
\axi_araddr_latched_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \axi_araddr_latched[4]_i_1_n_0\,
      Q => sel0(2),
      R => u_peak_det_n_1
    );
beat_flag_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => u_peak_det_n_34,
      Q => data3(0),
      R => u_peak_det_n_1
    );
ir_sample_valid_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"10000000"
    )
        port map (
      I0 => p_1_in_0(1),
      I1 => p_1_in_0(0),
      I2 => p_1_in_0(2),
      I3 => write_execute,
      I4 => s_axi_aresetn,
      O => ir_sample_valid_i_1_n_0
    );
ir_sample_valid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => ir_sample_valid_i_1_n_0,
      Q => ir_sample_valid,
      R => '0'
    );
red_sample_valid_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"01000000"
    )
        port map (
      I0 => p_1_in_0(2),
      I1 => p_1_in_0(0),
      I2 => p_1_in_0(1),
      I3 => write_execute,
      I4 => s_axi_aresetn,
      O => red_sample_valid_i_1_n_0
    );
red_sample_valid_i_2: unisim.vcomponents.LUT3
    generic map(
      INIT => X"20"
    )
        port map (
      I0 => aw_done,
      I1 => \^s_axi_bvalid_reg_0\,
      I2 => w_done_reg_n_0,
      O => write_execute
    );
red_sample_valid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => red_sample_valid_i_1_n_0,
      Q => red_sample_valid,
      R => '0'
    );
\reg_ir_raw[7]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"0000002000000000"
    )
        port map (
      I0 => w_done_reg_n_0,
      I1 => \^s_axi_bvalid_reg_0\,
      I2 => aw_done,
      I3 => p_1_in_0(1),
      I4 => p_1_in_0(0),
      I5 => p_1_in_0(2),
      O => reg_ir_raw_3(0)
    );
\reg_ir_raw_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => w_data_latched(0),
      Q => reg_ir_raw(0),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[1]\,
      Q => reg_ir_raw(1),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[2]\,
      Q => reg_ir_raw(2),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[3]\,
      Q => reg_ir_raw(3),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[4]\,
      Q => reg_ir_raw(4),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[5]\,
      Q => reg_ir_raw(5),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[6]\,
      Q => reg_ir_raw(6),
      R => u_peak_det_n_1
    );
\reg_ir_raw_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_ir_raw_3(0),
      D => \w_data_latched_reg_n_0_[7]\,
      Q => reg_ir_raw(7),
      R => u_peak_det_n_1
    );
\reg_red_raw[7]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"0000000000000020"
    )
        port map (
      I0 => w_done_reg_n_0,
      I1 => \^s_axi_bvalid_reg_0\,
      I2 => aw_done,
      I3 => p_1_in_0(2),
      I4 => p_1_in_0(0),
      I5 => p_1_in_0(1),
      O => reg_red_raw_2(0)
    );
\reg_red_raw_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => w_data_latched(0),
      Q => reg_red_raw(0),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[1]\,
      Q => reg_red_raw(1),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[2]\,
      Q => reg_red_raw(2),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[3]\,
      Q => reg_red_raw(3),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[4]\,
      Q => reg_red_raw(4),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[5]\,
      Q => reg_red_raw(5),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[6]\,
      Q => reg_red_raw(6),
      R => u_peak_det_n_1
    );
\reg_red_raw_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_red_raw_2(0),
      D => \w_data_latched_reg_n_0_[7]\,
      Q => reg_red_raw(7),
      R => u_peak_det_n_1
    );
\reg_threshold[7]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"0000000020000000"
    )
        port map (
      I0 => w_done_reg_n_0,
      I1 => \^s_axi_bvalid_reg_0\,
      I2 => aw_done,
      I3 => p_1_in_0(1),
      I4 => p_1_in_0(0),
      I5 => p_1_in_0(2),
      O => reg_threshold(0)
    );
\reg_threshold_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[8]\,
      Q => data3(8),
      R => u_peak_det_n_1
    );
\reg_threshold_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[9]\,
      Q => data3(9),
      R => u_peak_det_n_1
    );
\reg_threshold_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[10]\,
      Q => data3(10),
      R => u_peak_det_n_1
    );
\reg_threshold_reg[3]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[11]\,
      Q => data3(11),
      S => u_peak_det_n_1
    );
\reg_threshold_reg[4]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[12]\,
      Q => data3(12),
      S => u_peak_det_n_1
    );
\reg_threshold_reg[5]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[13]\,
      Q => data3(13),
      S => u_peak_det_n_1
    );
\reg_threshold_reg[6]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[14]\,
      Q => data3(14),
      S => u_peak_det_n_1
    );
\reg_threshold_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => reg_threshold(0),
      D => \w_data_latched_reg_n_0_[15]\,
      Q => data3(15),
      R => u_peak_det_n_1
    );
s_axi_arready_i_1: unisim.vcomponents.LUT2
    generic map(
      INIT => X"2"
    )
        port map (
      I0 => s_axi_arvalid,
      I1 => \^s_axi_arready\,
      O => s_axi_arready0
    );
s_axi_arready_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_arready0,
      Q => \^s_axi_arready\,
      R => u_peak_det_n_1
    );
s_axi_awready_i_2: unisim.vcomponents.LUT2
    generic map(
      INIT => X"2"
    )
        port map (
      I0 => s_axi_awvalid,
      I1 => aw_done,
      O => s_axi_awready0
    );
s_axi_awready_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_awready0,
      Q => s_axi_awready,
      R => u_peak_det_n_1
    );
s_axi_bvalid_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0080F080"
    )
        port map (
      I0 => aw_done,
      I1 => w_done_reg_n_0,
      I2 => s_axi_aresetn,
      I3 => \^s_axi_bvalid_reg_0\,
      I4 => s_axi_bready,
      O => s_axi_bvalid_i_1_n_0
    );
s_axi_bvalid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_bvalid_i_1_n_0,
      Q => \^s_axi_bvalid_reg_0\,
      R => '0'
    );
\s_axi_rdata[31]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"2"
    )
        port map (
      I0 => \^s_axi_arready\,
      I1 => \^s_axi_rvalid\,
      O => s_axi_rvalid01_out
    );
\s_axi_rdata_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(0),
      Q => s_axi_rdata(0),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(10),
      Q => s_axi_rdata(10),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(11),
      Q => s_axi_rdata(11),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(12),
      Q => s_axi_rdata(12),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(13),
      Q => s_axi_rdata(13),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(14),
      Q => s_axi_rdata(14),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(15),
      Q => s_axi_rdata(15),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(16),
      Q => s_axi_rdata(16),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(17),
      Q => s_axi_rdata(17),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(18),
      Q => s_axi_rdata(18),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(19),
      Q => s_axi_rdata(19),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(1),
      Q => s_axi_rdata(1),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(20),
      Q => s_axi_rdata(20),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(21),
      Q => s_axi_rdata(21),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(22),
      Q => s_axi_rdata(22),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(23),
      Q => s_axi_rdata(23),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(24),
      Q => s_axi_rdata(24),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(25),
      Q => s_axi_rdata(25),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(26),
      Q => s_axi_rdata(26),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(27),
      Q => s_axi_rdata(27),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(28),
      Q => s_axi_rdata(28),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(29),
      Q => s_axi_rdata(29),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(2),
      Q => s_axi_rdata(2),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(30),
      Q => s_axi_rdata(30),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(31),
      Q => s_axi_rdata(31),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(3),
      Q => s_axi_rdata(3),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(4),
      Q => s_axi_rdata(4),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(5),
      Q => s_axi_rdata(5),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(6),
      Q => s_axi_rdata(6),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(7),
      Q => s_axi_rdata(7),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(8),
      Q => s_axi_rdata(8),
      R => u_peak_det_n_1
    );
\s_axi_rdata_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_rvalid01_out,
      D => s_axi_rdata_1(9),
      Q => s_axi_rdata(9),
      R => u_peak_det_n_1
    );
s_axi_rvalid_i_1: unisim.vcomponents.LUT3
    generic map(
      INIT => X"3A"
    )
        port map (
      I0 => \^s_axi_arready\,
      I1 => s_axi_rready,
      I2 => \^s_axi_rvalid\,
      O => s_axi_rvalid_i_1_n_0
    );
s_axi_rvalid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_rvalid_i_1_n_0,
      Q => \^s_axi_rvalid\,
      R => u_peak_det_n_1
    );
s_axi_wready_i_1: unisim.vcomponents.LUT2
    generic map(
      INIT => X"2"
    )
        port map (
      I0 => s_axi_wvalid,
      I1 => w_done_reg_n_0,
      O => s_axi_wready0
    );
s_axi_wready_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_wready0,
      Q => s_axi_wready,
      R => u_peak_det_n_1
    );
u_filter_ir: entity work.design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap
     port map (
      D(0) => s_axi_rdata_1(0),
      Q(7 downto 0) => reg_ir_raw(7 downto 0),
      \data_out_reg[1]_0\ => u_filter_ir_n_1,
      \data_out_reg[2]_0\ => u_filter_ir_n_2,
      \data_out_reg[3]_0\ => u_filter_ir_n_3,
      \data_out_reg[4]_0\ => u_filter_ir_n_4,
      \data_out_reg[5]_0\ => u_filter_ir_n_5,
      \data_out_reg[6]_0\ => u_filter_ir_n_6,
      \data_out_reg[7]_0\ => u_filter_ir_n_0,
      ir_sample_valid => ir_sample_valid,
      \running_sum_reg[0]_0\ => u_peak_det_n_1,
      s_axi_aclk => s_axi_aclk,
      \s_axi_rdata_reg[0]\ => u_filter_red_n_21,
      \s_axi_rdata_reg[0]_0\(0) => ibi_cycles(0),
      sel0(2 downto 0) => sel0(2 downto 0)
    );
u_filter_red: entity work.design_ZYNQ_axi_ppg_accelerator_0_0_moving_average_8tap_0
     port map (
      D(6 downto 0) => s_axi_rdata_1(7 downto 1),
      DI(3) => u_filter_red_n_9,
      DI(2) => u_filter_red_n_10,
      DI(1) => u_filter_red_n_11,
      DI(0) => u_filter_red_n_12,
      Q(7 downto 0) => reg_red_raw(7 downto 0),
      S(3) => u_filter_red_n_17,
      S(2) => u_filter_red_n_18,
      S(1) => u_filter_red_n_19,
      S(0) => u_filter_red_n_20,
      data3(8 downto 1) => data3(15 downto 8),
      data3(0) => data3(0),
      data_out(7 downto 0) => data_out(7 downto 0),
      \data_out_reg[0]_0\ => u_peak_det_n_1,
      \data_out_reg[6]_0\(3) => u_filter_red_n_13,
      \data_out_reg[6]_0\(2) => u_filter_red_n_14,
      \data_out_reg[6]_0\(1) => u_filter_red_n_15,
      \data_out_reg[6]_0\(0) => u_filter_red_n_16,
      \data_out_reg[6]_1\(3) => u_filter_red_n_29,
      \data_out_reg[6]_1\(2) => u_filter_red_n_30,
      \data_out_reg[6]_1\(1) => u_filter_red_n_31,
      \data_out_reg[6]_1\(0) => u_filter_red_n_32,
      red_filter_valid => red_filter_valid,
      red_sample_valid => red_sample_valid,
      \reg_red_raw_reg[0]\ => u_filter_red_n_21,
      s_axi_aclk => s_axi_aclk,
      \s_axi_rdata_reg[1]\ => u_filter_ir_n_1,
      \s_axi_rdata_reg[2]\ => u_filter_ir_n_2,
      \s_axi_rdata_reg[3]\ => u_filter_ir_n_3,
      \s_axi_rdata_reg[4]\ => u_filter_ir_n_4,
      \s_axi_rdata_reg[5]\ => u_filter_ir_n_5,
      \s_axi_rdata_reg[6]\ => u_filter_ir_n_6,
      \s_axi_rdata_reg[7]\(6 downto 0) => ibi_cycles(7 downto 1),
      \s_axi_rdata_reg[7]_0\ => u_filter_ir_n_0,
      \s_axi_rdata_reg[7]_1\(6 downto 0) => reg_ir_raw(7 downto 1),
      sel0(2 downto 0) => sel0(2 downto 0)
    );
u_peak_det: entity work.design_ZYNQ_axi_ppg_accelerator_0_0_ppg_peak_detector
     port map (
      D(7 downto 0) => data_out(7 downto 0),
      DI(3) => u_filter_red_n_9,
      DI(2) => u_filter_red_n_10,
      DI(1) => u_filter_red_n_11,
      DI(0) => u_filter_red_n_12,
      \FSM_sequential_current_state[1]_i_2_0\(3) => u_filter_red_n_13,
      \FSM_sequential_current_state[1]_i_2_0\(2) => u_filter_red_n_14,
      \FSM_sequential_current_state[1]_i_2_0\(1) => u_filter_red_n_15,
      \FSM_sequential_current_state[1]_i_2_0\(0) => u_filter_red_n_16,
      \FSM_sequential_current_state[1]_i_2_1\(3) => u_filter_red_n_29,
      \FSM_sequential_current_state[1]_i_2_1\(2) => u_filter_red_n_30,
      \FSM_sequential_current_state[1]_i_2_1\(1) => u_filter_red_n_31,
      \FSM_sequential_current_state[1]_i_2_1\(0) => u_filter_red_n_32,
      Q(7 downto 0) => ibi_cycles(7 downto 0),
      S(3) => u_filter_red_n_17,
      S(2) => u_filter_red_n_18,
      S(1) => u_filter_red_n_19,
      S(0) => u_filter_red_n_20,
      SR(0) => u_peak_det_n_1,
      \axi_araddr_latched_reg[2]\(23 downto 0) => s_axi_rdata_1(31 downto 8),
      beat_detected_reg_0 => irq_beat,
      beat_detected_reg_1 => u_peak_det_n_34,
      beat_flag_reg(0) => w_data_latched(0),
      data3(8 downto 1) => data3(15 downto 8),
      data3(0) => data3(0),
      p_1_in_0(2 downto 0) => p_1_in_0(2 downto 0),
      red_filter_valid => red_filter_valid,
      s_axi_aclk => s_axi_aclk,
      s_axi_aresetn => s_axi_aresetn,
      sel0(2 downto 0) => sel0(2 downto 0),
      write_execute => write_execute
    );
\w_data_latched_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(0),
      Q => w_data_latched(0),
      R => u_peak_det_n_1
    );
\w_data_latched_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(10),
      Q => \w_data_latched_reg_n_0_[10]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(11),
      Q => \w_data_latched_reg_n_0_[11]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(12),
      Q => \w_data_latched_reg_n_0_[12]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(13),
      Q => \w_data_latched_reg_n_0_[13]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(14),
      Q => \w_data_latched_reg_n_0_[14]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(15),
      Q => \w_data_latched_reg_n_0_[15]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(1),
      Q => \w_data_latched_reg_n_0_[1]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(2),
      Q => \w_data_latched_reg_n_0_[2]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(3),
      Q => \w_data_latched_reg_n_0_[3]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(4),
      Q => \w_data_latched_reg_n_0_[4]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(5),
      Q => \w_data_latched_reg_n_0_[5]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(6),
      Q => \w_data_latched_reg_n_0_[6]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(7),
      Q => \w_data_latched_reg_n_0_[7]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(8),
      Q => \w_data_latched_reg_n_0_[8]\,
      R => u_peak_det_n_1
    );
\w_data_latched_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => s_axi_wready0,
      D => s_axi_wdata(9),
      Q => \w_data_latched_reg_n_0_[9]\,
      R => u_peak_det_n_1
    );
w_done_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"CC0C8888"
    )
        port map (
      I0 => s_axi_wvalid,
      I1 => s_axi_aresetn,
      I2 => aw_done,
      I3 => \^s_axi_bvalid_reg_0\,
      I4 => w_done_reg_n_0,
      O => w_done_i_1_n_0
    );
w_done_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => w_done_i_1_n_0,
      Q => w_done_reg_n_0,
      R => '0'
    );
end STRUCTURE;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_ZYNQ_axi_ppg_accelerator_0_0 is
  port (
    s_axi_aclk : in STD_LOGIC;
    s_axi_aresetn : in STD_LOGIC;
    s_axi_awaddr : in STD_LOGIC_VECTOR ( 4 downto 0 );
    s_axi_awvalid : in STD_LOGIC;
    s_axi_awready : out STD_LOGIC;
    s_axi_wdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_wstrb : in STD_LOGIC_VECTOR ( 3 downto 0 );
    s_axi_wvalid : in STD_LOGIC;
    s_axi_wready : out STD_LOGIC;
    s_axi_bresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_bvalid : out STD_LOGIC;
    s_axi_bready : in STD_LOGIC;
    s_axi_araddr : in STD_LOGIC_VECTOR ( 4 downto 0 );
    s_axi_arvalid : in STD_LOGIC;
    s_axi_arready : out STD_LOGIC;
    s_axi_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_rresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_rvalid : out STD_LOGIC;
    s_axi_rready : in STD_LOGIC;
    irq_beat : out STD_LOGIC
  );
  attribute NotValidForBitStream : boolean;
  attribute NotValidForBitStream of design_ZYNQ_axi_ppg_accelerator_0_0 : entity is true;
  attribute CHECK_LICENSE_TYPE : string;
  attribute CHECK_LICENSE_TYPE of design_ZYNQ_axi_ppg_accelerator_0_0 : entity is "design_ZYNQ_axi_ppg_accelerator_0_0,axi_ppg_accelerator,{}";
  attribute DowngradeIPIdentifiedWarnings : string;
  attribute DowngradeIPIdentifiedWarnings of design_ZYNQ_axi_ppg_accelerator_0_0 : entity is "yes";
  attribute IP_DEFINITION_SOURCE : string;
  attribute IP_DEFINITION_SOURCE of design_ZYNQ_axi_ppg_accelerator_0_0 : entity is "module_ref";
  attribute X_CORE_INFO : string;
  attribute X_CORE_INFO of design_ZYNQ_axi_ppg_accelerator_0_0 : entity is "axi_ppg_accelerator,Vivado 2026.1";
end design_ZYNQ_axi_ppg_accelerator_0_0;

architecture STRUCTURE of design_ZYNQ_axi_ppg_accelerator_0_0 is
  signal \<const0>\ : STD_LOGIC;
  attribute X_INTERFACE_INFO : string;
  attribute X_INTERFACE_INFO of s_axi_aclk : signal is "xilinx.com:signal:clock:1.0 s_axi_aclk CLK";
  attribute X_INTERFACE_MODE : string;
  attribute X_INTERFACE_MODE of s_axi_aclk : signal is "slave";
  attribute X_INTERFACE_PARAMETER : string;
  attribute X_INTERFACE_PARAMETER of s_axi_aclk : signal is "XIL_INTERFACENAME s_axi_aclk, ASSOCIATED_BUSIF s_axi, ASSOCIATED_RESET s_axi_aresetn, FREQ_HZ 50000000, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_ZYNQ_processing_system7_0_0_FCLK_CLK0, INSERT_VIP 0";
  attribute X_INTERFACE_INFO of s_axi_aresetn : signal is "xilinx.com:signal:reset:1.0 s_axi_aresetn RST";
  attribute X_INTERFACE_MODE of s_axi_aresetn : signal is "slave";
  attribute X_INTERFACE_PARAMETER of s_axi_aresetn : signal is "XIL_INTERFACENAME s_axi_aresetn, POLARITY ACTIVE_LOW, INSERT_VIP 0";
  attribute X_INTERFACE_INFO of s_axi_arready : signal is "xilinx.com:interface:aximm:1.0 s_axi ARREADY";
  attribute X_INTERFACE_INFO of s_axi_arvalid : signal is "xilinx.com:interface:aximm:1.0 s_axi ARVALID";
  attribute X_INTERFACE_INFO of s_axi_awready : signal is "xilinx.com:interface:aximm:1.0 s_axi AWREADY";
  attribute X_INTERFACE_INFO of s_axi_awvalid : signal is "xilinx.com:interface:aximm:1.0 s_axi AWVALID";
  attribute X_INTERFACE_INFO of s_axi_bready : signal is "xilinx.com:interface:aximm:1.0 s_axi BREADY";
  attribute X_INTERFACE_INFO of s_axi_bvalid : signal is "xilinx.com:interface:aximm:1.0 s_axi BVALID";
  attribute X_INTERFACE_INFO of s_axi_rready : signal is "xilinx.com:interface:aximm:1.0 s_axi RREADY";
  attribute X_INTERFACE_INFO of s_axi_rvalid : signal is "xilinx.com:interface:aximm:1.0 s_axi RVALID";
  attribute X_INTERFACE_INFO of s_axi_wready : signal is "xilinx.com:interface:aximm:1.0 s_axi WREADY";
  attribute X_INTERFACE_INFO of s_axi_wvalid : signal is "xilinx.com:interface:aximm:1.0 s_axi WVALID";
  attribute X_INTERFACE_INFO of s_axi_araddr : signal is "xilinx.com:interface:aximm:1.0 s_axi ARADDR";
  attribute X_INTERFACE_INFO of s_axi_awaddr : signal is "xilinx.com:interface:aximm:1.0 s_axi AWADDR";
  attribute X_INTERFACE_MODE of s_axi_awaddr : signal is "slave";
  attribute X_INTERFACE_PARAMETER of s_axi_awaddr : signal is "XIL_INTERFACENAME s_axi, DATA_WIDTH 32, PROTOCOL AXI4LITE, FREQ_HZ 50000000, ID_WIDTH 0, ADDR_WIDTH 5, AWUSER_WIDTH 0, ARUSER_WIDTH 0, WUSER_WIDTH 0, RUSER_WIDTH 0, BUSER_WIDTH 0, READ_WRITE_MODE READ_WRITE, HAS_BURST 0, HAS_LOCK 0, HAS_PROT 0, HAS_CACHE 0, HAS_QOS 0, HAS_REGION 0, HAS_WSTRB 1, HAS_BRESP 1, HAS_RRESP 1, SUPPORTS_NARROW_BURST 0, NUM_READ_OUTSTANDING 1, NUM_WRITE_OUTSTANDING 1, MAX_BURST_LENGTH 1, PHASE 0.0, CLK_DOMAIN design_ZYNQ_processing_system7_0_0_FCLK_CLK0, NUM_READ_THREADS 1, NUM_WRITE_THREADS 1, RUSER_BITS_PER_BYTE 0, WUSER_BITS_PER_BYTE 0, INSERT_VIP 0";
  attribute X_INTERFACE_INFO of s_axi_bresp : signal is "xilinx.com:interface:aximm:1.0 s_axi BRESP";
  attribute X_INTERFACE_INFO of s_axi_rdata : signal is "xilinx.com:interface:aximm:1.0 s_axi RDATA";
  attribute X_INTERFACE_INFO of s_axi_rresp : signal is "xilinx.com:interface:aximm:1.0 s_axi RRESP";
  attribute X_INTERFACE_INFO of s_axi_wdata : signal is "xilinx.com:interface:aximm:1.0 s_axi WDATA";
  attribute X_INTERFACE_INFO of s_axi_wstrb : signal is "xilinx.com:interface:aximm:1.0 s_axi WSTRB";
begin
  s_axi_bresp(1) <= \<const0>\;
  s_axi_bresp(0) <= \<const0>\;
  s_axi_rresp(1) <= \<const0>\;
  s_axi_rresp(0) <= \<const0>\;
GND: unisim.vcomponents.GND
     port map (
      G => \<const0>\
    );
inst: entity work.design_ZYNQ_axi_ppg_accelerator_0_0_axi_ppg_accelerator
     port map (
      irq_beat => irq_beat,
      s_axi_aclk => s_axi_aclk,
      s_axi_araddr(2 downto 0) => s_axi_araddr(4 downto 2),
      s_axi_aresetn => s_axi_aresetn,
      s_axi_arready => s_axi_arready,
      s_axi_arvalid => s_axi_arvalid,
      s_axi_awaddr(2 downto 0) => s_axi_awaddr(4 downto 2),
      s_axi_awready => s_axi_awready,
      s_axi_awvalid => s_axi_awvalid,
      s_axi_bready => s_axi_bready,
      s_axi_bvalid_reg_0 => s_axi_bvalid,
      s_axi_rdata(31 downto 0) => s_axi_rdata(31 downto 0),
      s_axi_rready => s_axi_rready,
      s_axi_rvalid => s_axi_rvalid,
      s_axi_wdata(15 downto 0) => s_axi_wdata(15 downto 0),
      s_axi_wready => s_axi_wready,
      s_axi_wvalid => s_axi_wvalid
    );
end STRUCTURE;
