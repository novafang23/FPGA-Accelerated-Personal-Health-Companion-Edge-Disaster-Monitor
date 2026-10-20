# MAX30102 reflective finger cradle

A two-part, light-tight 3D-printed cradle for the MAX30102 / GY-MAX30102.
It is the **reflective** analogue of a medical pulse-oximeter clip: the sensor
sits against the finger **pad**, and the black shell blocks ambient light from
the sides, top and ends.

## Files

| File | Purpose |
|---|---|
| `max30102_reflective_cradle.scad` | Parameterised OpenSCAD model (bottom + top) |
| `stl/cradle_bottom.stl` | Bottom half, exported with `part = "bottom"` (print as modelled) |
| `stl/cradle_top.stl` | Lid, exported with `part = "top"` (print roof-down) |

## Why this is needed

The MAX30102 is **reflective** (LED and photodiode side by side). Room light
leaks into the photodiode and raises the DC without raising the AC, so the
**perfusion index (AC/DC)** and the **SQI** fall. Covering the finger with a
hand raised the SQI on the bench; this cradle makes that repeatable.

Do **not** use a transmissive pulse-ox clip: it puts the LED and detector on
opposite sides, which is the wrong geometry for this sensor.

## Bill of materials

| Item | Notes |
|---|---|
| Matte black PLA or PETG | ~20 g; never clear/translucent |
| Closed-cell foam, 2 mm | 2 gasket strips + end seals (mouse pad / EVA craft foam) |
| Velcro strap, ~200 mm | holds the two halves together |
| MAX30102 breakout, 20.6 x 15.5 mm | sensor window on the **top**; 7-pin 2.54 mm straight header soldered **pins-down** |
| 4 female-female Dupont jumpers | VIN (3.3 V only), GND, SDA, SCL - INT/IRD/RD are not used by the firmware |
| Black tape | optional, for the seam and the header slot |

## Print

1. Open `max30102_reflective_cradle.scad` in OpenSCAD (free).
2. Set `part = "bottom";` -> render (F6) -> export STL -> print **as modelled**.
3. Set `part = "top";` -> render -> export STL -> print **roof-down**
   (rotate 180 deg about X in the slicer) so the 22 mm channel is not a bridge.
4. Settings: 0.2 mm layers, **3 perimeters**, 20-30% infill, black matte.

## Assembly

1. Solder the straight header to the board **pins pointing down** (away from
   the sensor window).
2. Drop the MAX30102 into the **pocket from the top (channel side)**, sensor
   window facing **up**, the **20.6 mm side across the finger** and the
   **header edge toward the back** of the cradle. The header spacer and pins
   pass through the **18.6 x 3.4 mm slot** in the pocket floor. The pocket is
   deliberately a little deeper than the module: a 0.5 mm foam pad or a drop
   of silicone under the front of the board brings the sensor flush with the
   channel floor.
3. From underneath, push the 4 Dupont jumpers onto VIN, GND, SDA, SCL. The
   plugs hang below the cradle, so rest it on the edge of the table or on
   your palm, not flat. Press black foam or tape around the plugs to close
   the rest of the slot.
4. Cut two closed-cell foam strips (~2.5 x 22 mm, 2 mm thick) and press them
   into the **gasket grooves** in front of and behind the pocket. The board
   fills the channel width, so the side walls seal the sides.
5. Fit the **top** onto the alignment pins, then wrap the **velcro strap**
   around the middle.
6. Cut two foam rings for the finger openings at the ends, or let the hand and
   a foam cap on the fingertip close the ends.
7. Optional: a strip of black tape along the seam if the SQI still shows a
   leak.

## Fit check

- Measure your finger and the breakout before printing; the defaults are
  `finger_w = 22`, `finger_h = 18`, `sensor_l = 15.5` (along the finger),
  `sensor_w = 20.6` (across), `sensor_t = 3.2`, `pocket_clr = 0.3`.
- `sensor_t = 3.2` is an estimate (1.6 mm PCB + 1.55 mm IC). Measure from the
  back of the board to the top of the sensor window and update it.
- The header slot assumes the 7 pins are **centred** on the 20.6 mm edge and
  the pin row is within ~1.5 mm of that edge. If yours is off-centre, set
  `hdr_off_y`; if the row sits further in, increase `hdr_slot_l`.
- The side walls are 5 mm (`side_wall`) so the 3.3 mm alignment holes keep
  ~0.85 mm of plastic on each side (2 perimeters at 0.4 mm). The model
  `assert`s this; if you thin `side_wall` or grow `pin_d`, OpenSCAD will
  refuse to render rather than export a lid with a slit at every pin.
- The pins are 3 mm with 0.3 mm diametral clearance (`pin_clear`). If the lid
  is tight on your printer, raise `pin_clear` to 0.4-0.5.
- The pocket already has 0.3 mm clearance (`pocket_clr`). If your printer
  runs tight, raise it to 0.5; if it runs loose, add a foam shim.
- The sensor window should sit flush against the skin after the foam
  compresses; if it feels too deep, reduce `floor_t` or `pocket_h` and reprint.
- Light pressure only - a hard clamp blanches the finger and kills the AC.

## Validate

Measure the same finger for 60 s **without** and **with** the cradle:

- SQI - target **>= 0.85** (was 0.71-0.73 on the bench)
- SpO2 stability - should tighten toward 98-99
- RR confidence - the baseline path should improve

Do not lower the firmware SQI threshold; the cradle is the fix.
