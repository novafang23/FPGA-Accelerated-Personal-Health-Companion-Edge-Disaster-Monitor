# run_vivado_synth.tcl
# Automated Vivado Synthesis/Simulation & Timing Closure Script
# Usage:
#   Simulation: vivado -mode batch -source run_vivado_synth.tcl -tclargs sim
#   Synthesis:  vivado -mode batch -source run_vivado_synth.tcl -tclargs synth

puts "=========================================================================="
puts "  SIH26181: AXI PPG Accelerator Flow"
puts "=========================================================================="

# Parse arguments: first arg is mode (sim | synth)
set mode [lindex $argv 0]
if {$mode == ""} { set mode "synth" }

# Target part: Zynq-7000 (xc7z020clg400-1) as per project
set part "xc7z020clg400-1"
puts "  Target Part: $part"
puts "  Mode: $mode"
puts "=========================================================================="

set_param general.maxThreads 4

if {$mode == "sim"} {
    # ---- SIMULATION MODE ----
    puts ">> Running Behavioral Simulation (xsim)..."
    create_project -in_memory -part $part
    read_verilog moving_average_8tap.v
    read_verilog ppg_peak_detector.v
    read_verilog axi_ppg_accelerator.v
    read_verilog tb_ppg_system.v
    
    # Compile & elaborate
    puts ">> Compiling..."
    xvlog -sv -work xil_defaultlib [glob *.v]
    
    # Elaborate
    puts ">> Elaborating..."
    xelab -debug typical -top tb_ppg_system -s sim_snapshot -timescale 1ns/1ps
    
    # Run simulation
    puts ">> Simulating..."
    xsim sim_snapshot -runall -testplusarg VERBOSE
    
    puts ">> Simulation complete"
    exit
}

# ---- SYNTHESIS MODE ----
puts "  SIH26181: Synthesizing AXI PPG Accelerator IP"
puts "=========================================================================="

create_project -in_memory -part $part

# Read RTL source files
read_verilog moving_average_8tap.v
read_verilog ppg_peak_detector.v
read_verilog axi_ppg_accelerator.v

# Read Timing Constraints
read_xdc ppg_accelerator.xdc

# Run Synthesis targeting 50 MHz (20ns period)
puts ">> Starting RTL Synthesis..."
synth_design -top axi_ppg_accelerator -part $part -mode out_of_context -flatten_hierarchy rebuilt

# Generate Synthesis Utilization Report
puts ">> Generating Utilization Report..."
report_utilization -file utilization_synth.rpt -pb utilization_synth.pb

# Generate Timing Summary Report
puts ">> Generating Static Timing Analysis (STA) Report..."
report_timing_summary -delay_type min_max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -routable_nets -file timing_synth.rpt

# Generate Power Analysis Report
puts ">> Generating Power Estimation Report..."
report_power -file power_synth.rpt

# Check for Setup and Hold Violations
set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
set whs [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -hold]]

puts "=========================================================================="
puts "  SYNTHESIS RESULTS SUMMARY"
puts "=========================================================================="
puts [format "  Worst Negative Slack (WNS / Setup): %6.3f ns" $wns]
puts [format "  Worst Hold Slack     (WHS / Hold) : %6.3f ns" $whs]

if {$wns >= 0.0} {
    puts "  [TIMING STATUS]: >>> TIMING MET (NO VIOLATIONS) <<<"
} else {
    puts "  [TIMING STATUS]: >>> TIMING VIOLATION DETECTED <<<"
}
puts "=========================================================================="
puts "  Reports generated in working directory:"
puts "    - utilization_synth.rpt  (LUTs, FFs, BRAMs, DSP slices)"
puts "    - timing_synth.rpt       (Detailed STA setup/hold slacks)"
puts "    - power_synth.rpt        (Estimated dynamic and leakage power)"
puts "=========================================================================="

# Exit with error code if timing violated
if {$wns < 0.0} {
    exit 1
}
exit
