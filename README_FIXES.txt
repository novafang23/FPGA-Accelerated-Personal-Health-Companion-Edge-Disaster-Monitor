This folder mirrors your repo's structure exactly (verilog/firmware/...,
verilog/hardware/...). Drop the "verilog" folder here directly into your
repo root and let it merge/overwrite -- every file lands in its correct
existing location automatically. No manual renaming or path-hunting needed.

Files included (9 total):

FIRMWARE FIXES:
  firmware/core/disaster_risk_engine.c      - env==NULL crash guard
  firmware/zynq/test_disaster_risk_engine.c - new test_null_env() regression test
  firmware/zynq/main_simulation.c           - RMSSD jitter now tracks scripted scenario
  firmware/zynq/compare_harness.c           - same RMSSD jitter fix
  firmware/shrikefi/main_shrikefi.c         - real HRV-ready tracking + SpO2 wired up

RTL FIXES:
  hardware/common/ppg_peak_detector.v       - 2-consecutive-decrease + refractory
                                               baseline-return fixes
  hardware/common/tb_ppg_peak_detector.v    - NEW unit testbench (didn't exist before)
  hardware/zynq/tb_ppg_system.v             - tightened TEST 5 assertion
  hardware/shrikefi/tb_forgefpga_system.v   - tightened TEST 4 assertion

All of these were compiled/simulated and verified passing before being
included here (see the earlier chat messages for the exact verification
commands and output, and for what was deliberately left unfixed: i2c_hal.c
register ordering/NACK checking).
