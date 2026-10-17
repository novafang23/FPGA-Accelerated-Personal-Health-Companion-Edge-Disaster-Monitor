# MAX30102 reflective finger cradle

A two-part, light-tight 3D-printed cradle for the MAX30102 / GY-MAX30102.
It is the **reflective** analogue of a medical pulse-oximeter clip: the sensor
sits against the finger **pad**, and the black shell blocks ambient light from
the sides, top and ends.

## Files

| File | Purpose |
|---|---|
| `max30102_reflective_cradle.scad` | Parameterised OpenSCAD model (bottom + top) |

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
| Closed-cell foam, 2 mm | gasket ring + end seals (mouse pad / EVA craft foam) |
| Velcro strap, ~200 mm | holds the two halves together |
| MAX30102 / GY-MAX30102 breakout | sensor window on the **top** of the module |
| Black tape | optional, for the seam |

## Print

1. Open `max30102_reflective_cradle.scad` in OpenSCAD (free).
2. Set `part = "bottom";` -> render (F6) -> export STL -> print **as modelled**.
3. Set `part = "top";` -> render -> export STL -> print **roof-down**
   (rotate 180 deg about X in the slicer) so the 20 mm channel is not a bridge.
4. Settings: 0.2 mm layers, **3 perimeters**, 20-30% infill, black matte.

## Assembly

1. Drop the MAX30102 into the **pocket from the top (channel side)**, sensor
   window facing **up**. The pocket is deliberately a little deeper than the
   module: a 0.5 mm foam pad or a drop of silicone under it brings the sensor
   flush with the channel floor. Route the cable through the **8 x 3 mm
   channel** to the back.
2. Cut a closed-cell foam ring (outer ~20.5 x 20.5 mm, inner ~15.5 x 15.5 mm,
   2 mm thick) and press it into the **gasket groove** around the sensor.
3. Fit the **top** onto the alignment pins, then wrap the **velcro strap**
   around the middle.
4. Cut two foam rings for the finger openings at the ends, or let the hand and
   a foam cap on the fingertip close the ends.
5. Optional: a strip of black tape along the seam if the SQI still shows a
   leak.

## Fit check

- Measure your finger and the breakout before printing; the defaults are
  `finger_w = 20`, `finger_h = 18`, `sensor_l/w = 15.5`, `sensor_t = 3.2`.
- The pocket is a **nominal** fit. If your printer runs tight, increase
  `sensor_l` and `sensor_w` by 0.3 mm; if it runs loose, add a foam shim.
- The sensor window should sit flush against the skin after the foam
  compresses; if it feels too deep, reduce `floor_t` or `pocket_h` and reprint.
- Light pressure only - a hard clamp blanches the finger and kills the AC.

## Validate

Measure the same finger for 60 s **without** and **with** the cradle:

- SQI - target **>= 0.85** (was 0.71-0.73 on the bench)
- SpO2 stability - should tighten toward 98-99
- RR confidence - the baseline path should improve

Do not lower the firmware SQI threshold; the cradle is the fix.
