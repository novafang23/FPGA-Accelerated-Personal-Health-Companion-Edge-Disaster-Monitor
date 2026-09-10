## Bug 1 — `firmware/core/disaster_risk_engine.c`

The fix is a triage helper that must be called at the end of:

- `disaster_assess()`
- `disaster_assess_nn()`
- `disaster_assess_nn_int8()`

Replace the old linear `>` aggregation block with this helper, or paste the helper before the three functions and replace the old tail block with a call to it.

```c
/* Replace the old tail in each disaster_assess* function:

   result->overall_risk = result->heat_risk;
   result->overall_advisory = result->heat_advisory;
   ...
*/
static void finalize_overall_risk(disaster_risk_result_t *result)
{
#define DISASTER_RISK_MODALITIES 3U

    int levels[DISASTER_RISK_MODALITIES];
    const char *advisories[DISASTER_RISK_MODALITIES];

    int worst = RISK_NORMAL;
    const char *worst_advisory = NULL;

    int have_hazard = 0;
    int have_unknown = 0;
    int have_normal = 0;
    unsigned int i;

    levels[0]    = result->heat_risk;
    levels[1]    = result->pollution_risk;
    levels[2]    = result->flood_risk;

    advisories[0] = result->heat_advisory;
    advisories[1] = result->pollution_advisory;
    advisories[2] = result->flood_advisory;

    for (i = 0; i < DISASTER_RISK_MODALITIES; ++i) {
        if (levels[i] == RISK_UNKNOWN) {
            have_unknown = 1;
            continue;
        }

        if (levels[i] == RISK_NORMAL) {
            have_normal = 1;
            continue;
        }

        /*
         * Any real hazard overrides UNKNOWN/NORMAL.
         * Choose the most severe hazard.
         */
        have_hazard = 1;
        if (levels[i] > worst) {
            worst = levels[i];
            worst_advisory = advisories[i];
        }
    }

    if (have_hazard) {
        result->overall_risk = worst;
        result->overall_advisory = worst_advisory;
    } else if (have_unknown) {
        result->overall_risk = RISK_UNKNOWN;
        result->overall_advisory = have_normal
            ? "Active modalities are normal; other sensors are unmonitored/missing"
            : "Insufficient data; one or more modalities are unmonitored/missing";
    } else {
        result->overall_risk = RISK_NORMAL;
        result->overall_advisory =
            "All vitals and environmental conditions normal";
    }

#undef DISASTER_RISK_MODALITIES
}
```

Then in each of the three functions, after the per-modality risk fields are populated and before `return`:

```c
    finalize_overall_risk(result);
```

This fixes the glitch because `RISK_NORMAL == 0` is no longer allowed to dominate `RISK_UNKNOWN == -1` through a linear maximum-style comparison.

---

## Bug 2 — NN inference duplication

### `firmware/core/disaster_risk_engine.h`

Update the prototype to add the optional telemetry output pointer:

```c
void disaster_assess_nn_int8(const int8_t *input,
                             disaster_risk_result_t *result,
                             nn_output_t *raw_out); /* NULL = discard raw output */
```

### `firmware/core/disaster_risk_engine.c`

```c
void disaster_assess_nn_int8(const int8_t *input,
                             disaster_risk_result_t *result,
                             nn_output_t *raw_out)
{
    nn_output_t local_out;
    nn_output_t *out = raw_out ? raw_out : &local_out;

    /* Run inference exactly once.
     * If raw_out is provided, it also receives the raw output for telemetry.
     */
    nn_predict_int8(input, out);

    /* Existing classification code lives here.
     * It should read from `out` and set:
     *   result->heat_risk / heat_advisory
     *   result->pollution_risk / pollution_advisory
     *   result->flood_risk / flood_advisory
     */

    /* Bug 1 triage */
    finalize_overall_risk(result);
}
```

### `firmware/shrikefi/main_shrikefi.c`

Replace the old two-call sequence:

```c
    /* Before:
     * disaster_assess_nn_int8(&nn_input, &risk_result);
     * nn_predict_int8(&nn_input, &nn_out);      // duplicate inference
     */

    /* After: one inference call, raw telemetry output still populated */
    disaster_assess_nn_int8(&nn_input, &risk_result, &nn_out);
```

If any other caller needs the old two-argument behavior, pass `NULL` as `raw_out`.

Note: if the real codebase uses a different input type, keep that existing type; only the `nn_output_t *raw_out` parameter is added.

---

## Bug 3 — Hardware beat IRQ clear race

### `hardware/zynq/axi_ppg_accelerator.v`

Modify the same clocked block so the software W1C clear cannot overwrite a hardware beat on the same cycle. Put the software clear first, then let the hardware beat be the final writer:

```verilog
    if (write_execute) begin
        case (aw_addr_latched[4:2])
            3'b011: begin
                reg_threshold <= w_data_latched[15:8];

                /* W1C clear, but never on a hardware-beat cycle */
                if (w_data_latched[0] && !hw_beat_pulse)
                    beat_flag <= 1'b0;
            end
        endcase
    end

    /* Hardware beat has absolute priority: last writer wins */
    if (hw_beat_pulse)
        beat_flag <= 1'b1;
```

This prevents software from clearing a beat flag that was just set by a systolic-peak pulse.

### `hardware/shrikefi/forgefpga_ppg_top.v`

In the command/clear handling, make the clear conditional on no simultaneous peak. If the original code is a `case` item, replace:

```verilog
    CMD_CLEAR_IRQ: irq_beat <= 1'b0;
```

with:

```verilog
    CMD_CLEAR_IRQ: begin
        if (!peak_beat_detected)
            irq_beat <= 1'b0;
    end
```

Also keep the peak assignment in the clocked block:

```verilog
    if (peak_beat_detected) begin
        reg_ibi_latched <= peak_ibi_cycles;
        irq_beat <= 1'b1;
    end
```

Even when the clear command arrives on the same clock edge as a systolic peak, the clear is suppressed and the IRQ remains asserted. This prevents lost heartbeats and the resulting `2× IBI` HRV artifact.