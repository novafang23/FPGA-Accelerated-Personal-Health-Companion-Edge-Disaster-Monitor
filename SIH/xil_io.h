#ifndef XIL_IO_H
#define XIL_IO_H

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

#endif /* XIL_IO_H */
