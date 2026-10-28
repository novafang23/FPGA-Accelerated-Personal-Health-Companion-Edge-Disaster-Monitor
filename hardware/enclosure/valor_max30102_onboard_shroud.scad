// =============================================================================
// valor_max30102_onboard_shroud_v2.scad
// Fixed Manifold Geometry - Solid Internal Mask & Sealed Back Light-Trap
// =============================================================================

$fn = 64;

// 1. Hardware Dimensions
pcb_w          = 14.4;  // MAX30102 PCB width (X) + clearance
pcb_l          = 18.8;  // MAX30102 PCB length (Y) + clearance
pcb_t          = 1.6;   // PCB thickness
hdr_h          = 8.5;   // Black female header height
stack_h        = hdr_h + pcb_t; // 10.1 mm from perfboard to top of PCB

// 2. Optical Window & Internal Component Reliefs (FIXED HEIGHTS)
ic_offset_y    = 2.2;   // Optical IC shift toward back of PCB
win_w          = 7.0;   // Optical window width over MAX30102 glass
win_l          = 5.0;   // Optical window length over MAX30102 glass

mask_t         = 2.8;   // Total thickness of protective ceiling above PCB
relief_h       = 1.6;   // Underside pocket depth for tall solder cones & SMDs
                        // Leaves (2.8 - 1.6) = 1.2 mm of SOLID plastic above pins!

pin_trench_w   = 3.4;   // Width of left/right underside slots for the 8 solder cones
smd_trench_l   = 7.0;   // Front underside relief length for LDO & capacitors

// 3. Exterior & Finger Tunnel Dimensions
wall_left      = 1.6;   // Slim left wall to clear blue TMP117 board
wall_right     = 3.2;   // Sturdy right wall
back_ext       = 10.0;  // Extension behind PCB (fits in 27 mm GPS space)
front_ext      = 7.0;   // Front finger entry extension

finger_w       = 16.5;  // Finger tunnel width
tunnel_h       = 14.5;  // Finger tunnel height
roof_t         = 2.2;   // Top roof thickness

// Calculated X boundaries
left_x         = -pcb_w / 2.0 - wall_left;
right_x        =  pcb_w / 2.0 + wall_right + 2.5;
total_w        = right_x - left_x;
tunnel_cx      = 1.0;

// Calculated Y boundaries
front_y        = -pcb_l / 2.0 - front_ext;
back_y         =  pcb_l / 2.0 + back_ext;
total_l        = back_y - front_y;

floor_z        = stack_h + mask_t; // Z height where the finger rests (12.9 mm)
total_h        = floor_z + tunnel_h + roof_t;

module valor_max30102_shroud() {
    difference() {
        // 1. Outer Light-Tight Monolithic Shell
        hull() {
            translate([left_x, front_y, 0])
                cube([total_w, total_l, total_h - 4.0]);
            translate([left_x + 2.0, front_y, 0])
                cube([total_w - 4.0, total_l, total_h]);
        }

        // 2. Lower Pocket for Dual Female Headers + MAX30102 PCB
        translate([-pcb_w / 2.0, -pcb_l / 2.0, -0.1])
            cube([pcb_w, pcb_l, stack_h + 0.1]);

        // Center channel between the two 4-pin female headers
        translate([-pcb_w / 2.0 + 3.0, -pcb_l / 2.0 - 0.5, -0.1])
            cube([pcb_w - 6.0, pcb_l + 1.0, hdr_h - 1.0]);

        // 3. Underside Relief Pockets for the 8 Solder Peaks (Blind pockets - do NOT break through floor!)
        translate([-pcb_w / 2.0, -pcb_l / 2.0, stack_h - 0.1])
            cube([pin_trench_w, pcb_l, relief_h + 0.1]);
        translate([pcb_w / 2.0 - pin_trench_w, -pcb_l / 2.0, stack_h - 0.1])
            cube([pin_trench_w, pcb_l, relief_h + 0.1]);

        // 4. Underside Front Relief Cavity for SMD LDO & Capacitors
        translate([-pcb_w / 2.0, -pcb_l / 2.0, stack_h - 0.1])
            cube([pcb_w, smd_trench_l, relief_h + 0.1]);

        // 5. Precision Optical Aperture Over MAX30102 Glass Window
        // Connects the finger tunnel floor down to the optical sensor IC
        hull() {
            translate([-win_w / 2.0, ic_offset_y - win_l / 2.0, stack_h - 0.2])
                cube([win_w, win_l, 0.2]);
            translate([-win_w / 2.0 - 1.5, ic_offset_y - win_l / 2.0 - 1.5, floor_z + 0.1])
                cube([win_w + 3.0, win_l + 3.0, 0.2]);
        }

        // 6. Contoured Finger Tunnel (Open at Front, SOLID Light-Tight Wall at Back)
        hull() {
            translate([tunnel_cx - 5.5, front_y - 1.0, floor_z])
                cube([11.0, total_l - 3.0, 0.5]);
            translate([tunnel_cx - finger_w / 2.0, front_y - 1.0, floor_z + 3.5])
                cube([finger_w, total_l - 3.0, tunnel_h - 6.5]);
            translate([tunnel_cx - 5.0, front_y - 1.0, floor_z + tunnel_h - 1.5])
                cube([10.0, total_l - 3.0, 1.5]);
        }

        // 7. Front & Back Lower Skirt Cutouts (Stops 2 mm below PCB so back stays light-tight)
        translate([left_x - 1, front_y - 1, -0.1])
            cube([total_w + 2, front_ext + 0.1, hdr_h]);
        translate([left_x - 1, pcb_l / 2.0 - 0.1, -0.1])
            cube([total_w + 2, back_ext + 1.5, hdr_h]);
    }
}

valor_max30102_shroud();
