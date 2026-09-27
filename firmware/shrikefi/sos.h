/*
 * sos.h — Emergency assistance state machine (requirement 6)
 * SIH26181 / VALOR: Personal Health Companion & Edge Disaster Monitor
 *
 * The device already decides when a subject is in trouble: clinical_fuse_triage()
 * produces a CRITICAL level. Until now that verdict changed a number on a screen
 * and nothing else. This module is what turns the verdict into an action - it
 * latches an emergency, and the caller renders a full-screen card on the OLED so
 * that whoever finds the patient is told what is happening and what to do.
 *
 * WHY THIS FILE HAS NO HARDWARE IN IT
 * The state machine is the part worth testing: confirm timers, the cancel hold,
 * the recovery stand-down and the re-arm rule. Keeping it free of I2C, FreeRTOS
 * and the SSD1306 means all of that runs in the host unit-test harness, and the
 * device code is left holding only the trigger plumbing and the rendering.
 * sos.c therefore includes nothing but <stdint.h> and <stdbool.h>.
 *
 * TIMEBASE
 * All times are a monotonic millisecond counter supplied by the caller
 * (esp_timer_get_time()/1000 on the device, a plain counter in tests). There is
 * no wall clock: the board has no battery-backed RTC, so offline there is no
 * calendar time to stamp a card with. The caller reports elapsed-since-boot and
 * the card says so rather than inventing a time of day.
 */

#ifndef SHRIKEFI_SOS_H
#define SHRIKEFI_SOS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Four states, as specified. ARMED is the "a trigger condition is being timed"
 * state: it exists so that a single artefact-corrupted second cannot latch a
 * full emergency, and so that a condition which clears before its confirm window
 * expires leaves no trace. */
typedef enum {
    SOS_IDLE = 0,   /* monitoring normally */
    SOS_ARMED,      /* a trigger condition is present but not yet sustained */
    SOS_ACTIVE,     /* emergency latched; the caller shows the card */
    SOS_CANCELLED   /* stood down deliberately; latched until the condition clears */
} sos_state_t;

typedef enum {
    SOS_TRIGGER_NONE = 0,
    SOS_TRIGGER_CRITICAL_TRIAGE,
    SOS_TRIGGER_CONTACT_LOST,
    SOS_TRIGGER_MANUAL
} sos_trigger_t;

typedef struct {
    /* CRITICAL must hold this long before it latches. NEWS2 is computed at 1 Hz
     * and a single corrupt second can reach CLINICAL_CRITICAL, so latching on
     * the first one would cry wolf. 3 s costs nothing in a real collapse. */
    uint32_t critical_confirm_ms;

    /* Contact must be absent this long before it latches. DISABLED BY DEFAULT,
     * and that is deliberate. This is a fingertip device: the finger comes off
     * between every measurement, every hand-off and every time the operator
     * answers a question, so "prolonged loss of contact" as an emergency trigger
     * would fire continuously during normal use. It is the right trigger for a
     * worn device and the wrong one for this one. Left implemented and off, so
     * that T2.3 (IMU, worn) can enable it with evidence rather than optimism. */
    uint32_t contact_lost_confirm_ms;
    bool     contact_lost_enabled;

    /* Cancel requires a sustained press, so a knock or a brush against the
     * device cannot dismiss a real emergency. The button itself arrives with
     * T2.2; the console path is explicit-operator and bypasses the hold. */
    uint32_t cancel_hold_ms;

    /* Automatic stand-down. A CRITICAL that has been continuously non-critical
     * for this long clears itself.
     *
     * This exists because without it a false trigger is unrecoverable: there is
     * no button yet and no network, so a latched SOS would hold the screen until
     * the battery died. It cannot be removed when the button arrives either -
     * the same stuck-screen problem would simply need a human instead. It only
     * ever applies to the CRITICAL trigger; a manually triggered or
     * contact-lost emergency stays until it is cancelled. */
    uint32_t recovery_clear_ms;
} sos_config_t;

/* Sensible device defaults. Overridable before sos_init(). */
void sos_default_config(sos_config_t *cfg);

void sos_init(const sos_config_t *cfg);

/* Call at 1 Hz.
 *   now_ms          monotonic milliseconds
 *   critical        true when the fused triage verdict is critical
 *   contact_present true while the optical front-end sees a finger
 *
 * A critical verdict is only acted on while contact is present: the triage
 * engines do not run without a finger, so a critical flag with no contact is
 * stale rather than a finding. */
void sos_update(uint32_t now_ms, bool critical, bool contact_present);

/* Operator-initiated emergency (dashboard button in T1.3, console key now). */
void sos_manual_trigger(uint32_t now_ms);

/* Sustained-press cancel. press() records the start of a press, release() ends
 * it early; the hold only completes inside sos_update(). */
void sos_cancel_press(uint32_t now_ms);
void sos_cancel_release(void);

/* Immediate, deliberate stand-down. Used by the console path and by the
 * dashboard, where the operator's intent is unambiguous. */
void sos_cancel_now(uint32_t now_ms);

sos_state_t   sos_get_state(void);
sos_trigger_t sos_get_trigger(void);

/* Milliseconds the emergency has been active, or 0 when it is not. */
uint32_t sos_active_ms(uint32_t now_ms);

/* 0-100 progress of a cancel hold in progress, for a progress bar. */
uint32_t sos_cancel_progress_pct(uint32_t now_ms);

/* Human-readable names, for logs and the card. */
const char *sos_state_name(sos_state_t s);
const char *sos_trigger_name(sos_trigger_t t);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_SOS_H */
