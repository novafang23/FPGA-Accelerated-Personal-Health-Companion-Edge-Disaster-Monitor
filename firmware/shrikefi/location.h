/*
 * location.h — Stored deployment location (T1.4, requirement 6 bullet 3)
 * SIH26181 / VALOR: Personal Health Companion & Edge Disaster Monitor
 *
 * Shown on the emergency card and on the local status page, so that whoever
 * responds knows WHERE as well as what. Set once by the caregiver, then it
 * survives power cycles.
 *
 * WHY TEXT AND ALSO GPS
 * The board does carry a GNSS receiver (u-blox NEO-M8N on UART2, see
 * gnss_nmea.c), so this stored text is not a replacement for a fix - it is the
 * fallback. main_shrikefi.c applies the precedence when it publishes a status
 * snapshot: a live fix with a fresh position wins, and the stored text is used
 * when there is no fix (indoors, under debris, antenna shielded) or once the
 * fix expires five seconds after the last valid sentence. The OLED card and the
 * SoftAP page both read that same published value, so they cannot disagree.
 *
 * The fallback exists for two reasons. Indoors or under debris a fix often does
 * not arrive at all, and "Village / Block / District" is how people in rural
 * India actually describe where they are and how a responder is dispatched. A
 * latitude that cannot be resolved to a place name is not more useful than the
 * place name - it is less. There is no reverse geocoding on the board, so the
 * fix is shown as raw coordinates and never as a guessed address.
 *
 * WHY THE PARSING IS PURE
 * location_sanitize() and location_parse_form() take bytes and return bytes:
 * no NVS, no ESP-IDF, no allocation. Everything that decides what actually gets
 * stored is therefore exercised by the host test suite, and only the small
 * persistence wrapper needs a board.
 */

#ifndef SHRIKEFI_LOCATION_H
#define SHRIKEFI_LOCATION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Longest stored location, including the terminator.
 *
 * 32 is a compromise between the two places it is displayed. The phone page has
 * room for anything; the 128 px emergency card has 16 usable characters after
 * its "LOC: " prefix. Rather than truncate silently on the card - a cut-off
 * district name is worse than a short one - the card marks a shortened value
 * with "..." so a reader knows to check the phone page for the full string. */
#define LOCATION_MAX_LEN 32
#define LOCATION_CARD_COLS 16

/* Clean free text into something safe to store and display.
 *
 * Trims leading and trailing whitespace, drops bytes outside printable ASCII
 * (control characters, and any high-bit byte that could split a UTF-8 sequence
 * mid-glyph), and truncates to LOCATION_MAX_LEN - 1 characters.
 *
 * Returns false, leaving `out` empty, if nothing usable survives. Callers must
 * treat that as "not set" rather than storing an empty string. */
bool location_sanitize(const char *in, char *out, size_t cap);

/* Extract the `loc` field from an application/x-www-form-urlencoded body, then
 * sanitise it. `+` decodes to a space and %XX to a byte, so a phone keyboard's
 * "Ward 3, Kolar" arrives intact.
 *
 * Returns false if there is no loc field or it sanitises to nothing. */
bool location_parse_form(const char *body, char *out, size_t cap);

/* Render the location for the emergency card: as much as fits in
 * LOCATION_CARD_COLS, with "..." appended when it had to be shortened. Pure, so
 * the shortening rule is tested rather than eyeballed on a 0.96" panel. */
void location_card_text(const char *loc, char *out, size_t cap);

/* Load the stored location at boot. Safe to call more than once. */
void location_init(void);

/* Sanitise and persist. Returns false if the text sanitised to nothing, in
 * which case the stored value is left unchanged. Persistence failures are
 * logged and still update the in-memory value, so a page set during an
 * emergency is visible even if NVS is unavailable. */
bool location_set(const char *raw);

/* Never NULL. Reads "UNSET" until a location has been set. */
const char *location_get(void);

bool location_is_set(void);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_LOCATION_H */
