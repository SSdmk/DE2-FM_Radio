#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

/**
 * @file
 * @brief Deklarácie UI udalostí a rozhrania pre obsluhu tlačidiel a enkódera rádia.
 *
 * Tento modul definuje:
 * - typy udalostí z používateľského rozhrania (tlačidlá, enkóder),
 * - režimy práce enkódera (hlasitosť, ladenie),
 * - API funkcie na inicializáciu a spracovanie udalostí,
 * - pomocnú funkciu na zistenie ON/OFF stavu rádia z pohľadu UI vrstvy.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * UDALOSTI Z UI (tlačidlá + enkóder)
 * --------------------------------------------------------------------------*/

/**
 * @brief Výčtový typ reprezentujúci všetky možné udalosti z UI.
 *
 * Zastrešuje udalosti z:
 * - štyroch tlačidiel (LEFT, RIGHT, UP, DOWN) s rozlíšením krátkeho/dlhého stlačenia,
 * - rotačného enkódera (kroky CW/CCW + klik).
 */
typedef enum {
    UI_EVENT_NONE = 0,     /**< Žiadna udalosť. */

    UI_BTN_LEFT,           /**< Krátke stlačenie ľavého tlačidla. */
    UI_BTN_RIGHT,          /**< Krátke stlačenie pravého tlačidla. */
    UI_BTN_UP_SHORT,       /**< Krátke stlačenie horného tlačidla. */
    UI_BTN_UP_LONG,        /**< Dlhé stlačenie horného tlačidla. */
    UI_BTN_DOWN_SHORT,     /**< Krátke stlačenie dolného tlačidla. */
    UI_BTN_DOWN_LONG,      /**< Dlhé stlačenie dolného tlačidla. */

    UI_ENC_STEP_CW,        /**< Enkóder otočený doprava (clockwise). */
    UI_ENC_STEP_CCW,       /**< Enkóder otočený doľava (counter-clockwise). */
    UI_ENC_CLICK           /**< Krátke stlačenie tlačidla enkódera. */
} ui_event_t;

/* --------------------------------------------------------------------------
 * REŽIM ENKÓDERA
 * --------------------------------------------------------------------------*/

/**
 * @brief Režim, v ktorom aktuálne pracuje rotačný enkóder.
 *
 * - ::RADIO_MODE_VOLUME – enkóder mení hlasitosť rádia,
 * - ::RADIO_MODE_TUNE – enkóder mení naladenú frekvenciu.
 */
typedef enum {
    RADIO_MODE_VOLUME = 0, /**< Enkóder ovláda hlasitosť. */
    RADIO_MODE_TUNE        /**< Enkóder ovláda naladený kanál/frekvenciu. */
} radio_mode_t;

/* --------------------------------------------------------------------------
 * API tejto vrstvy
 * --------------------------------------------------------------------------*/

/**
 * @brief Zistí, či je rádio podľa UI logiky aktuálne zapnuté.
 *
 * Funkcia nečítá priamo HW stav, ale vracia vnútorný stav,
 * ktorý udržiava UI vrstva (napr. po volaní @c radio.powerUp() /
 * @c radio.powerDown() cez @ref radio_ui_handle_event).
 *
 * @return @c true ak je rádio v stave ON, @c false ak je rádio vypnuté (powerDown).
 */
bool radio_ui_is_on(void);

/**
 * @brief Inicializácia UI vrstvy rádia.
 *
 * Zavolaj raz v `main()` po inicializácii rádia (a prípadne ďalších periférií),
 * aby sa pripravili interné štruktúry pre obsluhu tlačidiel a enkódera
 * (prednastavenie režimu, defaultný stav ON/OFF a pod.).
 */
void radio_ui_init(void);

/**
 * @brief Získa aktuálny režim enkódera.
 *
 * @return Aktuálny režim enkódera, napr. ::RADIO_MODE_VOLUME alebo ::RADIO_MODE_TUNE.
 */
radio_mode_t radio_ui_get_mode(void);

/**
 * @brief Spracuje jednu udalosť z tlačidiel alebo enkódera.
 *
 * Túto funkciu volaj vždy, keď vstupná vrstva (Button/RotaryEncoder) deteguje niektorý
 * z typov ::ui_event_t a chceš ho premietnuť do akcie v UI (zmena hlasitosti,
 * ladenia, prepínanie režimov, mute, power on/off a pod.).
 *
 * Typické použitie v hlavnej slučke:
 * - prečítať stav tlačidiel a enkódera,
 * - preložiť ho na ::ui_event_t,
 * - odovzdať sem na spracovanie.
 *
 * @param ev Udalosť z používateľského rozhrania, ktorú chceš spracovať.
 */
void radio_ui_handle_event(ui_event_t ev);

#ifdef __cplusplus
}
#endif

#endif // BUTTONS_H
