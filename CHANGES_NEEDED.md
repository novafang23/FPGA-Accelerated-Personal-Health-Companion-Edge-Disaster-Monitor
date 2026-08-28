# Changes Needed — ShrikeFi Docs vs. Implementation

Found by cross-checking `docs/theory/THEORY_NOTES.md` and `docs/SHRIKEFI_LINK_PROTOCOL.md` against the actual Verilog and firmware source. None require redesigning anything — just reconciling numbers and file references so the story holds up under judge questioning.

---

## 1. Link speed: two different numbers for the same link

- [x] **`docs/theory/THEORY_NOTES.md`** (Section 2 dual-FPGA table, and Section 7 analogy #4 "4-Lane Walkie-Talkie") claims:
  - Reconciled with verified simulation & protocol specification: **10 MHz** theoretical max ($T_{\text{strobe}} = 100\text{ ns}$, ~900 ns 32-bit IBI transfer, 5.0 MB/s raw bandwidth).
- [x] **`docs/SHRIKEFI_LINK_PROTOCOL.md`** (Section 4, Timing & Latency Budget) claims:
  - 10 MHz strobe rate (`T_strobe = 100 ns`)
  - ~900 ns for the same 8-nibble IBI read (9 strobe cycles including command nibble)

**Fix:** Reconciled across all documentation. Delineated between measured bring-up driver rate and simulated protocol max.

---

## 2. Documented speed vs. what the firmware actually does

- [x] `firmware/shrikefi/shrikefi_link_driver.c` (`pulse_strobe()`) bit-bangs the strobe with `esp_rom_delay_us(1)` — a 1 microsecond software delay on both the high and low edge of every strobe pulse.
- [x] That puts the *real*, as-implemented toggle rate at roughly 0.5 MHz (~2 µs per strobe cycle, ~18 µs for 32-bit IBI read), not the 10–25 MHz assumed in early drafts.

**Fix Applied:**
- Updated documentation to clearly state the honest current bring-up number (~0.5 MHz / 18 µs per IBI read / <0.1% bus duty cycle at 50 Hz), and separately noted the protocol's theoretical max (10 MHz / 900 ns / 5.0 MB/s) verified in simulation as the hardware-timer/RMT/SPI-assisted target.

---

## 3. Doc references files that don't exist in the repo

- [x] `docs/theory/THEORY_NOTES.md` (Section 4) listed `shrikefi_link_rx.v` and `shrikefi_link_tx.v` as if they are standalone Verilog files/modules.
- [x] In the actual repo, that RX/TX nibble logic all lives inside a single file: `hardware/shrikefi/forgefpga_ppg_top.v` (one combined FSM, command decode at line ~177 onward).

**Fix Applied:**
- Updated `THEORY_NOTES.md` Section 4 to accurately describe the 4 physical Verilog modules in the repo (`moving_average_8tap.v`, `ppg_peak_detector.v`, `axi_ppg_accelerator.v`, and `forgefpga_ppg_top.v`), presenting RX sample reassembly and TX 32-bit IBI serialization as internal FSM stages of `forgefpga_ppg_top.v`.

---

## 4. After fixing

- [x] Fixed path resolution in `tools/generate_pdf.py` and regenerated `docs/theory/theory_notes_print.html` and `docs/theory/SIH26181_Master_Theory_Notes.pdf`.
- [x] Skimmed `README.md` and `docs/MIGRATION.md` to confirm consistency.
