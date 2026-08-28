# ShrikeFi FPGA-MCU link protocol

## Physical interface
TODO: pin assignment for the 4-bit link, direction of each line, clock source

## Data framing
TODO: how filtered PPG samples and IBI counts are serialized over 4 bits

## Timing
TODO: link clock frequency, sample rate, latency budget

## Comparison to the Zynq AXI4-Lite interface
TODO: explicit note on what functionality from the old REG_* map has no
equivalent yet and needs a protocol decision (e.g. status/interrupt flags,
which had a dedicated W1C register on Zynq and currently has no analog here)
