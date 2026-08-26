# run_vivado_synth.tcl
# Automated Vivado Synthesis & Timing Closure Script
# Usage: vivado -mode batch -source run_vivado_synth.tcl

puts "=========================================================================="
puts "  SIH26181: Synthesizing AXI PPG Accelerator IP"
puts "  Target Part: xc7z020clg400-1 (Zynq-7000)"
puts "=========================================================================="

# Create in-memory project
set_param general.maxThreads 4
create_project -in_memory -part xc7z020clg400-1

# Read RTL source files
read_verilog moving_average_8tap.v
read_verilog ppg_peak_detector.v
read_verilog axi_ppg_accelerator.v

# Read Timing Constraints
read_xdc ppg_accelerator.xdc

# Run Synthesis targeting 50 MHz (20ns period)
puts ">> Starting RTL Synthesis..."
synth_design -top axi_ppg_accelerator -part xc7z020clg400-1 -mode out_of_context -flatten_hierarchy rebuilt

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

exit
