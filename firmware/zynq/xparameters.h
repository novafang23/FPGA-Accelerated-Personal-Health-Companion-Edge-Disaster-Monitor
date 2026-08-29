/*
 * xparameters.h -- PC-SIMULATION SHIM, NOT THE REAL VIVADO/VITIS OUTPUT.
 *
 * =====================================================================
 *  Real Xilinx projects auto-generate this file from your Vivado block
 *  design address map. This hand-written stand-in exists only so
 *  the C files under firmware/zynq/ can be built and unit-tested on a plain Linux/gcc
 *  host without a Vitis project. The base address below (0x43C00000) is
 *  a PLAUSIBLE GUESS for the first custom AXI4-Lite IP in a typical
 *  Zynq PL address map -- it is NOT verified against any real block
 *  design and may not match your actual hardware.
 *
 *  Before bringing this up on real Zynq hardware in Vitis:
 *    1. Delete this file (or exclude it from the Vitis project sources).
 *    2. Let Vitis pull in the block-design-generated xparameters.h,
 *       which has the real, verified base address for your design.
 *
 *  The include guard is deliberately NOT "XPARAMETERS_H" (the name
 *  Xilinx's generated file uses), for the same reason as xil_io.h: two
 *  headers sharing a guard name means whichever is #include'd first
 *  wins silently, with no compiler warning -- exactly the failure mode
 *  you don't want for a hardcoded placeholder address.
 * =====================================================================
 */
#ifndef SIH26181_SIM_XPARAMETERS_H
#define SIH26181_SIM_XPARAMETERS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Definitions for peripheral AXI_PPG_ACCELERATOR_0
 * Base address for AXI4-Lite slave interface (5-bit address space = 32 bytes)
 *
 * Register offsets: 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14
 */
#define XPAR_AXI_PPG_ACCELERATOR_0_S_AXI_BASEADDR  0x43C00000U
#define XPAR_AXI_PPG_ACCELERATOR_0_S_AXI_HIGHADDR  0x43C0001FU

#ifdef __cplusplus
}
#endif

#endif /* SIH26181_SIM_XPARAMETERS_H */
