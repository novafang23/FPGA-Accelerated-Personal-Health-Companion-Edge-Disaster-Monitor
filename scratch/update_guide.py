import re

with open('VALOR_COMPLETE_GUIDE.md', 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Formula in Section 8: x[n] - x[n-7] -> x[n] - x[n-8]
old_s8 = r'$$\text{Sum}_{\text{new}} = \text{Sum}_{\text{old}} + x[n] - x[n-7]$$'
new_s8 = r'$$\text{Sum}_{\text{new}} = \text{Sum}_{\text{old}} + x[n] - x[n-8]$$'
assert old_s8 in text, "Target 1 not found"
text = text.replace(old_s8, new_s8)

# Also update the explanation below it
old_s8_desc = 'wire [DATA_WIDTH+2:0] next_sum = running_sum + {3\'b000, data_in} - {3\'b000, shift_reg[7]};'
new_s8_desc = (
    'wire [DATA_WIDTH+2:0] next_sum = running_sum + {3\'b000, data_in} - {3\'b000, shift_reg[7]};\n'
    '// Note: shift_reg[7] stores sample x[n-8] (the oldest of the 8 historical window taps),\n'
    '// where shift_reg[0]=x[n-1], shift_reg[1]=x[n-2], ..., shift_reg[7]=x[n-8].'
)
assert old_s8_desc in text, "Target 1 desc not found"
text = text.replace(old_s8_desc, new_s8_desc)

# 2. Section 9 Reset Synchronicity (Q: Is the FSM synchronous?)
old_s9 = (
    '- **Q:** *Is the FSM synchronous?*  \n'
    '  **A:** Yes, 100% synchronous. All state transitions, running crest tracking, and interval counter updates occur strictly on the rising edge of the 50 MHz system clock, preventing race conditions or timing hazards.'
)
new_s9 = (
    '- **Q:** *Is the FSM synchronous?*  \n'
    '  **A:** State transitions and counter updates are 100% clock-synchronous to the rising edge of the 50 MHz system clock (`posedge clk`). However, system reset uses an asynchronous active-low reset (`always @(posedge clk or negedge rst_n)` in `ppg_peak_detector.v`), ensuring instantaneous hardware recovery without race conditions.'
)
assert old_s9 in text, "Target 2 not found"
text = text.replace(old_s9, new_s9)

# 3. Section 10 State Machine Count & Classification
old_s10_head = 'In addition to the systolic peak detector, VALOR contains four other distinct state machines in RTL and firmware:'
new_s10_head = 'In addition to the systolic peak detector FSM, VALOR contains three other formal state machines in firmware and RTL, alongside two cycle-accurate hardware sequencers:'
assert old_s10_head in text, "Target 3 not found"
text = text.replace(old_s10_head, new_s10_head)

# Update SPI Target Bit Shifter heading in Section 10
old_spi_shifter = '### 3. SPI Target Bit Shifter (`hardware/shrikefi/spi_target.v`)'
new_spi_shifter = '### 3. Synchronous SPI Target Bit Shifter & Counter (`hardware/shrikefi/spi_target.v`)'
assert old_spi_shifter in text, "Target 3b not found"
text = text.replace(old_spi_shifter, new_spi_shifter)

# 4. Section 10 SOS Confirmation Windows & Recovery Transition
old_sos_win = '- **15-Second Confirmation Window (`critical_confirm_ms` & `fall_confirm_ms`):** The system initiates a timed grace period (configurable up to 15 seconds; defaults: 3 seconds for vitals, 10 seconds for falls). An on-screen countdown allows a conscious wearer to cancel false alarms before escalation.'
new_sos_win = '- **Timed Confirmation Windows (`critical_confirm_ms` & `fall_confirm_ms`):** The system initiates a timed grace period: 15 seconds for critical vitals (`critical_confirm_ms = 15000u` in `sos.c:41`, increased from 3s to suppress transient motion false alarms) and 10 seconds for biomechanical falls (`fall_confirm_ms = 10000u` in `sos.c:46`). An on-screen countdown allows a conscious wearer to cancel false alarms before escalation.'
assert old_sos_win in text, "Target 4 not found"
text = text.replace(old_sos_win, new_sos_win)

old_sos_diagram = (
    'stateDiagram-v2\n'
    '    [*] --> SOS_IDLE\n'
    '    SOS_IDLE --> SOS_ARMED: Critical Vitals (>=3s) OR Fall Event\n'
    '    SOS_ARMED --> SOS_ACTIVE: Grace Timer Expires (10s uncancelled)\n'
    '    SOS_ARMED --> SOS_IDLE: Condition Clears Early OR User Cancels\n'
    '    SOS_ACTIVE --> SOS_CANCELLED: 5s Button Hold OR Web Cancel\n'
    '    SOS_ACTIVE --> SOS_IDLE: 30s Sustained Recovery Stand-Down\n'
    '    SOS_CANCELLED --> SOS_IDLE: Underlying Fault Clears'
)
new_sos_diagram = (
    'stateDiagram-v2\n'
    '    [*] --> SOS_IDLE\n'
    '    SOS_IDLE --> SOS_ARMED: Critical Vitals (>=15s sustained) OR Fall Event\n'
    '    SOS_ARMED --> SOS_ACTIVE: Grace Timer Expires (10s fall / 15s vitals uncancelled)\n'
    '    SOS_ARMED --> SOS_IDLE: Condition Clears Early OR User Cancels\n'
    '    SOS_ACTIVE --> SOS_CANCELLED: 5s Button Hold OR Web Cancel OR 30s Sustained Recovery Stand-Down\n'
    '    SOS_CANCELLED --> SOS_IDLE: Underlying Fault Clears'
)
assert old_sos_diagram in text, "Target 4b not found"
text = text.replace(old_sos_diagram, new_sos_diagram)

# 5. Section 13 TinyML Inference Latency
old_t13_1 = '2. **Speed & Latency:** An on-device INT8 forward pass executes in less than **1 microsecond**, compared to 200–2000 milliseconds for a cloud round-trip.'
new_t13_1 = '2. **Speed & Latency:** An on-device INT8 forward pass executes in tens of microseconds (~20–50 µs on the 160 MHz ESP32-S3 Xtensa LX7; 0.44 µs on x86 host), compared to 200–2000 milliseconds for a cloud round-trip, easily leaving >99.9% of the CPU free during the 1-second triage cycle.'
assert old_t13_1 in text, "Target 5a not found"
text = text.replace(old_t13_1, new_t13_1)

old_t13_2 = '- Inference Latency: **0.44 microseconds** on x86 host; runs in single-digit microseconds on the 240 MHz ESP32-S3 FPU.'
new_t13_2 = '- Inference Latency: **0.44 microseconds** on x86 host benchmark; executes in tens of microseconds (~20–50 µs) on the 160 MHz ESP32-S3 Xtensa LX7 (576 float MACs + 3 software `expf()` sigmoid calls).'
assert old_t13_2 in text, "Target 5b not found"
text = text.replace(old_t13_2, new_t13_2)

old_t13_3 = 'running in single-digit microseconds on the hardware FPU'
new_t13_3 = 'running in tens of microseconds (~20–50 µs) on the Xtensa LX7 FPU'
assert old_t13_3 in text, "Target 5c not found"
text = text.replace(old_t13_3, new_t13_3)

old_t13_4 = 'computes multi-hazard risk in single-digit microseconds using the ESP32-S3 single-precision hardware FPU with dequantized INT8 weights.'
new_t13_4 = 'computes multi-hazard risk in tens of microseconds (~20–50 µs) using the ESP32-S3 single-precision hardware FPU with dequantized INT8 weights and runtime `expf()` sigmoid activation.'
assert old_t13_4 in text, "Target 5d not found"
text = text.replace(old_t13_4, new_t13_4)

# 6. Section 14 PM2.5 Threshold
old_pm25 = '- **Caution:** $\\text{PM2.5} \\ge 35\\ \\mu\\text{g/m}^3$ (WHO 24-hour limit exceedance) OR $\\text{SpO}_2 \\le 96\\%$.'
new_pm25 = '- **Caution:** $\\text{PM2.5} \\ge 35\\ \\mu\\text{g/m}^3$ (US EPA 24-hour NAAQS standard; WHO 2021 24-hour guideline is $15\\ \\mu\\text{g/m}^3$) OR $\\text{SpO}_2 \\le 96\\%$.'
assert old_pm25 in text, "Target 6 not found"
text = text.replace(old_pm25, new_pm25)

# 7. Section 15 Skin Temperature Accuracy
old_skin = 'This outputs an accurate resting skin temperature ($35.7^\\circ\\text{C}$) on the OLED screen (`Skin: 35.7 C`) and web dashboard while strictly withholding temperature points ($0\\text{ points}$) from mNEWS2 core triage, preventing clinical false alarms.'
new_skin = 'This provides a smooth, uncalibrated heuristic skin temperature trend estimate ($T_{base} = 34.50^\\circ\\text{C}$ nominal) displayed on the OLED screen (`Skin: 35.7 C`) and web dashboard, while strictly withholding temperature points ($0\\text{ points}$) from mNEWS2 core triage because it has not undergone clinical gold-standard thermometer calibration.'
assert old_skin in text, "Target 7 not found"
text = text.replace(old_skin, new_skin)

# 8. Section 19 Q14 MAX30102 Die Temp
old_q14 = '- *Don\'t Say:* "The MAX30102 doesn\'t have a temperature sensor." (It does have one; we deliberately deleted the read from `max30102.c` due to LED self-heating).'
new_q14 = '- *Don\'t Say:* "The MAX30102 doesn\'t have a temperature sensor." (It does have an on-chip sensor read via `max30102_read_die_temp()` in `max30102.c`; we poll it every 1 second in `main_shrikefi.c` to track silicon equilibrium for our baseline-relative thermal model, but never use raw die temperature directly as core body temperature).'
assert old_q14 in text, "Target 8 not found"
text = text.replace(old_q14, new_q14)

# 9. Section 19 Q19 INT8 Quantization Error
old_q19 = 'executing in single-digit microseconds with only 0.018 mean absolute error vs. the FP32 baseline.'
new_q19 = 'executing in tens of microseconds with only 0.00790 Mean Absolute Error (max absolute error 0.0914) vs. the FP32 baseline.'
assert old_q19 in text, "Target 9 not found"
text = text.replace(old_q19, new_q19)

# 10. Section 19 Q26 LED Drive Current
old_q26 = '- *20-Second Answer:* Darker pigmentation absorbs more green and red light. The MAX30102 driver implements automatic current scaling, boosting LED drive current up to 50 mA to maintain perfusion index.'
new_q26 = '- *20-Second Answer:* Darker pigmentation absorbs more red and infrared light. The MAX30102 driver configures a calibrated static LED current of 11.0 mA (register 0x37, with a driver ceiling of 18.0 mA 0x5A) to ensure stable optical perfusion while preventing excessive silicon heating.'
assert old_q26 in text, "Target 10 not found"
text = text.replace(old_q26, new_q26)

with open('VALOR_COMPLETE_GUIDE.md', 'w', encoding='utf-8') as f:
    f.write(text)

print("All 10 updates applied cleanly to VALOR_COMPLETE_GUIDE.md!")
