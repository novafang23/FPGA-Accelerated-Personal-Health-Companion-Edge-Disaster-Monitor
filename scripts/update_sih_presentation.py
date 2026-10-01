import pptx
from pptx.util import Inches, Pt
from pptx.enum.text import PP_ALIGN
from pptx.dml.color import RGBColor
from pptx.enum.shapes import MSO_SHAPE
import os
import sys

sys.stdout.reconfigure(encoding='utf-8')

input_path = sys.argv[1] if len(sys.argv) > 1 else 'docs/presentation/FINAL_FINAL_v6.pptx'
prs = pptx.Presentation(input_path)

# -------------------------------------------------------------
# Color Palette
# -------------------------------------------------------------
COLOR_PRIMARY_NAVY = RGBColor(20, 45, 85)     # #142D55
COLOR_SECONDARY_BLUE = RGBColor(0, 114, 206)  # #0072CE
COLOR_ACCENT_TEAL = RGBColor(0, 168, 143)     # #00A88F
COLOR_DARK_TEXT = RGBColor(30, 41, 59)        # #1E293B
COLOR_MUTED_TEXT = RGBColor(71, 85, 105)      # #475569
COLOR_CARD_BG = RGBColor(248, 250, 252)       # #F8FAFC
COLOR_CARD_BORDER = RGBColor(203, 213, 225)   # #CBD5E1
COLOR_WHITE = RGBColor(255, 255, 255)
COLOR_SUCCESS_GREEN = RGBColor(16, 124, 65)   # #107C41
COLOR_LIGHT_GREEN = RGBColor(240, 253, 244)   # #F0FDF4
COLOR_GREEN_BORDER = RGBColor(187, 247, 208)  # #BBF7D0

# -------------------------------------------------------------
# 1. Update Slide 1: Badge text alignment
# -------------------------------------------------------------
slide1 = prs.slides[0]
for shape in slide1.shapes:
    if shape.has_text_frame and "20 ns FPGA Timing" in shape.text_frame.text:
        shape.text_frame.paragraphs[0].text = "⚡ 20 ns FPGA Timing   |   🧠 619 B INT8 TinyML   |   🏥 94.11% MIMIC-III Validated   |   🌐 100% Offline Edge"
        shape.text_frame.paragraphs[0].font.size = Pt(13)
        shape.text_frame.paragraphs[0].font.bold = True
        shape.text_frame.paragraphs[0].font.color.rgb = COLOR_PRIMARY_NAVY
        print("Updated Slide 1 badge text.")

# -------------------------------------------------------------
# 2. Update Slide 5: Add Target Indian Disaster Context
# -------------------------------------------------------------
slide5 = prs.slides[4]
for shape in slide5.shapes:
    if shape.has_text_frame and "Implements the NHS early-warning score" in shape.text_frame.text:
        tf = shape.text_frame
        tf.clear()
        p = tf.paragraphs[0]
        p.text = "🇮🇳 Tailored for Indian Disasters: Protects 380M+ outdoor workers in North India heatwaves (48°C) • Indo-Gangetic toxic smog (AQI 500+) • Monsoon flash floods & blackouts"
        p.font.size = Pt(11)
        p.font.bold = True
        p.alignment = PP_ALIGN.CENTER
        p.font.color.rgb = COLOR_PRIMARY_NAVY
        print("Updated Slide 5 bottom banner with Indian disaster context.")

# -------------------------------------------------------------
# 3. Transform Slide 6: Working Prototype Showcase + References
# -------------------------------------------------------------
slide6 = prs.slides[5]

# Remove old Table 6 and old footer text box (Shape 5 and 6)
# Identify shapes to delete: keep official template shapes (0: oval, 1: title, 2: logo, 3: footer bar, 4: footer text)
shapes_to_remove = []
for idx, shape in enumerate(slide6.shapes):
    if idx >= 5:  # Old table and text box
        shapes_to_remove.append(shape)

for sp in shapes_to_remove:
    sp_elem = sp._element
    sp_elem.getparent().remove(sp_elem)

print(f"Removed {len(shapes_to_remove)} old shapes from Slide 6.")

# --- LEFT COLUMN: Working Prototype Showcase ---
left_col_x = Inches(0.80)
left_col_w = Inches(5.60)

# Section Header 1
h1 = slide6.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left_col_x, Inches(1.30), left_col_w, Inches(0.40))
h1.fill.solid()
h1.fill.fore_color.rgb = COLOR_PRIMARY_NAVY
h1.line.color.rgb = COLOR_PRIMARY_NAVY
tf = h1.text_frame
tf.word_wrap = True
p = tf.paragraphs[0]
p.text = "🔬 WORKING PROTOTYPE & HARDWARE SYSTEM"
p.font.size = Pt(11)
p.font.bold = True
p.font.color.rgb = COLOR_WHITE
p.alignment = PP_ALIGN.CENTER

# Prototype Box 1: CAD Hardware Wearable
card1 = slide6.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left_col_x, Inches(1.80), left_col_w, Inches(2.15))
card1.fill.solid()
card1.fill.fore_color.rgb = COLOR_CARD_BG
card1.line.color.rgb = COLOR_CARD_BORDER

# Image 1: sih_hero_render.jpg
hero_img_path = 'docs/images/sih_hero_render.jpg'
if os.path.exists(hero_img_path):
    slide6.shapes.add_picture(hero_img_path, left_col_x + Inches(0.12), Inches(1.90), width=Inches(2.70), height=Inches(1.51))

# Caption / Bullet points for Card 1
tb1 = slide6.shapes.add_textbox(left_col_x + Inches(2.90), Inches(1.85), Inches(2.60), Inches(2.00))
tf1 = tb1.text_frame
tf1.word_wrap = True
tf1.margin_left = tf1.margin_right = tf1.margin_top = tf1.margin_bottom = 0

p = tf1.paragraphs[0]
p.text = "Wearable Hardware & CAD"
p.font.size = Pt(11)
p.font.bold = True
p.font.color.rgb = COLOR_PRIMARY_NAVY

bullets1 = [
    "Custom ergonomic 3D enclosure",
    "Optical skin aperture (MAX30102)",
    "Airflow vents for BME280/PM2.5",
    "Dual-target ShrikeFi Zero-PCB"
]
for b in bullets1:
    bp = tf1.add_paragraph()
    bp.text = "• " + b
    bp.font.size = Pt(9)
    bp.font.color.rgb = COLOR_DARK_TEXT

# Prototype Box 2: Live 60 FPS Emergency Dashboard
card2 = slide6.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left_col_x, Inches(4.05), left_col_w, Inches(2.20))
card2.fill.solid()
card2.fill.fore_color.rgb = COLOR_CARD_BG
card2.line.color.rgb = COLOR_CARD_BORDER

# Image 2: valor_dashboard_heatwave.png
dash_img_path = 'docs/images/valor_dashboard_heatwave.png'
if os.path.exists(dash_img_path):
    slide6.shapes.add_picture(dash_img_path, left_col_x + Inches(0.12), Inches(4.15), width=Inches(2.70), height=Inches(2.00))

# Caption / Bullet points for Card 2
tb2 = slide6.shapes.add_textbox(left_col_x + Inches(2.90), Inches(4.10), Inches(2.60), Inches(2.05))
tf2 = tb2.text_frame
tf2.word_wrap = True
tf2.margin_left = tf2.margin_right = tf2.margin_top = tf2.margin_bottom = 0

p = tf2.paragraphs[0]
p.text = "Live 60 FPS Disaster Dashboard"
p.font.size = Pt(11)
p.font.bold = True
p.font.color.rgb = COLOR_PRIMARY_NAVY

bullets2 = [
    "100% Offline SPIFFS Web Server",
    "Real-time optical systolic trace",
    "RSA breathing wave extraction",
    "Live mNEWS2 & multi-hazard radar"
]
for b in bullets2:
    bp = tf2.add_paragraph()
    bp.text = "• " + b
    bp.font.size = Pt(9)
    bp.font.color.rgb = COLOR_DARK_TEXT

# Bottom badge on Left Column
b1 = slide6.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left_col_x, Inches(6.35), left_col_w, Inches(0.48))
b1.fill.solid()
b1.fill.fore_color.rgb = COLOR_LIGHT_GREEN
b1.line.color.rgb = COLOR_GREEN_BORDER
tf_b1 = b1.text_frame
p = tf_b1.paragraphs[0]
p.text = "⚡ Hardware Verified: Renesas ForgeFPGA (20 ns IBI) + ESP32-S3 SoftAP"
p.font.size = Pt(9.5)
p.font.bold = True
p.alignment = PP_ALIGN.CENTER
p.font.color.rgb = COLOR_SUCCESS_GREEN

# --- RIGHT COLUMN: Peer-Reviewed Research Foundations ---
right_col_x = Inches(6.65)
right_col_w = Inches(5.85)

# Section Header 2
h2 = slide6.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, right_col_x, Inches(1.30), right_col_w, Inches(0.40))
h2.fill.solid()
h2.fill.fore_color.rgb = COLOR_PRIMARY_NAVY
h2.line.color.rgb = COLOR_PRIMARY_NAVY
tf = h2.text_frame
tf.word_wrap = True
p = tf.paragraphs[0]
p.text = "📚 PEER-REVIEWED CLINICAL & ENGINEERING REFERENCES"
p.font.size = Pt(11)
p.font.bold = True
p.font.color.rgb = COLOR_WHITE
p.alignment = PP_ALIGN.CENTER

# References Table
rows = 10
cols = 2
table_shape = slide6.shapes.add_table(rows, cols, right_col_x, Inches(1.80), right_col_w, Inches(4.45))
table = table_shape.table
table.columns[0].width = Inches(3.90)
table.columns[1].width = Inches(1.95)

table_data = [
    ("CLINICAL / TECHNICAL STANDARD (PEER-REVIEWED)", "IMPLEMENTED IN"),
    ("Royal College of Physicians (2017) NEWS2 Acute Severity", "clinical_vitals_engine.c"),
    ("Charlton PH et al. (IEEE 2018) EDR Breathing Rate from PPG", "ppg_respiratory_rate.c"),
    ("Karlen W et al. (Physiol Meas 2012) Signal Quality Index (SQI)", "ppg_sqi.c"),
    ("Moran DS et al. (Am J Physiol 1998) Physiological Strain (PSI)", "disaster_risk_engine.c"),
    ("Brook RD et al. / AHA (Circulation 2010) PM2.5 Cardiac Stress", "disaster_risk_engine.c"),
    ("Johnson AEW et al. (MIMIC-III 2016) 16,387 ICU Records", "data/mimic/ (Host Evaluator)"),
    ("Si M et al. (Atmos Meas Tech 2019) ML Particulate Calibrator", "pm25_calibration_int8.c"),
    ("Steadman RG (J Appl Meteorol 1979) Sultriness & Heat Index", "disaster_risk_engine.c"),
    ("ARM AMBA AXI / SPI Mode 0 Protocol Specification", "forgefpga_ppg_top.v")
]

for r_idx, (col0, col1) in enumerate(table_data):
    cell0 = table.cell(r_idx, 0)
    cell1 = table.cell(r_idx, 1)
    
    cell0.text = col0
    cell1.text = col1
    
    # Header row formatting
    if r_idx == 0:
        cell0.fill.solid()
        cell0.fill.fore_color.rgb = COLOR_SECONDARY_BLUE
        cell1.fill.solid()
        cell1.fill.fore_color.rgb = COLOR_SECONDARY_BLUE
        
        for cell in (cell0, cell1):
            p = cell.text_frame.paragraphs[0]
            p.font.size = Pt(8.5)
            p.font.bold = True
            p.font.color.rgb = COLOR_WHITE
    else:
        # Alternating row fill
        row_bg = RGBColor(255, 255, 255) if r_idx % 2 == 1 else RGBColor(241, 245, 249)
        cell0.fill.solid()
        cell0.fill.fore_color.rgb = row_bg
        cell1.fill.solid()
        cell1.fill.fore_color.rgb = row_bg
        
        p0 = cell0.text_frame.paragraphs[0]
        p0.font.size = Pt(8)
        p0.font.bold = False
        p0.font.color.rgb = COLOR_DARK_TEXT
        
        p1 = cell1.text_frame.paragraphs[0]
        p1.font.size = Pt(8)
        p1.font.bold = True
        p1.font.color.rgb = COLOR_SECONDARY_BLUE

# Bottom callout box on Right Column
b2 = slide6.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, right_col_x, Inches(6.35), right_col_w, Inches(0.48))
b2.fill.solid()
b2.fill.fore_color.rgb = COLOR_LIGHT_GREEN
b2.line.color.rgb = COLOR_GREEN_BORDER
tf_b2 = b2.text_frame
p = tf_b2.paragraphs[0]
p.text = "✅ 100% Real Code: Every reference maps to verified C99/Verilog source in repo."
p.font.size = Pt(9.5)
p.font.bold = True
p.alignment = PP_ALIGN.CENTER
p.font.color.rgb = COLOR_SUCCESS_GREEN

# Save presentation
output_path = sys.argv[2] if len(sys.argv) > 2 else input_path
prs.save(output_path)
print(f"Successfully updated presentation at: {output_path}")
