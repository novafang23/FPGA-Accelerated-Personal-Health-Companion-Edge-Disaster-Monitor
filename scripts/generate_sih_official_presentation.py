"""
generate_sih_official_format_presentation.py
Generates the 100% official SIH 2026 template presentation (strictly 6 slides)
matching the official SIH template background, headers, logos, ovals, and blue footers.
"""

import os
from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.enum.text import PP_ALIGN
from pptx.dml.color import RGBColor
from pptx.enum.shapes import MSO_SHAPE

# ── Paths ────────────────────────────────────────────────────────────────────
ROOT_DIR = r"C:\Users\abhin\OneDrive\Desktop\verilog"
ASSETS_DIR = os.path.join(ROOT_DIR, "docs", "presentation", "template_assets")
DOCS_IMAGES = os.path.join(ROOT_DIR, "docs", "images")
OUT_PPTX = os.path.join(ROOT_DIR, "docs", "presentation", "SIH26181_Official_Template_Presentation.pptx")

SIH_LOGO = os.path.join(ASSETS_DIR, "page_1_img_2.png")
SIH_BRAIN = os.path.join(ASSETS_DIR, "page_1_img_1.png")
BLUE_FOOTER = os.path.join(ASSETS_DIR, "page_2_img_1.png")

# ── Color Palette ────────────────────────────────────────────────────────────
C_SIH_BLUE  = RGBColor(11, 98, 164)     # #0B62A4 (Official SIH Blue)
C_DARK_BLUE = RGBColor(30, 58, 138)     # #1E3A8A (Navy Blue)
C_BLACK     = RGBColor(0, 0, 0)
C_WHITE     = RGBColor(255, 255, 255)
C_CARD_BG   = RGBColor(248, 250, 252)   # #F8FAFC
C_BORDER    = RGBColor(203, 213, 225)   # #CBD5E1
C_TEXT_DARK = RGBColor(30, 41, 59)      # #1E293B
C_TEXT_MUTED= RGBColor(71, 85, 105)     # #475569
C_RED_WARN  = RGBColor(220, 38, 38)
C_TEAL      = RGBColor(13, 148, 136)
C_GREEN     = RGBColor(22, 163, 74)
C_ORANGE    = RGBColor(234, 88, 12)

prs = Presentation()
prs.slide_width = Inches(13.333)
prs.slide_height = Inches(7.5)
blank_layout = prs.slide_layouts[6]

def setup_official_slide(slide, title_text, slide_num):
    """Adds the exact SIH official template header, oval badge, logo, and blue footer."""
    # 1. Top Left: Team Name Oval
    oval = slide.shapes.add_shape(MSO_SHAPE.OVAL, Inches(0.4), Inches(0.25), Inches(1.5), Inches(0.9))
    oval.fill.solid()
    oval.fill.fore_color.rgb = C_WHITE
    oval.line.color.rgb = C_BLACK
    oval.line.width = Pt(1.5)
    tf_o = oval.text_frame
    p_o = tf_o.paragraphs[0]
    p_o.text = "Team\nChipsters"
    p_o.alignment = PP_ALIGN.CENTER
    p_o.font.name = "Arial"
    p_o.font.size = Pt(11)
    p_o.font.bold = True
    p_o.font.color.rgb = C_BLACK

    # 2. Centered Official Title
    tx_t = slide.shapes.add_textbox(Inches(2.2), Inches(0.35), Inches(8.8), Inches(0.8))
    tf_t = tx_t.text_frame
    p_t = tf_t.paragraphs[0]
    p_t.text = title_text
    p_t.alignment = PP_ALIGN.CENTER
    p_t.font.name = "Times New Roman"
    p_t.font.size = Pt(28)
    p_t.font.bold = True
    p_t.font.color.rgb = C_BLACK

    # 3. Top Right: Official SIH Logo
    if os.path.exists(SIH_LOGO):
        slide.shapes.add_picture(SIH_LOGO, Inches(11.3), Inches(0.18), Inches(1.65), Inches(0.78))

    # 4. Bottom Footer: Solid Blue Footer Bar
    if os.path.exists(BLUE_FOOTER):
        slide.shapes.add_picture(BLUE_FOOTER, Inches(0), Inches(7.05), Inches(13.333), Inches(0.45))
    else:
        bar = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0), Inches(7.05), Inches(13.333), Inches(0.45))
        bar.fill.solid(); bar.fill.fore_color.rgb = C_SIH_BLUE; bar.line.fill.background()

    # 5. Footer Text Overlay
    tx_f = slide.shapes.add_textbox(Inches(1.0), Inches(7.1), Inches(11.333), Inches(0.35))
    tf_f = tx_f.text_frame
    p_f = tf_f.paragraphs[0]
    p_f.text = f"@SIH Idea submission- Template                                                                                                {slide_num}"
    p_f.alignment = PP_ALIGN.CENTER
    p_f.font.name = "Arial"
    p_f.font.size = Pt(10.5)
    p_f.font.color.rgb = C_WHITE

def add_clean_box(slide, left, top, width, height, bg_color=C_WHITE, border_color=C_BORDER):
    card = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left, top, width, height)
    card.fill.solid()
    card.fill.fore_color.rgb = bg_color
    card.line.color.rgb = border_color
    card.line.width = Pt(1.5)
    return card

# =============================================================================
# SLIDE 1: TITLE PAGE (Exact Official SIH Template)
# =============================================================================
s1 = prs.slides.add_slide(blank_layout)

# Centered Top: SMART INDIA HACKATHON 2026
tx_sih = s1.shapes.add_textbox(Inches(1.0), Inches(0.35), Inches(10.0), Inches(0.6))
p = tx_sih.text_frame.paragraphs[0]
p.text = "SMART INDIA HACKATHON 2026"
p.alignment = PP_ALIGN.CENTER
p.font.name = "Times New Roman"
p.font.size = Pt(32)
p.font.bold = True
p.font.color.rgb = C_DARK_BLUE

# Centered Below: TITLE PAGE
tx_tp = s1.shapes.add_textbox(Inches(1.0), Inches(1.1), Inches(10.0), Inches(0.6))
p = tx_tp.text_frame.paragraphs[0]
p.text = "TITLE PAGE"
p.alignment = PP_ALIGN.CENTER
p.font.name = "Times New Roman"
p.font.size = Pt(26)
p.font.bold = True
p.font.color.rgb = C_BLACK

# Top Right: SIH Logo
if os.path.exists(SIH_LOGO):
    s1.shapes.add_picture(SIH_LOGO, Inches(11.2), Inches(0.2), Inches(1.75), Inches(0.82))

# Left Details Box (Exact SIH Pointers)
tx_info = s1.shapes.add_textbox(Inches(0.8), Inches(2.1), Inches(7.5), Inches(4.8))
tf_info = tx_info.text_frame
tf_info.word_wrap = True

def add_info_line(tf, label, value, is_first=False):
    p = tf.paragraphs[0] if is_first else tf.add_paragraph()
    p.text = f"• {label} - "
    p.font.name = "Arial"
    p.font.size = Pt(14)
    p.font.bold = True
    p.font.color.rgb = C_BLACK
    r = p.add_run()
    r.text = value
    r.font.bold = False
    r.font.color.rgb = C_TEXT_DARK
    p.space_after = Pt(14)

add_info_line(tf_info, "Problem Statement ID", "26181", is_first=True)
add_info_line(tf_info, "Problem Statement Title", "AI-Powered Personal Health Companion & Edge Disaster Monitor")
add_info_line(tf_info, "Theme", "MedTech")
add_info_line(tf_info, "PS Category", "Hardware")
add_info_line(tf_info, "Team ID", "01")
add_info_line(tf_info, "Team Name (Registered on portal)", "CHIPSTERS")

# Right: Official Brain-Bulb Graphic + Embedded Wearable Render
if os.path.exists(SIH_BRAIN):
    s1.shapes.add_picture(SIH_BRAIN, Inches(8.3), Inches(1.8), Inches(4.3), Inches(4.6))

# High-impact highlight banner at bottom of Slide 1
ban1 = s1.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(0.8), Inches(6.5), Inches(7.2), Inches(0.6))
ban1.fill.solid(); ban1.fill.fore_color.rgb = RGBColor(238, 242, 255)
ban1.line.color.rgb = C_SIH_BLUE
p = ban1.text_frame.paragraphs[0]
p.text = "⚡ 20 ns FPGA Timing  |  🧠 91% INT8 TinyML  |  🏥 92% MIMIC-III Validated"
p.alignment = PP_ALIGN.CENTER
p.font.name = "Arial"
p.font.size = Pt(11)
p.font.bold = True
p.font.color.rgb = C_DARK_BLUE

# =============================================================================
# SLIDE 2: IDEA TITLE & PROPOSED SOLUTION
# =============================================================================
s2 = prs.slides.add_slide(blank_layout)
setup_official_slide(s2, "IDEA TITLE", slide_num=2)

# Official Section Prompt
tx_pr = s2.shapes.add_textbox(Inches(0.8), Inches(1.2), Inches(11.7), Inches(0.5))
p = tx_pr.text_frame.paragraphs[0]
p.text = "❖ Proposed Solution (Describe your Idea/Solution/Prototype)"
p.font.name = "Arial"
p.font.size = Pt(16)
p.font.bold = True
p.font.color.rgb = C_SIH_BLUE

# Left Card: Detailed Problem & The Smartwatch Blind Spot
add_clean_box(s2, Inches(0.8), Inches(1.75), Inches(5.65), Inches(4.4), bg_color=C_WHITE, border_color=C_RED_WARN)
ban_p = s2.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0.8), Inches(1.75), Inches(5.65), Inches(0.45))
ban_p.fill.solid(); ban_p.fill.fore_color.rgb = C_RED_WARN; ban_p.line.fill.background()
p = ban_p.text_frame.paragraphs[0]; p.text = "❌ How Current Technology Fails During Disasters"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_pb = s2.shapes.add_textbox(Inches(1.0), Inches(2.25), Inches(5.25), Inches(3.8))
tf_pb = tx_pb.text_frame
tf_pb.word_wrap = True

def add_bullet_p(tf, title, desc, first=False):
    p = tf.paragraphs[0] if first else tf.add_paragraph()
    p.text = "• " + title + ": "
    p.font.name = "Arial"
    p.font.bold = True
    p.font.size = Pt(11)
    p.font.color.rgb = C_TEXT_DARK
    r = p.add_run()
    r.text = desc
    r.font.bold = False
    r.font.color.rgb = C_TEXT_MUTED
    p.space_after = Pt(7)

add_bullet_p(tf_pb, "Wearables Are Environmentally Blind", "Commercial smartwatches track fitness steps and resting heart rate, but have zero awareness if ambient air is toxic (AQI 500+) or if 48°C heat is causing heat exhaustion.", first=True)
add_bullet_p(tf_pb, "Environmental Sensors Ignore the Body", "Weather apps report regional city forecasts, not what an individual worker's cardiovascular system is actually enduring at street level.", first=False)
add_bullet_p(tf_pb, "The Cloud Dependency Trap", "Cloud AI health platforms require mobile networks. During floods, cyclones, or power grid failures, cellular towers collapse and cloud health apps die completely.", first=False)
add_bullet_p(tf_pb, "Software Jitter Destroys HRV", "Operating system scheduling delays on standard CPUs jitter by 5-20 ms, severely corrupting heart rate variability (HRV) metrics.", first=False)

# Right Card: Detailed Proposed Solution & Innovation
add_clean_box(s2, Inches(6.85), Inches(1.75), Inches(5.65), Inches(4.4), bg_color=C_WHITE, border_color=C_GREEN)
ban_s = s2.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(6.85), Inches(1.75), Inches(5.65), Inches(0.45))
ban_s.fill.solid(); ban_s.fill.fore_color.rgb = C_GREEN; ban_s.line.fill.background()
p = ban_s.text_frame.paragraphs[0]; p.text = "✅ EdgeGuard Proposed Solution & Innovation"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_sb = s2.shapes.add_textbox(Inches(7.05), Inches(2.25), Inches(5.25), Inches(3.8))
tf_sb = tx_sb.text_frame
tf_sb.word_wrap = True

add_bullet_p(tf_sb, "Detailed Solution Explanation", "Simultaneously acquires dual-wavelength optical PPG pulses along with hyper-local ambient temperature, relative humidity, and laser PM2.5 particulate matter on the wrist.", first=True)
add_bullet_p(tf_sb, "How It Addresses the Problem", "Fuses bodily vitals with environmental stress in real time to calculate cardiac drift and respiratory strain, alerting workers 15-20 minutes before medical collapse.", first=False)
add_bullet_p(tf_sb, "Hardware Innovation (FPGA Silicon)", "Custom Verilog 8-tap filter runs on 50 MHz hardware with 20 ns cycle resolution, completely eliminating operating system jitter without DSP multipliers or BRAM.", first=False)
add_bullet_p(tf_sb, "Uniqueness & Zero Cloud Dependency", "Quantized INT8 neural network fits into only 619 bytes SRAM and evaluates risks in 42 µs, providing 100% offline protection during infrastructure blackouts.", first=False)

# Bottom Takeaway Box
box_tw = s2.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(0.8), Inches(6.25), Inches(11.7), Inches(0.65))
box_tw.fill.solid(); box_tw.fill.fore_color.rgb = RGBColor(240, 249, 255)
box_tw.line.color.rgb = C_SIH_BLUE
p = box_tw.text_frame.paragraphs[0]
p.text = "💡 Layman Core: An offline medical guardian that connects what's happening outside (disaster) with what's happening inside (your heart) — saving lives before emergency collapse."
p.alignment = PP_ALIGN.CENTER
p.font.name = "Arial"
p.font.size = Pt(10.5)
p.font.bold = True
p.font.color.rgb = C_SIH_BLUE

# =============================================================================
# SLIDE 3: TECHNICAL APPROACH
# =============================================================================
s3 = prs.slides.add_slide(blank_layout)
setup_official_slide(s3, "TECHNICAL APPROACH", slide_num=3)

# Sub-header prompt
tx_t_prompt = s3.shapes.add_textbox(Inches(0.8), Inches(1.2), Inches(11.7), Inches(0.4))
p = tx_t_prompt.text_frame.paragraphs[0]
p.text = "❖ Technologies Used & Implementation Methodology (4-Stage Autonomous Dataflow)"
p.font.name = "Arial"
p.font.size = Pt(14)
p.font.bold = True
p.font.color.rgb = C_SIH_BLUE

# 4 Pipeline Stage Cards
col_w = Inches(2.75)
col_gap = Inches(0.24)
start_x = Inches(0.8)
card_y = Inches(1.65)
card_h = Inches(3.5)

stages_s3 = [
    ("Stage 01: Sensing", "Physical Acquisition", C_DARK_BLUE, [
        ("MAX30102 PPG", "Dual Red & IR optical pulse oximeter for arterial blood flow & SpO2."),
        ("BME280 Climate", "Ambient temperature & relative humidity for Steadman Heat Index."),
        ("PMSA003 PM2.5", "Laser scattering sensor measuring hazardous fine particulate smog.")
    ]),
    ("Stage 02: FPGA RTL", "50 MHz Custom Silicon", C_TEAL, [
        ("Dual 8-Tap Filter", "O(1) running-sum noise filter with bit-shift (>> 3). 0 DSP, 0 BRAM."),
        ("Systolic Peak FSM", "4-state finite state machine with 250 ms refractory lock-out window."),
        ("20 ns Cycle Timer", "Continuous 50 MHz cycle counter latching IBI with zero software jitter.")
    ]),
    ("Stage 03: Clinical AI", "On-Device TinyML", C_GREEN, [
        ("mNEWS2 Engine", "Royal College of Physicians clinical early warning triage protocol."),
        ("Karlen/Elgendi SQI", "Signal Quality Index rejecting motion noise and false beats."),
        ("6→24→16→3 Model", "INT8 deep micro-network running in 42 µs (91.00% validation accuracy).")
    ]),
    ("Stage 04: Edge Action", "Alerts & Telemetry", C_ORANGE, [
        ("Local OLED Display", "128x64 display of live HR, SpO2, RR, and multi-hazard risk tiers."),
        ("Haptic Alarm", "Vibration motor & emergency buzzer for immediate worker evacuation."),
        ("WiFi / MQTT Mesh", "Optional emergency beacon broadcasting vitals to disaster rescue teams.")
    ])
]

for i, (stg_num, stg_sub, col_color, items) in enumerate(stages_s3):
    cx = start_x + i * (col_w + col_gap)
    card = add_clean_box(s3, cx, card_y, col_w, card_h, bg_color=C_WHITE, border_color=col_color)
    
    top_ban = s3.shapes.add_shape(MSO_SHAPE.RECTANGLE, cx, card_y, col_w, Inches(0.65))
    top_ban.fill.solid(); top_ban.fill.fore_color.rgb = col_color; top_ban.line.fill.background()
    p1 = top_ban.text_frame.paragraphs[0]; p1.text = stg_num; p1.font.size = Pt(11); p1.font.bold = True; p1.font.color.rgb = C_WHITE; p1.alignment = PP_ALIGN.CENTER
    p2 = top_ban.text_frame.add_paragraph(); p2.text = stg_sub; p2.font.size = Pt(9); p2.font.color.rgb = RGBColor(226, 232, 240); p2.alignment = PP_ALIGN.CENTER
    
    tx_b = s3.shapes.add_textbox(cx + Inches(0.1), card_y + Inches(0.7), col_w - Inches(0.2), card_h - Inches(0.8))
    tf_b = tx_b.text_frame
    tf_b.word_wrap = True
    for j, (it_title, it_desc) in enumerate(items):
        p = tf_b.paragraphs[0] if j == 0 else tf_b.add_paragraph()
        p.text = "• " + it_title
        p.font.name = "Arial"
        p.font.bold = True
        p.font.size = Pt(10)
        p.font.color.rgb = C_TEXT_DARK
        
        pr = tf_b.add_paragraph()
        pr.text = it_desc
        pr.font.name = "Arial"
        pr.font.size = Pt(8.5)
        pr.font.color.rgb = C_TEXT_MUTED
        pr.space_after = Pt(4)

# Bottom Box: Embedded RTL Waveform Evidence
box_wf = add_clean_box(s3, Inches(0.8), Inches(5.25), Inches(11.7), Inches(1.65), bg_color=C_WHITE, border_color=C_SIH_BLUE)
wf_img = os.path.join(DOCS_IMAGES, "waveform_snapshot.png")
if os.path.exists(wf_img):
    s3.shapes.add_picture(wf_img, Inches(0.95), Inches(5.35), Inches(4.3), Inches(1.45))

tx_wdesc = s3.shapes.add_textbox(Inches(5.4), Inches(5.3), Inches(7.0), Inches(1.5))
tf_wdesc = tx_wdesc.text_frame
tf_wdesc.word_wrap = True

add_bullet_p(tf_wdesc, "Verified Cycle-Accurate Timing", "GTKWave trace proves O(1) noise filter convergence and systolic peak latching with 20 ns resolution.", first=True)
add_bullet_p(tf_wdesc, "Zero OS Scheduling Jitter", "Unlike software CPUs with 5-20 ms latency, the 50 MHz FPGA timer gives 100% medically reliable Heart Rate Variability (HRV).", first=False)
add_bullet_p(tf_wdesc, "Hardware Technologies", "Synthesizable Verilog-2001 (Renesas ForgeFPGA + Xilinx Zynq-7000), C99 bare-metal / FreeRTOS, INT8 TinyML, Win32 GDI GUI.", first=False)

# =============================================================================
# SLIDE 4: FEASIBILITY AND VIABILITY
# =============================================================================
s4 = prs.slides.add_slide(blank_layout)
setup_official_slide(s4, "FEASIBILITY AND VIABILITY", slide_num=4)

# Left Column: Feasibility Analysis & Hardware Stats
add_clean_box(s4, Inches(0.8), Inches(1.35), Inches(5.65), Inches(5.5), bg_color=C_WHITE, border_color=C_SIH_BLUE)
ban_f = s4.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0.8), Inches(1.35), Inches(5.65), Inches(0.45))
ban_f.fill.solid(); ban_f.fill.fore_color.rgb = C_SIH_BLUE; ban_f.line.fill.background()
p = ban_f.text_frame.paragraphs[0]; p.text = "📊 Feasibility Analysis: Hardware, Silicon & Economics"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_fa = s4.shapes.add_textbox(Inches(1.0), Inches(1.85), Inches(5.25), Inches(4.8))
tf_fa = tx_fa.text_frame
tf_fa.word_wrap = True

add_bullet_p(tf_fa, "Vendor-Agnostic Verilog RTL", "Identical synthesizable core Verilog runs on both AMD Xilinx (6-input LUT) and Renesas ForgeFPGA (5-input LUT) architectures without redesign.", first=True)
add_bullet_p(tf_fa, "Ultra-Low Silicon Footprint", "Synthesized on Renesas ForgeFPGA requiring only 195 / 1120 LUT5s (17.4% logic utilization) with 0 DSP multipliers and 0 Block RAM.", first=False)
add_bullet_p(tf_fa, "Static Timing Closure (+5.603 ns WNS)", "Proven static timing slack closure in AMD Xilinx Vivado ML (+5.603 ns setup slack, operating safely at 69.45 MHz on a 50 MHz target clock).", first=False)
add_bullet_p(tf_fa, "Economic Viability & Bulk BOM", "• Renesas ForgeFPGA ($1.80) + ESP32-S3 ($2.20)\n• Optical PPG + Climate + Laser PM2.5 ($5.80)\n• OLED, Battery & 3D Enclosure ($4.20)\n=> Total Mass-Production BOM: Under $15 (~₹1,250)", first=False)

# Right Column: Potential Challenges & Mitigations Table
add_clean_box(s4, Inches(6.85), Inches(1.35), Inches(5.65), Inches(5.5), bg_color=C_WHITE, border_color=C_DARK_BLUE)
ban_c = s4.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(6.85), Inches(1.35), Inches(5.65), Inches(0.45))
ban_c.fill.solid(); ban_c.fill.fore_color.rgb = C_DARK_BLUE; ban_c.line.fill.background()
p = ban_c.text_frame.paragraphs[0]; p.text = "🛡️ Potential Challenges, Risks & Mitigation Strategies"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_ca = s4.shapes.add_textbox(Inches(7.05), Inches(1.85), Inches(5.25), Inches(4.8))
tf_ca = tx_ca.text_frame
tf_ca.word_wrap = True

add_bullet_p(tf_ca, "Challenge 1: Motion & Optical Noise", "Mitigation -> Dual 8-tap running-sum digital filter on FPGA plus Karlen/Elgendi Signal Quality Index (SQI) algorithm automatically discards corrupted beats.", first=True)
add_bullet_p(tf_ca, "Challenge 2: Micro-FPGA Resource Limits", "Mitigation -> Pure logic O(1) bit-shift division (>> 3) requires 0 DSP48 multipliers and 0 Block RAM, running on the lowest-cost micro-FPGAs.", first=False)
add_bullet_p(tf_ca, "Challenge 3: Grid & Cellular Tower Outages", "Mitigation -> 100% on-device offline TinyML running in 619 bytes SRAM. Operates autonomously during catastrophic floods and storms without internet.", first=False)
add_bullet_p(tf_ca, "Challenge 4: Clinical Real-World Accuracy", "Mitigation -> Benchmarked against 16,387 gold-standard patient records from the MIT MIMIC-III ICU database (achieving 92.05% clinical concordance).", first=False)
add_bullet_p(tf_ca, "Commercial Roadmap Target", "Direct migration path mapped to Qualcomm Snapdragon Wear W5+ Gen 1 Low-Power Island (<5 mW continuous power).", first=False)

# =============================================================================
# SLIDE 5: IMPACT AND BENEFITS
# =============================================================================
s5 = prs.slides.add_slide(blank_layout)
setup_official_slide(s5, "IMPACT AND BENEFITS", slide_num=5)

# Left Column: Target Audience Impact (India Focus)
add_clean_box(s5, Inches(0.8), Inches(1.35), Inches(5.65), Inches(5.5), bg_color=C_WHITE, border_color=C_ORANGE)
ban_i = s5.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0.8), Inches(1.35), Inches(5.65), Inches(0.45))
ban_i.fill.solid(); ban_i.fill.fore_color.rgb = C_ORANGE; ban_i.line.fill.background()
p = ban_i.text_frame.paragraphs[0]; p.text = "🇮🇳 Potential Impact on Target Audience (India 2026)"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_ia = s5.shapes.add_textbox(Inches(1.0), Inches(1.85), Inches(5.25), Inches(4.8))
tf_ia = tx_ia.text_frame
tf_ia.word_wrap = True

add_bullet_p(tf_ia, "☀️ 380+ Million Outdoor Laborers", "Protects construction workers, agricultural laborers, and delivery partners during 48°C heatwaves in Rajasthan, UP, and central India by warning 15-20 minutes before heat exhaustion collapse.", first=True)
add_bullet_p(tf_ia, "🌫️ Vulnerable Urban Populations (Delhi NCR)", "Warns traffic police, elderly individuals, and asthmatic patients when micro-pocket PM2.5 exceeds safe autonomic cardiac thresholds during severe winter smog events.", first=False)
add_bullet_p(tf_ia, "🌊 Monsoon Flood Victims & First Responders", "Detects cold-water immersion hyperventilation and hypothermia during power grid and mobile network blackouts in flood-hit urban zones (Mumbai, Chennai, Assam).", first=False)
add_bullet_p(tf_ia, "🏥 Low-Income Accessibility", "Brings high-grade clinical early warning monitoring to vulnerable daily-wage workers who cannot afford premium $400 consumer smartwatches.", first=False)

# Right Column: Multi-Dimensional Benefits (Social, Economic, Environmental)
add_clean_box(s5, Inches(6.85), Inches(1.35), Inches(5.65), Inches(5.5), bg_color=C_WHITE, border_color=C_GREEN)
ban_b = s5.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(6.85), Inches(1.35), Inches(5.65), Inches(0.45))
ban_b.fill.solid(); ban_b.fill.fore_color.rgb = C_GREEN; ban_b.line.fill.background()
p = ban_b.text_frame.paragraphs[0]; p.text = "🌱 Benefits: Social, Economic & Environmental"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_ba = s5.shapes.add_textbox(Inches(7.05), Inches(1.85), Inches(5.25), Inches(4.8))
tf_ba = tx_ba.text_frame
tf_ba.word_wrap = True

add_bullet_p(tf_ba, "Social Benefits", "• Zero-cloud biometric privacy (health data stays on the user's wrist).\n• Functions autonomously during disaster grid blackouts.\n• Prevents emergency hospitalizations and permanent organ damage.", first=True)
add_bullet_p(tf_ba, "Economic Benefits", "• Sub-$15 bill of materials enables mass deployment by municipal bodies, police departments, and construction enterprises.\n• Drastically cuts lost workdays and out-of-pocket emergency medical expenses for low-income families.", first=False)
add_bullet_p(tf_ba, "Environmental & Public Health Benefits", "• Delivers hyper-local microclimate and air quality monitoring to guide targeted municipal disaster response and evacuation.\n• Ultra-low-power <5 mW micro-FPGA architecture extends battery longevity and minimizes electronic waste.", first=False)

# =============================================================================
# SLIDE 6: RESEARCH AND REFERENCES
# =============================================================================
s6 = prs.slides.add_slide(blank_layout)
setup_official_slide(s6, "RESEARCH AND REFERENCES", slide_num=6)

# Left Column: Clinical & Scientific Foundations
add_clean_box(s6, Inches(0.8), Inches(1.35), Inches(5.65), Inches(5.5), bg_color=C_WHITE, border_color=C_SIH_BLUE)
ban_r = s6.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0.8), Inches(1.35), Inches(5.65), Inches(0.45))
ban_r.fill.solid(); ban_r.fill.fore_color.rgb = C_SIH_BLUE; ban_r.line.fill.background()
p = ban_r.text_frame.paragraphs[0]; p.text = "📚 Clinical & Scientific Foundations (Peer-Reviewed)"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_ra = s6.shapes.add_textbox(Inches(1.0), Inches(1.85), Inches(5.25), Inches(4.8))
tf_ra = tx_ra.text_frame
tf_ra.word_wrap = True

add_bullet_p(tf_ra, "Royal College of Physicians (UK)", "National Early Warning Score (mNEWS2): Standardising the assessment of acute-illness severity in the NHS (respiratory, pulse & SpO2 triage).", first=True)
add_bullet_p(tf_ra, "Prof. Daniel S. Moran (1998)", "'An Evaluated Physiological Strain Index in Human Heat Stress', American Journal of Physiology (Heart Rate + Core Temperature cardio-thermal formula).", first=False)
add_bullet_p(tf_ra, "Prof. Robert D. Brook / AHA (2010)", "'Particulate Matter Air Pollution and Cardiovascular Disease', Circulation (Scientific statement on PM2.5 autonomic vagal suppression).", first=False)
add_bullet_p(tf_ra, "R. G. Steadman (1979)", "'The Assessment of Sultriness: Part I & II', Journal of Applied Meteorology (Biometeorological formulation of the Steadman Heat Index).", first=False)
add_bullet_p(tf_ra, "Karlen et al. (2012) & Elgendi (2016)", "'Photoplethysmogram Signal Quality Estimation for Pulse Oximetry' (Automated SQI classification of optical pulse morphology).", first=False)

# Right Column: Datasets, Standards & Commercial Target
add_clean_box(s6, Inches(6.85), Inches(1.35), Inches(5.65), Inches(5.5), bg_color=C_WHITE, border_color=C_DARK_BLUE)
ban_d = s6.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(6.85), Inches(1.35), Inches(5.65), Inches(0.45))
ban_d.fill.solid(); ban_d.fill.fore_color.rgb = C_DARK_BLUE; ban_d.line.fill.background()
p = ban_d.text_frame.paragraphs[0]; p.text = "🔬 Datasets, Standards & Commercial Deployment Target"; p.font.size = Pt(11.5); p.font.bold = True; p.font.color.rgb = C_WHITE

tx_da = s6.shapes.add_textbox(Inches(7.05), Inches(1.85), Inches(5.25), Inches(4.8))
tf_da = tx_da.text_frame
tf_da.word_wrap = True

add_bullet_p(tf_da, "MIT / PhysioNet MIMIC-III Database", "Medical Information Mart for Intensive Care v1.4: 16,387 gold-standard arterial hemodynamic records evaluated with 92.05% clinical concordance.", first=True)
add_bullet_p(tf_da, "World Health Organization (WHO 2021)", "WHO Global Air Quality Guidelines: Particulate matter (PM2.5) 24-hour and annual hazard threshold standards.", first=False)
add_bullet_p(tf_da, "ARM AMBA AXI4-Lite Specification", "ARM IHI0022E: Interconnect standard for decoupled AW/W register channels and Write-1-to-Clear (W1C) interrupt status handling.", first=False)
add_bullet_p(tf_da, "Qualcomm Snapdragon Wear Roadmap", "Architecture designed for migration to Snapdragon Wear W5+ Gen 1, mapping Verilog DSP to Hexagon™ Vector eXtensions (HVX) on the <5 mW Low-Power Island.", first=False)
add_bullet_p(tf_da, "EDA & Synthesis Verification Tools", "AMD Xilinx Vivado ML v2022.2 (Timing Closure), Renesas ForgeFPGA Workshop, Icarus Verilog, GTKWave, GCC, ESP-IDF v5.x (FreeRTOS).", first=False)

# Save
prs.save(OUT_PPTX)
print(f"[SUCCESS] Official SIH template presentation saved cleanly at: {OUT_PPTX}")
