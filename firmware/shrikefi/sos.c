/*
 * sos.c — Emergency assistance state machine (requirement 6)
 * SIH26181 / VALOR
 *
 * See sos.h for the state and trigger definitions and for why this file
 * deliberately depends on nothing but <stdint.h> and <stdbool.h>.
 */

#include "sos.h"

/* ------------------------------------------------------------------------- */
/* State                                                                      */
/* ------------------------------------------------------------------------- */

static sos_config_t  s_cfg;

static sos_state_t   s_state        = SOS_IDLE;
static sos_trigger_t s_trigger      = SOS_TRIGGER_NONE;
static sos_trigger_t s_pending      = SOS_TRIGGER_NONE;

static uint32_t      s_active_since = 0;  /* when ACTIVE began                */
static uint32_t      s_cond_since   = 0;  /* when the pending condition began */
static uint32_t      s_ok_since     = 0;  /* since the level was last critical */

static uint32_t      s_press_ms     = 0;  /* cancel press start, 0 = released */
static uint32_t      s_cleared_ms   = 0;  /* when a stand-down was recorded   */

void sos_default_config(sos_config_t *cfg) {
    if (!cfg) return;
    cfg->critical_confirm_ms     = 15000u; /* Increased from 3s to 15s to avoid transient false alarms */
    cfg->contact_lost_confirm_ms = 120000u;
    cfg->contact_lost_enabled    = false;  /* see sos.h - fingertip device */
    cfg->cancel_hold_ms          = 5000u;
    cfg->recovery_clear_ms       = 30000u;
}

void sos_init(const sos_config_t *cfg) {
    if (cfg) {
        s_cfg = *cfg;
    } else {
        sos_default_config(&s_cfg);
    }
    s_state        = SOS_IDLE;
    s_trigger      = SOS_TRIGGER_NONE;
    s_pending      = SOS_TRIGGER_NONE;
    s_active_since = 0;
    s_cond_since   = 0;
    s_ok_since     = 0;
    s_press_ms     = 0;
    s_cleared_ms   = 0;
}

/* ------------------------------------------------------------------------- */
/* Internal transitions                                                       */
/* ------------------------------------------------------------------------- */

static uint32_t confirm_ms_for(sos_trigger_t t) {
    switch (t) {
        case SOS_TRIGGER_CRITICAL_TRIAGE: return s_cfg.critical_confirm_ms;
        case SOS_TRIGGER_CONTACT_LOST:    return s_cfg.contact_lost_confirm_ms;
        default:                          return 0u;
    }
}

static void latch(sos_trigger_t trigger, uint32_t now_ms) {
    s_state        = SOS_ACTIVE;
    s_trigger      = trigger;
    s_active_since = now_ms;
    s_ok_since     = 0;
    s_press_ms     = 0;
    s_cond_since   = 0;
}

static void stand_down(uint32_t now_ms) {
    /* CANCELLED is a latch, not an idle: while the triggering condition is still
     * present the device must not immediately re-latch, or a cancel during an
     * ongoing emergency would be undone by the next 1 Hz update. It returns to
     * IDLE once the condition clears, so a *new* episode arms normally. */
    s_state      = SOS_CANCELLED;
    s_ok_since   = 0;
    s_press_ms   = 0;
    s_cond_since = 0;
    s_cleared_ms = now_ms;
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */

void sos_update(uint32_t now_ms, bool critical, bool contact_present) {
    /* A critical verdict with no finger on the sensor is not evidence of a
     * collapse - it is stale data from the last time there was one, because the
     * triage engines do not run without contact. */
    bool is_critical = critical && contact_present;

    /* 1. Complete a cancel hold in progress. */
    if (s_state == SOS_ACTIVE && s_press_ms != 0) {
        if ((now_ms - s_press_ms) >= s_cfg.cancel_hold_ms) {
            stand_down(now_ms);
            return;
        }
    }

    /* 2. What condition, if any, is pending right now? */
    sos_trigger_t pending = SOS_TRIGGER_NONE;
    if (is_critical) {
        pending = SOS_TRIGGER_CRITICAL_TRIAGE;
    } else if (s_cfg.contact_lost_enabled && !contact_present) {
        pending = SOS_TRIGGER_CONTACT_LOST;
    }

    switch (s_state) {
        case SOS_IDLE:
            if (pending != SOS_TRIGGER_NONE) {
                s_state      = SOS_ARMED;
                s_pending    = pending;
                s_cond_since = now_ms;
            }
            break;

        case SOS_ARMED:
            if (pending == SOS_TRIGGER_NONE) {
                /* Cleared inside its confirm window - an artefact, not an event.
                 * Leave no trace. */
                s_state      = SOS_IDLE;
                s_pending    = SOS_TRIGGER_NONE;
                s_cond_since = 0;
            } else if (pending != s_pending) {
                /* A different condition took over; restart its window. */
                s_pending    = pending;
                s_cond_since = now_ms;
            } else if ((now_ms - s_cond_since) >= confirm_ms_for(pending)) {
                latch(pending, now_ms);
            }
            break;

        case SOS_ACTIVE:
            /* Automatic stand-down, CRITICAL trigger only. A manual or
             * contact-lost emergency waits for a human. */
            if (s_trigger == SOS_TRIGGER_CRITICAL_TRIAGE) {
                if (!is_critical) {
                    if (s_ok_since == 0) s_ok_since = now_ms;
                    if ((now_ms - s_ok_since) >= s_cfg.recovery_clear_ms) {
                        stand_down(now_ms);
                    }
                } else {
                    s_ok_since = 0;
                }
            }
            break;

        case SOS_CANCELLED:
            if (pending == SOS_TRIGGER_NONE) {
                s_state   = SOS_IDLE;
                s_trigger = SOS_TRIGGER_NONE;
            }
            break;

        default:
            s_state = SOS_IDLE;
            break;
    }
}

void sos_manual_trigger(uint32_t now_ms) {
    latch(SOS_TRIGGER_MANUAL, now_ms);
}

void sos_cancel_press(uint32_t now_ms) {
    if (s_state == SOS_ACTIVE && s_press_ms == 0) {
        s_press_ms = now_ms;
    }
}

void sos_cancel_release(void) {
    s_press_ms = 0;
}

void sos_cancel_now(uint32_t now_ms) {
    if (s_state == SOS_ACTIVE || s_state == SOS_ARMED) {
        stand_down(now_ms);
    }
}

sos_state_t sos_get_state(void) { return s_state; }

sos_trigger_t sos_get_trigger(void) { return s_trigger; }

uint32_t sos_active_ms(uint32_t now_ms) {
    if (s_state != SOS_ACTIVE) return 0u;
    return now_ms - s_active_since;
}

uint32_t sos_cancel_progress_pct(uint32_t now_ms) {
    if (s_state != SOS_ACTIVE || s_press_ms == 0 || s_cfg.cancel_hold_ms == 0) {
        return 0u;
    }
    uint32_t held = now_ms - s_press_ms;
    if (held >= s_cfg.cancel_hold_ms) return 100u;
    return (held * 100u) / s_cfg.cancel_hold_ms;
}

const char *sos_state_name(sos_state_t s) {
    switch (s) {
        case SOS_IDLE:      return "IDLE";
        case SOS_ARMED:     return "ARMED";
        case SOS_ACTIVE:    return "ACTIVE";
        case SOS_CANCELLED: return "CANCELLED";
        default:            return "?";
    }
}

const char *sos_trigger_name(sos_trigger_t t) {
    switch (t) {
        case SOS_TRIGGER_NONE:            return "none";
        case SOS_TRIGGER_CRITICAL_TRIAGE: return "critical triage";
        case SOS_TRIGGER_CONTACT_LOST:    return "contact lost";
        case SOS_TRIGGER_MANUAL:          return "manual";
        default:                          return "?";
    }
}
