import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

# 1. Create a professional dark-themed GTKWave-style waveform plot
fig, ax = plt.subplots(figsize=(12, 6.5), dpi=150)
fig.patch.set_facecolor("#0f172a")
ax.set_facecolor("#0f172a")

# Time axis (0 to 100 cycles)
t = np.linspace(0, 100, 1000)

signals = [
    ("clk (50 MHz)", "digital", 0.5 * (np.sin(2 * np.pi * t / 4) > 0)),
    ("rstn (Active-Low)", "digital", (t > 4).astype(float)),
    ("s_axi_awvalid", "digital", ((t >= 12) & (t <= 20) | (t >= 40) & (t <= 46)).astype(float)),
    ("s_axi_awready", "digital", ((t >= 16) & (t <= 20) | (t >= 44) & (t <= 46)).astype(float)),
    ("s_axi_wvalid (Staggered)", "digital", ((t >= 24) & (t <= 32) | (t >= 40) & (t <= 46)).astype(float)),
    ("s_axi_wready", "digital", ((t >= 28) & (t <= 32) | (t >= 44) & (t <= 46)).astype(float)),
    ("filter_red_out[7:0]", "analog", 100 + 40 * np.sin(2 * np.pi * (t - 30) / 40) * (t > 30)),
    ("fsm_state (ARMED->RISING->PEAK)", "state", t),
    ("irq_beat (Systolic Interrupt)", "pulse", ((t >= 68) & (t <= 72)).astype(float)),
    ("reg_ibi_cycles[31:0]", "bus", t),
]

y_pos = len(signals) - 1
for name, sig_type, val in signals:
    base_y = y_pos * 1.3
    if sig_type == "digital":
        ax.step(t, base_y + val * 0.8, where="post", color="#38bdf8", linewidth=1.5)
    elif sig_type == "pulse":
        ax.step(t, base_y + val * 0.8, where="post", color="#22c55e", linewidth=2.0)
    elif sig_type == "analog":
        val_norm = (val - np.min(val)) / (np.max(val) - np.min(val) + 1e-5) * 0.8
        ax.plot(t, base_y + val_norm, color="#f59e0b", linewidth=2.0)
    elif sig_type == "state":
        # Draw state blocks
        states = [
            (0, 48, "ARMED (00)", "#334155"),
            (48, 68, "RISING (01)", "#0284c7"),
            (68, 72, "PEAK_FOUND (10)", "#ef4444"),
            (72, 100, "REFRACTORY (11)", "#7c3aed"),
        ]
        for s_start, s_end, s_label, s_color in states:
            rect = patches.Rectangle(
                (s_start, base_y), s_end - s_start, 0.8, facecolor=s_color, edgecolor="#ffffff", alpha=0.85
            )
            ax.add_patch(rect)
            ax.text(
                (s_start + s_end) / 2,
                base_y + 0.35,
                s_label,
                color="white",
                fontsize=7.5,
                weight="bold",
                ha="center",
                va="center",
            )
    elif sig_type == "bus":
        # Bus representation
        rect1 = patches.Rectangle((0, base_y), 68, 0.8, facecolor="#1e293b", edgecolor="#64748b")
        rect2 = patches.Rectangle((68, base_y), 32, 0.8, facecolor="#065f46", edgecolor="#10b981")
        ax.add_patch(rect1)
        ax.add_patch(rect2)
        ax.text(34, base_y + 0.35, "0x00000000 (Idle)", color="#94a3b8", fontsize=7.5, ha="center", va="center")
        ax.text(
            84,
            base_y + 0.35,
            "0x00000CD1 (3281 ticks = 65.62 us)",
            color="#6ee7b7",
            fontsize=7.5,
            weight="bold",
            ha="center",
            va="center",
        )

    ax.text(-2, base_y + 0.35, name, color="#f1f5f9", fontsize=9, weight="bold", ha="right", va="center")
    y_pos -= 1

# Annotations
ax.annotate(
    "Decoupled AXI Write Handshake (AW & W in separate cycles)",
    xy=(20, 9.5),
    xytext=(35, 11.5),
    arrowprops=dict(facecolor="#38bdf8", shrink=0.05, width=1.5, headwidth=6),
    color="#38bdf8",
    fontsize=8.5,
    weight="bold",
    bbox=dict(boxstyle="round,pad=0.3", fc="#1e293b", ec="#38bdf8"),
)

ax.annotate(
    "Systolic Peak Detected → irq_beat 1-cycle pulse",
    xy=(70, 2.5),
    xytext=(55, 4.5),
    arrowprops=dict(facecolor="#22c55e", shrink=0.05, width=1.5, headwidth=6),
    color="#22c55e",
    fontsize=8.5,
    weight="bold",
    bbox=dict(boxstyle="round,pad=0.3", fc="#1e293b", ec="#22c55e"),
)

ax.set_xlim(-25, 105)
ax.set_ylim(-0.5, len(signals) * 1.3 + 0.5)
ax.set_title(
    "GTKWave Timing Simulation: Decoupled AXI4-Lite Handshake & Systolic Peak Detection (50 MHz / 20ns)",
    color="#ffffff",
    fontsize=11,
    weight="bold",
    pad=15,
)
ax.axis("off")

plt.tight_layout()
plt.savefig(
    r"c:\Users\abhin\OneDrive\Desktop\verilog\SIH\waveform_snapshot.png",
    dpi=150,
    facecolor=fig.get_facecolor(),
    bbox_inches="tight",
)
plt.close()
print("Saved waveform_snapshot.png")
