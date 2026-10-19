# ==============================================================================
# ShrikeFi ForgeFPGA (SLG47910) - real timing constraint
# ==============================================================================
# WHY THIS FILE EXISTS
# Until now the project declared no clock anywhere, so the ForgeFPGA fitter
# auto-constrained every register to a 2000 ps (500 MHz) placeholder period.
# The reported WNS of -9.357 ns is measured against that placeholder, so it is
# meaningless in both directions: it is not evidence that the design fails
# timing, and it is not evidence that it closes. This file replaces the
# placeholder with the frequency the design is actually built for.
#
# WHAT THE REAL CLOCK IS
# forgefpga_ppg_top.v has exactly one clock domain: `clk`, driven by the
# SLG47910 internal 50 MHz oscillator (OSC_CLK), enabled by `clk_en -> OSC_EN`.
# It is not a pin - it is an internal resource - which is why it has no `set_io`
# line in forgefpga_pins.pcf.
#
# The RTL is written for 50 MHz and says so: CLK_FREQ_HZ = 50_000_000,
# REFRACTORY_CYC = 12_500_000 cycles for a 250 ms blanking window, and
# LED_PULSE_CYC = 2_500_000 cycles for a 50 ms pulse. Constraining anything
# other than 50 MHz would contradict the design itself.
#
# spi_sck is NOT a second clock domain. The 4-wire SPI target does not clock
# fabric logic with SCK; the SPI lines are sampled inside the clk domain. The
# ESP32 drives SCK at 1 MHz for the runtime link and 16 MHz while flashing the
# bitstream (see shrikefi_link_driver.c), both far below 50 MHz. Adding a
# create_clock for spi_sck would invent a domain that does not exist, so there
# is deliberately no second constraint and no set_clock_groups here.
#
# Syntax matches the vendor documentation (Renesas AN-FG-036, "ForgeFPGA Timing
# Constraints") and the installed example project
# application-notes/an-fg-024/.../ffpga/timing-constraints/timing_constraints.sdc.
# Period is in ns: 20.000 ns = 50 MHz; waveform {0 10.000} is a 50 % duty cycle.
# ==============================================================================

create_clock -name clk {clk} -period 20.000 -waveform {0 10.000}
