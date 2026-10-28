// =============================================================================
// max30102_reflective_cradle_v2.scad
//
// Light-tight reflective finger cradle for the black MAX30102 breakout.
//
// CHANGES FROM v1 (all driven by your perfboard photos and a geometry check)
//   1. HEADERS: the photo shows pins on BOTH long edges (two rows of 4, 2.54 mm
//      pitch, about 2.5 mm in from each edge), not one 7-pin row. v1 had one
//      18.6 mm slot at the back only, so the front row would have landed on
//      solid floor. v2 cuts one slot per row.
//   2. FLOOR: v1 left only 0.7 mm of plastic under the board. v2 leaves 1.2 mm.
//   3. WINDOW HEIGHT: v1 left the sensor 0.3 mm BELOW the finger-pad surface.
//      v2 makes the sensor window flush (sensor_proud = 0.0). Tune with the
//      one parameter below after a test fit.
//   4. STRAP GROOVES: shallow grooves on the sides and underside so the velcro
//      strap does not slide off.
//   5. part = "coupon": a 10-minute print of just the pocket and slots, to
//      test-fit the real board BEFORE printing the full cradle.
//
// PRINT: matte black PLA/PETG, 0.2 mm layers, 4+ perimeters, 40%+ infill.
//   bottom: print as modelled (flat side down).
//   top:    rotate 180 deg about X in the slicer (roof on the bed).
// LIGHT:  black plastic can still pass 880 nm IR. Test a printed wall first
//   (phone camera + the sensor's IR LED). If light shows, add black paint or
//   foil tape on the inside, or raise side_wall and use 100% infill.
// SEAL:   closed-cell foam in the gasket grooves, foam rings at both finger
//   openings, thin foam under the roof so the finger is pressed gently
//   against the sensor, black foam tape along the seam if needed.
//
// Set `part` and export each STL:  "bottom" | "top" | "coupon" | "both"
// =============================================================================

$fn = 48;

/* ---- finger / shell ------------------------------------------------------ */
finger_w   = 22.0;   // channel width, mm
finger_h   = 18.0;   // channel height, mm
length     = 55.0;   // along the finger, mm
wall       = 3.0;    // lid roof thickness, mm
side_wall  = 5.0;    // side-wall thickness, mm

/* ---- MAX30102 breakout (measure with calipers and edit) ------------------ */
sensor_l   = 15.5;   // along finger, mm (board's short side)
sensor_w   = 20.6;   // across finger, mm (board's long side)
sensor_t   = 3.2;    // board + sensor IC thickness, mm - MEASURE
pocket_clr = 0.3;    // pocket clearance per axis, mm
sensor_proud = 0.0;  // sensor window vs finger-pad surface: 0 flush,
                     // +0.3 proud, -0.3 recessed

/* ---- headers: one row of pins along EACH long edge (from your photo) ----- */
row_pins   = 4;      // pins per row
hdr_pitch  = 2.54;
row_inset  = 2.5;    // pin centre to board edge, mm - MEASURE
hdr_body   = 2.54;   // header plastic width (along the finger), mm
slot_clr   = 1.0;    // extra slot length beyond the body, mm
slot_l     = hdr_body + slot_clr;                 // 3.54
slot_w     = row_pins * hdr_pitch + 0.8;          // 10.96
row_x      = sensor_l / 2.0 - row_inset;          // 5.25 from centre

/* ---- details ------------------------------------------------------------- */
pocket_z   = 1.2;                                   // plastic under the board
floor_t    = pocket_z + sensor_t + sensor_proud;    // finger-pad surface height
half_h     = finger_h / 2.0;
bottom_h   = half_h + floor_t;
top_h      = half_h + wall;
out_w      = finger_w + 2.0 * side_wall;

pocket_l   = sensor_l + pocket_clr;
pocket_w   = sensor_w + pocket_clr;
pocket_h   = sensor_t + 0.3;
gasket_w   = 2.5;
gasket_d   = 1.5;

pin_d      = 3.0;
pin_clear  = 0.3;
pin_h      = 4.0;
pin_x      = 20.0;
pin_y      = out_w / 2.0 - side_wall / 2.0;

min_skin   = 0.8;

strap_w    = 9.0;    // strap groove width, mm
strap_d    = 1.0;    // strap groove depth, mm
strap_x    = 13.5;   // groove centre from the middle, mm (clear of the pins)

part       = "both";

assert((side_wall - (pin_d + pin_clear)) / 2.0 >= min_skin,
       "side_wall too thin for the alignment holes");
assert(pin_x + (pin_d + pin_clear) / 2.0 + min_skin <= length / 2.0,
       "alignment pins too close to the ends");
assert(pocket_w + 0.4 <= finger_w, "board is wider than the channel");
assert(row_x + slot_l / 2.0 <= pocket_l / 2.0,
       "header slot runs past the pocket end");
assert(slot_w / 2.0 <= pocket_w / 2.0, "header slot wider than the pocket");
assert(pin_x - (pin_d + pin_clear) / 2.0 > pocket_l / 2.0 + gasket_w,
       "alignment pins collide with the gasket grooves");
assert(strap_x + strap_w / 2.0 < pin_x - (pin_d + pin_clear) / 2.0,
       "strap grooves collide with the alignment pins");
assert((floor_t - gasket_d) - strap_d >= 1.5,
       "underside strap groove leaves under 1.5 mm of plastic below the gasket groove");

/* ========================================================================== */
module strap_sides(h) {
    // shallow grooves on the two outside faces only (never the seam faces)
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * strap_x, sy * (out_w / 2.0 - strap_d / 2.0 + 0.5), h / 2.0])
            cube([strap_w, strap_d + 1, h + 2], center = true);
}

module bottom() {
    difference() {
        translate([-length / 2.0, -out_w / 2.0, 0])
            cube([length, out_w, bottom_h]);

        // finger channel (bottom half)
        translate([-length / 2.0 - 1, -finger_w / 2.0, floor_t])
            cube([length + 2, finger_w, half_h + 1]);

        // sensor pocket, open to the channel
        translate([-pocket_l / 2.0, -pocket_w / 2.0, pocket_z])
            cube([pocket_l, pocket_w, pocket_h + 0.1]);

        // foam-gasket grooves in front of and behind the pocket
        for (sx = [-1, 1])
            translate([sx * (pocket_l / 2.0 + gasket_w / 2.0 - 0.025),
                       0, floor_t - gasket_d / 2.0])
                cube([gasket_w + 0.05, finger_w - 0.1, gasket_d + 0.02],
                     center = true);

        // one header slot per pin row, through the pocket floor
        for (sx = [-1, 1])
            translate([sx * row_x - slot_l / 2.0, -slot_w / 2.0, -1])
                cube([slot_l, slot_w, pocket_z + 1.1]);

        // strap grooves: both sides and the underside
        strap_sides(bottom_h);
        for (sx = [-1, 1])
            translate([sx * strap_x - strap_w / 2.0, -out_w / 2.0 - 1, -1])
                cube([strap_w, out_w + 2, strap_d + 1]);
    }

    // alignment pins
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * pin_x, sy * pin_y, bottom_h - 1])
            cylinder(d = pin_d, h = pin_h + 1);
}

module top() {
    difference() {
        translate([-length / 2.0, -out_w / 2.0, 0])
            cube([length, out_w, top_h]);

        translate([-length / 2.0 - 1, -finger_w / 2.0, -1])
            cube([length + 2, finger_w, half_h + 1]);

        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * pin_x, sy * pin_y, -0.5])
                cylinder(d = pin_d + pin_clear, h = pin_h + 1);

        strap_sides(top_h);
    }
}

// test-fit coupon: just the pocket, slots and gasket grooves
module coupon() {
    intersection() {
        bottom();
        translate([-14, -out_w / 2.0 - 1, -1])
            cube([28, out_w + 2, floor_t + 3]);
    }
}

/* ========================================================================== */
if (part == "bottom") {
    bottom();
} else if (part == "top") {
    top();
} else if (part == "coupon") {
    coupon();
} else {
    bottom();
    translate([0, 0, bottom_h + 12])
        top();
}
