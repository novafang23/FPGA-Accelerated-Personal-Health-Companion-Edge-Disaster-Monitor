// =============================================================================
// max30102_reflective_cradle.scad
//
// A light-tight, reflective finger cradle for the MAX30102 / GY-MAX30102.
//
// The MAX30102 is a REFLECTIVE sensor (LED + photodiode side by side), so the
// sensor sits against the finger PAD and the shell blocks ambient light from
// every other direction. This is the reflective analogue of the transmissive
// clip on a medical pulse oximeter.
//
// Sized for the measured 20.6 x 15.5 mm black MAX30102 breakout, mounted
// ACROSS the finger (20.6 mm side across the channel) with its 7-pin,
// 2.54 mm header edge toward the BACK of the cradle. The straight header is
// soldered pins-DOWN; the header spacer and the female Dupont plugs pass
// through a slot in the pocket floor and exit underneath the cradle.
//
// Two printed parts:
//   * bottom: the U-channel, sensor pocket, front/back gasket grooves, header
//     slot and alignment pins. The module drops into the pocket from the
//     CHANNEL side (top), so there is no large bridge over the cavity.
//   * top:    the matching lid. PRINT IT ROOF-DOWN (rotate 180 deg about X in
//     the slicer) so the 22 mm channel is not a bridge.
//
// Print in MATTE BLACK PLA/PETG (never clear/translucent - it pipes light),
// 0.2 mm layers, 3 perimeters, 20-30% infill. A velcro strap around the middle
// holds the halves; closed-cell foam strips seal the gasket grooves and foam
// rings seal the two finger openings.
//
// Set `part` below and export each STL:
//   part = "bottom";  -> bottom.stl
//   part = "top";     -> top.stl
//   part = "both";    -> preview only
// =============================================================================

$fn = 48;

/* ---- finger / shell ------------------------------------------------------ */
finger_w   = 22.0;   // channel width, mm (must clear the 20.6 mm board)
finger_h   = 18.0;   // channel height, mm
length     = 55.0;   // along the finger, mm
wall       = 3.0;    // lid roof thickness, mm
side_wall  = 5.0;    // side-wall thickness, mm (carries the alignment pins;
                     // must leave >= 2 perimeters either side of each hole)

/* ---- MAX30102 breakout (measured: 20.6 x 15.5 mm) ------------------------ */
sensor_l   = 15.5;   // along finger, mm  (board's short side)
sensor_w   = 20.6;   // across finger, mm (board's long side, header edge)
sensor_t   = 3.2;    // board + sensor IC, mm (~1.6 PCB + 1.55 IC) - MEASURE
pocket_clr = 0.3;    // pocket clearance per axis, mm

/* ---- header (7 pins, 2.54 mm pitch, straight, soldered pins-down) -------- */
hdr_pins   = 7;
hdr_pitch  = 2.54;
hdr_slot_l = 3.4;    // slot depth along the finger, from the pocket's back
                     // wall (2.54 mm spacer/Dupont body + clearance)
hdr_slot_w = hdr_pins * hdr_pitch + 0.8;   // 18.58 mm across the finger
hdr_off_y  = 0.0;    // shift if the header is not centred on the board edge

/* ---- details ------------------------------------------------------------- */
floor_t    = sensor_t + 1.0;         // floor thickness above the pocket (4.2)
half_h     = finger_h / 2.0;
bottom_h   = half_h + floor_t;
top_h      = half_h + wall;
out_w      = finger_w + 2.0 * side_wall;

pocket_l   = sensor_l + pocket_clr;
pocket_w   = sensor_w + pocket_clr;
pocket_h   = sensor_t + 0.3;         // pocket depth, module + clearance
pocket_z   = floor_t - pocket_h;     // pocket floor z (0.7)
gasket_w   = 2.5;                    // foam-strip width
gasket_d   = 1.5;                    // groove depth

pin_d      = 3.0;                    // 3 mm pins survive FDM layer lines
pin_clear  = 0.3;                    // diametral clearance in the lid holes
pin_h      = 4.0;
pin_x      = 20.0;
pin_y      = out_w / 2.0 - side_wall / 2.0;   // centred in the side wall

min_skin   = 0.8;                    // min plastic each side of a pin hole
                                     // (2 lines of a 0.4 mm nozzle)

part       = "both";   // "bottom" | "top" | "both"

assert((side_wall - (pin_d + pin_clear)) / 2.0 >= min_skin,
       "side_wall too thin for the alignment holes - slicer will leave a slit");
assert(pin_x + (pin_d + pin_clear) / 2.0 + min_skin <= length / 2.0,
       "alignment pins too close to the ends");
assert(pocket_w + 0.4 <= finger_w,
       "board is wider than the channel - it cannot drop into the pocket");
assert(abs(hdr_off_y) + hdr_slot_w / 2.0 <= pocket_w / 2.0,
       "header slot runs outside the pocket");
assert(pin_x - (pin_d + pin_clear) / 2.0 > pocket_l / 2.0 + gasket_w,
       "alignment pins collide with the gasket grooves");

/* ========================================================================== */
module bottom() {
    difference() {
        // shell
        translate([-length / 2.0, -out_w / 2.0, 0])
            cube([length, out_w, bottom_h]);

        // finger channel (the bottom half of the finger)
        translate([-length / 2.0 - 1, -finger_w / 2.0, floor_t])
            cube([length + 2, finger_w, half_h + 1]);

        // sensor pocket, open to the channel (module drops in from the top)
        translate([-pocket_l / 2.0, -pocket_w / 2.0, pocket_z])
            cube([pocket_l, pocket_w, pocket_h + 0.1]);

        // foam-gasket grooves in front of and behind the pocket. The board
        // nearly fills the channel width, so the channel side walls seal the
        // sides; the grooves stop 0.05 mm short of those walls and overlap
        // the pocket by 0.05 mm, so no faces coincide (non-manifold).
        for (sx = [-1, 1])
            translate([sx * (pocket_l / 2.0 + gasket_w / 2.0 - 0.025),
                       0, floor_t - gasket_d / 2.0])
                cube([gasket_w + 0.05, finger_w - 0.1, gasket_d + 0.02],
                     center = true);

        // header slot through the pocket floor at the back edge: the straight
        // header spacer and the Dupont plugs exit underneath the cradle
        translate([pocket_l / 2.0 - hdr_slot_l,
                   hdr_off_y - hdr_slot_w / 2.0, -1])
            cube([hdr_slot_l, hdr_slot_w, pocket_z + 1.1]);
    }

    // alignment pins (embedded 1 mm into the shell so the union is one solid)
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * pin_x, sy * pin_y, bottom_h - 1])
            cylinder(d = pin_d, h = pin_h + 1);
}

module top() {
    difference() {
        // lid shell
        translate([-length / 2.0, -out_w / 2.0, 0])
            cube([length, out_w, top_h]);

        // finger channel (the top half of the finger)
        translate([-length / 2.0 - 1, -finger_w / 2.0, -1])
            cube([length + 2, finger_w, half_h + 1]);

        // alignment holes
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * pin_x, sy * pin_y, -0.5])
                cylinder(d = pin_d + pin_clear, h = pin_h + 1);
    }
}

/* ========================================================================== */
if (part == "bottom") {
    bottom();
} else if (part == "top") {
    top();
} else {
    bottom();
    translate([0, 0, bottom_h + 12])
        top();
}
