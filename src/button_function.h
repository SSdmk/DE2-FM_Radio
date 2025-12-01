#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * UDALOSTI Z UI (tlačidlá + enkóder)
 * --------------------------------------------------------------------------*/
typedef enum {
    UI_EVENT_NONE = 0,

    UI_BTN_LEFT,          // ľavé tlačidlo
    UI_BTN_RIGHT,         // pravé tlačidlo
    UI_BTN_UP_SHORT,      // horné tlačidlo stlacene kratko
    UI_BTN_UP_LONG,       // horne tlacidlo sltacene dlho
    UI_BTN_DOWN_SHORT,    // dolné tlačidlo stlacene kratko
    UI_BTN_DOWN_LONG,     // dolné tlačidlo stlacene dlho

    UI_ENC_STEP_CW,       // enkóder otočený doprava (clockwise)
    UI_ENC_STEP_CCW,      // enkóder otočený doľava (counter-clockwise)
    UI_ENC_CLICK          // krátke stlačenie enkódera
} ui_event_t;

/* --------------------------------------------------------------------------
 * REŽIM ENKÓDERA
 * --------------------------------------------------------------------------*/
typedef enum {
    RADIO_MODE_VOLUME = 0,   // enkóder mení hlasitosť
    RADIO_MODE_TUNE          // enkóder mení naladený kanál (frekvenciu)
} radio_mode_t;

/* --------------------------------------------------------------------------
 * API tejto vrstvy
 * --------------------------------------------------------------------------*/

/**
 * @brief Inicializácia UI vrstvy rádia.
 * Zavolaj raz v main() po inicializácii rádia.
 */
void radio_ui_init(void);

/**
 * @brief Vráti aktuálny režim enkódera (VOLUME / TUNE).
 */
radio_mode_t radio_ui_get_mode(void);

/**
 * @brief Spracuje jednu udalosť z tlačidiel/enkódera.
 * Túto funkciu volaj vždy, keď ti vstupná vrstva vráti nejaký ui_event_t.
 */
void radio_ui_handle_event(ui_event_t ev);

#ifdef __cplusplus
}
#endif

#endif // BUTTONS_H
