/*
 * xil_io.h -- PC-SIMULATION SHIM, NOT THE REAL XILINX BSP HEADER.
 *
 * =====================================================================
 *  DO NOT compile this file into a real Vitis/Xilinx SDK project.
 *  Xilinx ships its own xil_io.h as part of the BSP; this file exists
 *  only so the C files under firmware/zynq/ can be built and unit-tested on a plain
 *  Linux/gcc host (see the Makefile's `all`/`test` targets).
 *
 *  Before bringing this up on real Zynq hardware in Vitis:
 *    1. Delete this file (or exclude it from the Vitis project sources).
 *    2. Let Vitis link against the BSP-generated xil_io.h instead.
 *
 *  The include guard below is deliberately NOT "XIL_IO_H" (the name the
 *  real Xilinx header uses). If it were, and both headers ended up on
 *  the include path, whichever got #include'd first would silently win
 *  with no compiler warning -- you could end up running this simplified
 *  stand-in on real hardware without ever finding out. Using a distinct
 *  guard means that situation instead fails loudly (duplicate Xil_In32
 *  definition), which is what you want.
 * =====================================================================
 */
#ifndef SIH26181_SIM_XIL_IO_H
#define SIH26181_SIM_XIL_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standard Xilinx memory-mapped I/O access functions.
 * When compiling outside of Xilinx BSP, this provides standard volatile MMIO access.
 */

#ifndef Xil_In32
static inline uint32_t Xil_In32(uintptr_t Addr) {
    return *(volatile uint32_t *)Addr;
}
#endif

#ifndef Xil_Out32
static inline void Xil_Out32(uintptr_t Addr, uint32_t Value) {
    *(volatile uint32_t *)Addr = Value;
}
#endif

#ifndef Xil_In16
static inline uint16_t Xil_In16(uintptr_t Addr) {
    return *(volatile uint16_t *)Addr;
}
#endif

#ifndef Xil_Out16
static inline void Xil_Out16(uintptr_t Addr, uint16_t Value) {
    *(volatile uint16_t *)Addr = Value;
}
#endif

#ifndef Xil_In8
static inline uint8_t Xil_In8(uintptr_t Addr) {
    return *(volatile uint8_t *)Addr;
}
#endif

#ifndef Xil_Out8
static inline void Xil_Out8(uintptr_t Addr, uint8_t Value) {
    *(volatile uint8_t *)Addr = Value;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* SIH26181_SIM_XIL_IO_H */
