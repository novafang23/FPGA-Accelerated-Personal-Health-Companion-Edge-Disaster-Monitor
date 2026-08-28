import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import os

repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
img_dir = os.path.join(repo_root, "docs", "images")
os.makedirs(img_dir, exist_ok=True)

# -----------------------------------------------------------------------------
# 1. GENERATE SHRIKEFI 4-BIT LINK WAVEFORM (shrikefi_waveform.png)
# -----------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(14, 7.5), dpi=180)
fig.patch.set_facecolor('#0b1120')
ax.set_facecolor('#0b1120')

t = np.linspace(0, 140, 1400)

# Generate clock and signals
clk = 0.5 * (np.sin(2 * np.pi * t / 2) > 0)
rst_n = (t > 4).astype(float)

# Strobe: series of pulses for Write (t=10 to 30) and Read (t=80 to 130)
strobe = np.zeros_like(t)
strobe_pulses = [12, 18, 24, 40, 46, 52, 82, 88, 94, 100, 106, 112, 118, 124, 130, 136]
for sp in strobe_pulses:
    strobe[(t >= sp) & (t < sp + 2.5)] = 1.0

# link_dir: 0 for write, 1 for read (t=86 to 132)
link_dir = np.zeros_like(t)
link_dir[(t >= 85) & (t <= 132)] = 1.0

# irq_beat pulse
irq_beat = np.zeros_like(t)
irq_beat[(t >= 75) & (t <= 136)] = 1.0  # Latches on peak at t=75 until cleared at t=136

# Filtered analog waveform
filter_red = 90 + 35 * np.sin(2 * np.pi * (t - 45) / 50) * (t > 45)

signals = [
    ("clk (50 MHz / 20ns)", "digital", clk, '#38bdf8'),
    ("rst_n (Active-Low)", "digital", rst_n, '#94a3b8'),
    ("link_strobe (Strobe Pulse)", "pulse", strobe, '#f43f5e'),
    ("link_dir (0=Write, 1=Read)", "digital", link_dir, '#a855f7'),
    ("link_din[3:0] (Host -> FPGA)", "bus_din", t, '#0ea5e9'),
    ("filter_red[7:0] (8-Tap Filter)", "analog", filter_red, '#f59e0b'),
    ("irq_beat (Hardware Beat IRQ)", "pulse", irq_beat, '#10b981'),
    ("link_dout[3:0] (FPGA -> Host)", "bus_dout", t, '#22c55e')
]

y_pos = len(signals) - 1
for name, sig_type, val, color in signals:
    base_y = y_pos * 1.35
    if sig_type == "digital":
        ax.step(t, base_y + val * 0.75, where='post', color=color, linewidth=1.4)
    elif sig_type == "pulse":
        ax.step(t, base_y + val * 0.75, where='post', color=color, linewidth=1.8)
    elif sig_type == "analog":
        val_norm = (val - 55) / (130 - 55) * 0.75
        val_norm = np.clip(val_norm, 0, 0.75)
        ax.plot(t, base_y + val_norm, color=color, linewidth=2.0)
    elif sig_type == "bus_din":
        # Write 8-bit sample: CMD_WRITE_RED (0x1) -> High Nibble (0x7) -> Low Nibble (0x8)
        rects = [
            (0, 10, "IDLE", "#1e293b", "#475569"),
            (10, 16, "CMD: 0x1", "#0369a1", "#38bdf8"),
            (16, 22, "HIGH: 0x7", "#0284c7", "#38bdf8"),
            (22, 28, "LOW: 0x8", "#0284c7", "#38bdf8"),
            (28, 80, "IDLE / SAMPLES", "#1e293b", "#475569"),
            (80, 86, "CMD: 0x6 (READ_IBI)", "#7c3aed", "#c084fc"),
            (86, 134, "TRISTATE (READ MODE)", "#334155", "#64748b"),
            (134, 140, "CMD: 0x7 (CLR_IRQ)", "#047857", "#34d399")
        ]
        for r_start, r_end, r_label, r_bg, r_border in rects:
            rect = patches.Rectangle((r_start, base_y), r_end - r_start, 0.75, facecolor=r_bg, edgecolor=r_border, alpha=0.9)
            ax.add_patch(rect)
            ax.text((r_start + r_end)/2, base_y + 0.35, r_label, color='white', fontsize=7, weight='bold', ha='center', va='center')
    elif sig_type == "bus_dout":
        # Read 32-bit IBI: 8 consecutive nibbles (0x0, 0x0, 0x0, 0x0, 0x0, 0xC, 0xD, 0x1)
        rects = [
            (0, 86, "HIGH-Z / IDLE", "#1e293b", "#475569"),
            (86, 92, "N0: 0x0", "#065f46", "#34d399"),
            (92, 98, "N1: 0x0", "#065f46", "#34d399"),
            (98, 104, "N2: 0x0", "#065f46", "#34d399"),
            (104, 110, "N3: 0x0", "#065f46", "#34d399"),
            (110, 116, "N4: 0x0", "#065f46", "#34d399"),
            (116, 122, "N5: 0xC", "#059669", "#6ee7b7"),
            (122, 128, "N6: 0xD", "#059669", "#6ee7b7"),
            (128, 134, "N7: 0x1", "#059669", "#6ee7b7"),
            (134, 140, "IDLE", "#1e293b", "#475569")
        ]
        for r_start, r_end, r_label, r_bg, r_border in rects:
            rect = patches.Rectangle((r_start, base_y), r_end - r_start, 0.75, facecolor=r_bg, edgecolor=r_border, alpha=0.9)
            ax.add_patch(rect)
            ax.text((r_start + r_end)/2, base_y + 0.35, r_label, color='white', fontsize=6.5, weight='bold', ha='center', va='center')

    ax.text(-2, base_y + 0.35, name, color='#f1f5f9', fontsize=8.5, weight='bold', ha='right', va='center')
    y_pos -= 1

# Annotations
ax.annotate('Write PPG Sample: CMD(0x1) + High(0x7) + Low(0x8)', xy=(19, 6.4), xytext=(28, 8.5),
            arrowprops=dict(facecolor='#38bdf8', shrink=0.05, width=1.5, headwidth=5),
            color='#38bdf8', fontsize=8, weight='bold', bbox=dict(boxstyle="round,pad=0.3", fc="#0f172a", ec="#38bdf8"))

ax.annotate('Peak Detected: irq_beat asserts\n(Hardware Interrupt on GPIO10)', xy=(76, 2.3), xytext=(55, 3.8),
            arrowprops=dict(facecolor='#10b981', shrink=0.05, width=1.5, headwidth=5),
            color='#10b981', fontsize=8, weight='bold', bbox=dict(boxstyle="round,pad=0.3", fc="#0f172a", ec="#10b981"))

ax.annotate('32-Bit IBI Read: 8 Sequential Nibbles (0x00000CD1 = 3281 cycles)', xy=(110, 0.9), xytext=(85, -0.6),
            arrowprops=dict(facecolor='#34d399', shrink=0.05, width=1.5, headwidth=5),
            color='#34d399', fontsize=8, weight='bold', bbox=dict(boxstyle="round,pad=0.3", fc="#0f172a", ec="#34d399"))

ax.set_xlim(-28, 145)
ax.set_ylim(-1.2, len(signals) * 1.35 + 0.6)
ax.set_title("Renesas ForgeFPGA (ShrikeFi) 4-Bit Parallel Link Protocol: Sample Streaming, Peak IRQ & 32-Bit IBI Read",
             color='#ffffff', fontsize=11, weight='bold', pad=15)
ax.axis('off')

plt.tight_layout()
wf_path = os.path.join(img_dir, "shrikefi_waveform.png")
plt.savefig(wf_path, dpi=180, facecolor=fig.get_facecolor(), bbox_inches='tight')
plt.close()
print(f"Saved {wf_path}")

# -----------------------------------------------------------------------------
# 2. GENERATE SHRIKEFI PINOUT & WIRING SCHEMATIC (shrikefi_pinout.png)
# -----------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(13, 7.5), dpi=180)
fig.patch.set_facecolor('#0f172a')
ax.set_facecolor('#0f172a')

# ESP32-S3 Block
rect_mcu = patches.FancyBboxPatch((5, 20), 30, 55, boxstyle="round,pad=1.5", facecolor='#1e293b', edgecolor='#38bdf8', linewidth=2)
ax.add_patch(rect_mcu)
ax.text(20, 70, "ESP32-S3 SoC\n(Host Microcontroller)", color='#38bdf8', fontsize=11, weight='bold', ha='center')
ax.text(20, 63, "Dual-Core Xtensa LX7 @ 240MHz\nFreeRTOS + TinyML Model", color='#94a3b8', fontsize=8, ha='center')

# ForgeFPGA Block
rect_fpga = patches.FancyBboxPatch((65, 25), 30, 50, boxstyle="round,pad=1.5", facecolor='#1e293b', edgecolor='#f59e0b', linewidth=2)
ax.add_patch(rect_fpga)
ax.text(80, 70, "Renesas ForgeFPGA\n(SLG47910 Hardware Accel)", color='#f59e0b', fontsize=11, weight='bold', ha='center')
ax.text(80, 63, "1120 5-Input LUTs (17.4% Used)\nMoving Avg Filter + Peak FSM", color='#94a3b8', fontsize=8, ha='center')

# Sensor Suite Block
rect_sensors = patches.FancyBboxPatch((5, 2), 90, 12, boxstyle="round,pad=1", facecolor='#1e293b', edgecolor='#10b981', linewidth=1.5)
ax.add_patch(rect_sensors)
ax.text(50, 10, "Integrated Edge Biometric & Environmental Sensor Suite (3.3V)", color='#34d399', fontsize=10, weight='bold', ha='center')
ax.text(50, 5, "MAX30102 (PPG/SpO2: I2C)  •  BME280 (Temp/Hum: I2C)  •  PMS5003 (PM2.5: UART)  •  SSD1306 (OLED: I2C)", color='#e2e8f0', fontsize=8.5, ha='center')

# Link Connections between ESP32 and ForgeFPGA
connections = [
    ("GPIO 3", "PIN_13", "rst_n (Active-Low Reset)", 55, '#94a3b8'),
    ("GPIO 4", "PIN_14", "link_strobe (Strobe Clock)", 49, '#f43f5e'),
    ("GPIO 5", "PIN_15", "link_dir (0=Write, 1=Read)", 43, '#a855f7'),
    ("GPIO 6-9", "PIN_16-19", "link_data[3:0] (4-Bit Bus)", 37, '#38bdf8'),
    ("GPIO 10", "PIN_24", "irq_beat (Systolic Peak IRQ)", 31, '#10b981'),
]

for mcu_pin, fpga_pin, label, y_c, col in connections:
    ax.plot([36, 64], [y_c, y_c], color=col, linewidth=2, linestyle='-')
    # Direction arrows
    if "irq_beat" in label:
        ax.annotate('', xy=(36, y_c), xytext=(64, y_c), arrowprops=dict(arrowstyle="->", color=col, lw=2))
    elif "link_data" in label:
        ax.annotate('', xy=(64, y_c), xytext=(36, y_c), arrowprops=dict(arrowstyle="<->", color=col, lw=2))
    else:
        ax.annotate('', xy=(64, y_c), xytext=(36, y_c), arrowprops=dict(arrowstyle="->", color=col, lw=2))
    
    ax.text(37, y_c + 1.2, mcu_pin, color='#cbd5e1', fontsize=7.5, weight='bold', ha='left')
    ax.text(63, y_c + 1.2, fpga_pin, color='#cbd5e1', fontsize=7.5, weight='bold', ha='right')
    ax.text(50, y_c + 1.2, label, color=col, fontsize=7.5, weight='bold', ha='center',
            bbox=dict(boxstyle="round,pad=0.2", fc="#0f172a", ec=col, lw=0.8))

# Sensor buses down to sensors
ax.plot([12, 12], [20, 14], color='#38bdf8', linewidth=1.8, linestyle='--')
ax.text(14, 17, "I2C Bus (SDA/SCL: GPIO 21/22)", color='#38bdf8', fontsize=7.5, weight='bold')

ax.plot([28, 28], [20, 14], color='#f59e0b', linewidth=1.8, linestyle='--')
ax.text(30, 17, "UART RX/TX (GPIO 1/2)", color='#f59e0b', fontsize=7.5, weight='bold')

# 3.3V Warning Banner
rect_warn = patches.FancyBboxPatch((15, 82), 70, 7, boxstyle="round,pad=0.8", facecolor='#450a0a', edgecolor='#ef4444', linewidth=1.5)
ax.add_patch(rect_warn)
ax.text(50, 85.5, "⚠ 3.3V LVCMOS ELECTRICAL BOUNDARY ONLY ⚠", color='#fca5a5', fontsize=9.5, weight='bold', ha='center')
ax.text(50, 82.5, "All FPGA and MCU pins are 3.3V strict. 5V logic inputs permanently destroy both ICs.", color='#fecaca', fontsize=7.5, ha='center')

ax.set_xlim(0, 100)
ax.set_ylim(-2, 95)
ax.set_title("ShrikeFi Heterogeneous Edge Architecture: ESP32-S3 <-> ForgeFPGA Interconnect & Pin Mapping",
             color='#ffffff', fontsize=11, weight='bold', pad=15)
ax.axis('off')

plt.tight_layout()
pin_path = os.path.join(img_dir, "shrikefi_pinout.png")
plt.savefig(pin_path, dpi=180, facecolor=fig.get_facecolor(), bbox_inches='tight')
plt.close()
print(f"Saved {pin_path}")

# -----------------------------------------------------------------------------
# 3. GENERATE FORGEFPGA UTILIZATION CHART (forgefpga_utilization.png)
# -----------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(10, 5.5), dpi=180)
fig.patch.set_facecolor('#0f172a')
ax.set_facecolor('#0f172a')

categories = [
    "CLB LUT5s (Logic)",
    "Flip-Flops (Registers)",
    "CLB Macrocells",
    "DSP Multipliers",
    "Block RAM (BRAM)"
]

used = [195, 110, 35, 0, 0]
total = [1120, 1120, 140, 4, 8]  # capacities
percentages = [17.41, 9.82, 25.00, 0.0, 0.0]

y_indices = np.arange(len(categories))[::-1]

# Total capacity background bar
ax.barh(y_indices, [100]*len(categories), height=0.45, color='#1e293b', edgecolor='#334155', linewidth=1.2, label='Free Capacity')
# Used capacity bar
bars = ax.barh(y_indices, percentages, height=0.45, color=['#0284c7', '#38bdf8', '#0ea5e9', '#64748b', '#64748b'],
               edgecolor='#ffffff', linewidth=0.8, label='Used by PPG Accelerator')

# Annotations
for i, y_idx in enumerate(y_indices):
    pct = percentages[i]
    u = used[i]
    tot = total[i]
    if pct > 0:
        ax.text(pct + 2, y_idx, f"{pct:.2f}% ({u} / {tot})", color='#ffffff', fontsize=8.5, weight='bold', va='center')
    else:
        ax.text(2, y_idx, f"0.00% (0 / {tot} — Pure Logic)", color='#94a3b8', fontsize=8.5, weight='bold', va='center')

ax.set_yticks(y_indices)
ax.set_yticklabels(categories, color='#f1f5f9', fontsize=9.5, weight='bold')
ax.set_xlabel("FPGA Fabric Utilization (%)", color='#94a3b8', fontsize=9.5, weight='bold', labelpad=10)
ax.set_xlim(0, 115)
ax.tick_params(colors='#94a3b8', labelsize=8.5)
ax.grid(axis='x', color='#334155', linestyle=':', alpha=0.6)

# Title & Subtitle
plt.title("Renesas ForgeFPGA (SLG47910C) Post-Synthesis Resource Footprint\n17.41% LUT Utilization — Zero DSP & Zero BRAM",
          color='#ffffff', fontsize=11, weight='bold', pad=15)

# Free fabric callout
ax.text(75, 1.0, "82.59% Logic Free\nReady for On-Chip\nExpanded DSP / Filters",
        color='#34d399', fontsize=8.5, weight='bold', ha='center', va='center',
        bbox=dict(boxstyle="round,pad=0.5", fc="#064e3b", ec="#10b981", lw=1.2))

plt.tight_layout()
util_path = os.path.join(img_dir, "forgefpga_utilization.png")
plt.savefig(util_path, dpi=180, facecolor=fig.get_facecolor(), bbox_inches='tight')
plt.close()
print(f"Saved {util_path}")
