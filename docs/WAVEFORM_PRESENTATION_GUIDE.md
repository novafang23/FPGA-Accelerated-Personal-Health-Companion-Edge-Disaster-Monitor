# Waveform Presentation Guide — SIH26181

Quick reference for the two platform waveforms. Open with GTKWave:

```bash
# Zynq-7000 (verified baseline)
gtkwave ppg_system.vcd hardware/zynq/presentation.gtkw

# ShrikeFi (ForgeFPGA + ESP32-S3)
gtkwave hardware/shrikefi/shrikefi_sim.vcd hardware/shrikefi/presentation.gtkw
```

---

## Zynq-7000 — Key Signal Groups

| Group | Signals | What to Watch |
|-------|---------|---------------|
| **Clock/Reset** | `s_axi_aclk`, `s_axi_aresetn` | 50 MHz clock, active-low reset |
| **AXI Write** | `awaddr`, `awvalid/awready`, `wdata`, `wvalid/wready`, `bvalid/bready` | Register config: threshold (0x0C), etc. |
| **AXI Read** | `araddr`, `arvalid/arready`, `rdata`, `rvalid/rready` | Data extraction: filtered PPG (0x04/0x14), IBI (0x08), status (0x0C) |
| **Filters** | `reg_red_raw`, `red_filtered`, `reg_ir_raw`, `ir_filtered` | 8-tap moving avg, 0 DSP, single-cycle latency |
| **Peak Detector FSM** | `current_state` (0=ARMED,1=RISING,2=PEAK,3=REFRACTORY), `interval_cnt`, `refractory_cnt` | 4-state systolic detector, 250 ms refractory |
| **Outputs** | `hw_beat_pulse`, `irq_beat`, `beat_flag`, `hw_ibi_cycles` | **Key result**: IBI in 20 ns ticks |
| **Scoreboard** | `tests_passed`, `tests_failed`, `test_num` | Should show 6 passed, 0 failed |

---

## ShrikeFi — Key Signal Groups

| Group | Signals | What to Watch |
|-------|---------|---------------|
| **Clock/Reset** | `clk`, `rst_n` (both TB and DUT) | ForgeFPGA clock domain |
| **4-Bit Link** | `link_strobe`, `link_dir`, `link_din[3:0]`, `link_dout[3:0]`, `link_dout_oe`, `strobe_sync` | **Core innovation**: 4-bit parallel FPGA↔MCU protocol |
| **Filters** | `reg_red_raw`, `red_filtered`, `red_valid_pulse`, `red_filtered_valid` (same for IR) | Same filter RTL, validated on ForgeFPGA |
| **Peak Detector** | `state` (link FSM), `u_peak_det.current_state`, `peak_beat_detected`, `reg_threshold`, `peak_ibi_cycles`, `reg_ibi_latched` | Peak detect + IBI latching for link transfer |
| **Output** | `irq_beat` (TB + DUT) | Beat interrupt to ESP32-S3 |
| **Scoreboard** | `tests_passed`, `tests_failed`, `total_tests` | Should show 5 passed, 0 failed |

---

## Presentation Tips

1. **Zoom to first beat**: Both simulations show ~2-3 beats in ~2s sim time
2. **Color coding**: Groups are color-coded (Clock=blue, AXI=magenta/cyan, Filters=purple, FSM=red, Outputs=green, TB=orange)
3. **Cursor measurements**: Use GTKWave cursors to measure:
   - IBI cycles → heart rate: `HR = 60 / (ibi_cycles * 20ns)`
   - Filter latency: `data_valid` to `filtered_valid` = 1 cycle
4. **Export images**: File → Write Image → PNG for slides

---

## Quick Signal Mapping (Zynq AXI ↔ ShrikeFi Link)

| Zynq AXI Register | ShrikeFi Link Equivalent |
|-------------------|--------------------------|
| `REG_RED_RAW` (0x00) | Nibble 0-1 of `link_dout` (FPGA→MCU) |
| `REG_RED_FILTERED` (0x04) | Nibble 2-3 of `link_dout` |
| `REG_IBI_CYCLES` (0x08) | Nibble 4-7 of `link_dout` (32-bit split) |
| `REG_STATUS_THRESH` (0x0C) | Status nibble in link protocol |
| `REG_IR_RAW` (0x10) | Nibble 8-9 of `link_dout` |
| `REG_IR_FILTERED` (0x14) | Nibble 10-11 of `link_dout` |

See `docs/SHRIKEFI_LINK_PROTOCOL.md` for full protocol spec.