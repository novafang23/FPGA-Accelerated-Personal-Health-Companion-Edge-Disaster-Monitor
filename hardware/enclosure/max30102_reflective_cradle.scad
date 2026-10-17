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
// Two printed parts:
//   * bottom: the U-channel, sensor pocket, gasket groove, cable channel and
//     alignment pins. The module drops into the pocket from the CHANNEL side
//     (top), so there is no large bridge over the cavity.
//   * top:    the matching lid. PRINT IT ROOF-DOWN (rotate 180 deg about X in
//     the slicer) so the 20 mm channel is not a bridge.
//
// Print in MATTE BLACK PLA/PETG (never clear/translucent - it pipes light),
// 0.2 mm layers, 3 perimeters, 20-30% infill. A velcro strap around the middle
// holds the halves; closed-cell foam rings seal the gasket groove and the two
// finger openings.
//
// Set `part` below and export each STL:
//   part = "bottom";  -> bottom.stl
//   part = "top";     -> top.stl
//   part = "both";    -> preview only
// =============================================================================

$fn = 48;

/* ---- finger / shell ------------------------------------------------------ */
finger_w   = 20.0;   // channel width, mm   (measure your finger)
finger_h   = 18.0;   // channel height, mm
length     = 55.0;   // along the finger, mm
wall       = 3.0;    // shell thickness, mm

/* ---- MAX30102 breakout (GY-MAX30102 is ~15.5 x 15.5) --------------------- */
sensor_l   = 15.5;   // along finger, mm
sensor_w   = 15.5;   // across finger, mm
sensor_t   = 3.2;    // module thickness, mm

/* ---- details ------------------------------------------------------------- */
floor_t    = sensor_t + 1.0;         // floor thickness above the pocket (4.2)
half_h     = finger_h / 2.0;
bottom_h   = half_h + floor_t;
top_h      = half_h + wall;
out_w      = finger_w + 2.0 * wall;

pocket_h   = sensor_t + 0.3;         // pocket depth, module + clearance
pocket_z   = floor_t - pocket_h;     // pocket floor z (0.7)
gasket_w   = 2.5;                    // foam-ring width
gasket_d   = 1.5;                    // groove depth

pin_d      = 2.5;
pin_h      = 4.0;
pin_x      = 20.0;
pin_y      = out_w / 2.0 - wall / 2.0;

cable_w    = 8.0;
cable_h    = 3.0;

part       = "both";   // "bottom" | "top" | "both"

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
        translate([-sensor_l / 2.0, -sensor_w / 2.0, pocket_z])
            cube([sensor_l, sensor_w, pocket_h + 0.1]);

        // foam-gasket groove around the pocket (inner face offset 0.1 mm so it
        // does not coincide with the pocket wall - coincident faces are what
        // makes OpenSCAD report a non-manifold)
        translate([0, 0, floor_t - gasket_d / 2.0])
            difference() {
                cube([sensor_l + 2 * gasket_w,
                      sensor_w + 2 * gasket_w,
                      gasket_d + 0.02], center = true);
                cube([sensor_l - 0.1, sensor_w - 0.1, gasket_d + 0.04], center = true);
            }

        // cable slot from the pocket to the back
        translate([sensor_l / 2.0, -cable_w / 2.0, pocket_z])
            cube([length / 2.0 - sensor_l / 2.0 + 1, cable_w, cable_h]);

        // cable exit through the back wall
        translate([length / 2.0 - 2.0, -cable_w / 2.0, 0])
            cube([3, cable_w, cable_h]);
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
                cylinder(d = pin_d + 0.3, h = pin_h + 1);
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
