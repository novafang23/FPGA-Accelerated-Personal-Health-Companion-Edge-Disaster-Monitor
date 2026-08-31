# ppg_accelerator.xdc
# Timing & Physical Constraints for PPG Accelerator IP

# 1. Primary Clock Definition
# 50 MHz System Clock (20.0 ns period, 50% duty cycle)
create_clock -period 20.000 -name s_axi_aclk -waveform {0.000 10.000} [get_ports s_axi_aclk]

# Clock uncertainty and jitter allowance (100 ps)
set_clock_uncertainty 0.100 [get_clocks s_axi_aclk]

# 2. Asynchronous Reset Constraints
# Treat active-low reset assertion as false path for static timing analysis
set_false_path -from [get_ports s_axi_aresetn]

# 3. AXI4-Lite Input Delays
# Assume up to 3.0 ns interconnect delay for AXI control and data inputs
set_input_delay -clock [get_clocks s_axi_aclk] -max 3.000 [get_ports {{s_axi_awaddr[*]} s_axi_awvalid {s_axi_wdata[*]} {s_axi_wstrb[*]} s_axi_wvalid s_axi_bready {s_axi_araddr[*]} s_axi_arvalid s_axi_rready}]
set_input_delay -clock [get_clocks s_axi_aclk] -min 0.500 [get_ports {{s_axi_awaddr[*]} s_axi_awvalid {s_axi_wdata[*]} {s_axi_wstrb[*]} s_axi_wvalid s_axi_bready {s_axi_araddr[*]} s_axi_arvalid s_axi_rready}]

# 4. AXI4-Lite Output Delays
# Assume up to 3.0 ns output setup requirement on receiving master
set_output_delay -clock [get_clocks s_axi_aclk] -max 3.000 [get_ports {s_axi_awready s_axi_wready {s_axi_bresp[*]} s_axi_bvalid s_axi_arready {s_axi_rdata[*]} {s_axi_rresp[*]} s_axi_rvalid irq_beat}]
set_output_delay -clock [get_clocks s_axi_aclk] -min 0.500 [get_ports {s_axi_awready s_axi_wready {s_axi_bresp[*]} s_axi_bvalid s_axi_arready {s_axi_rdata[*]} {s_axi_rresp[*]} s_axi_rvalid irq_beat}]

# 5. Timing Exceptions & Multicycle Paths
# The refractory counter and interval timer are free-running at 50 MHz
# All paths must close timing within the 20.0 ns single-cycle budget


set_load 5.000 [all_outputs]
set_property LOAD 5 [get_ports irq_beat]
set_property LOAD 5 [get_ports s_axi_arready]
set_property LOAD 5 [get_ports s_axi_awready]
set_property LOAD 5 [get_ports {s_axi_bresp[0]}]
set_property LOAD 5 [get_ports {s_axi_bresp[1]}]
set_property LOAD 5 [get_ports s_axi_bvalid]
set_property LOAD 5 [get_ports {s_axi_rdata[0]}]
set_property LOAD 5 [get_ports {s_axi_rdata[10]}]
set_property LOAD 5 [get_ports {s_axi_rdata[11]}]
set_property LOAD 5 [get_ports {s_axi_rdata[12]}]
set_property LOAD 5 [get_ports {s_axi_rdata[13]}]
set_property LOAD 5 [get_ports {s_axi_rdata[14]}]
set_property LOAD 5 [get_ports {s_axi_rdata[15]}]
set_property LOAD 5 [get_ports {s_axi_rdata[16]}]
set_property LOAD 5 [get_ports {s_axi_rdata[17]}]
set_property LOAD 5 [get_ports {s_axi_rdata[18]}]
set_property LOAD 5 [get_ports {s_axi_rdata[19]}]
set_property LOAD 5 [get_ports {s_axi_rdata[1]}]
set_property LOAD 5 [get_ports {s_axi_rdata[20]}]
set_property LOAD 5 [get_ports {s_axi_rdata[21]}]
set_property LOAD 5 [get_ports {s_axi_rdata[22]}]
set_property LOAD 5 [get_ports {s_axi_rdata[23]}]
set_property LOAD 5 [get_ports {s_axi_rdata[24]}]
set_property LOAD 5 [get_ports {s_axi_rdata[25]}]
set_property LOAD 5 [get_ports {s_axi_rdata[26]}]
set_property LOAD 5 [get_ports {s_axi_rdata[27]}]
set_property LOAD 5 [get_ports {s_axi_rdata[28]}]
set_property LOAD 5 [get_ports {s_axi_rdata[29]}]
set_property LOAD 5 [get_ports {s_axi_rdata[2]}]
set_property LOAD 5 [get_ports {s_axi_rdata[30]}]
set_property LOAD 5 [get_ports {s_axi_rdata[31]}]
set_property LOAD 5 [get_ports {s_axi_rdata[3]}]
set_property LOAD 5 [get_ports {s_axi_rdata[4]}]
set_property LOAD 5 [get_ports {s_axi_rdata[5]}]
set_property LOAD 5 [get_ports {s_axi_rdata[6]}]
set_property LOAD 5 [get_ports {s_axi_rdata[7]}]
set_property LOAD 5 [get_ports {s_axi_rdata[8]}]
set_property LOAD 5 [get_ports {s_axi_rdata[9]}]
set_property LOAD 5 [get_ports {s_axi_rresp[0]}]
set_property LOAD 5 [get_ports {s_axi_rresp[1]}]
set_property LOAD 5 [get_ports s_axi_rvalid]
set_property LOAD 5 [get_ports s_axi_wready]
