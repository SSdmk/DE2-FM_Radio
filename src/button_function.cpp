#include <stdbool.h>
#include "button_function.h"
#include "oled.h"

/// @file
/// @brief Implementácia obsluhy UI udalostí (tlačidlá + enkóder) pre FM rádio.
///
/// Tento modul:
/// - drží interný stav rádia (zapnuté/vypnuté, obľúbená stanica, režim enkódera),
/// - mapuje udalosti z UI (`ui_event_t`) na konkrétne akcie nad objektom `Si4703`,
/// - pri niektorých akciách aktualizuje OLED (napr. uloženie obľúbenej stanice).

// !!! Uprav podľa reálneho názvu hlavičky s funkciami rádia
// Na screenshote boli funkcie ako incChannel(), seekUp(), incVolume()...
#include "Si4703.h"   // alebo napr. "radio.h"

/// @brief Globálny objekt FM rádia (deklarovaný inde).
extern Si4703 radio;

/// @brief Aktuálna frekvencia rádia v kHz (deklarovaná inde).
extern int current_freq_khz;

/* --------------------------------------------------------------------------
 * REŽIM ENKÓDERA A VNÚTORNÝ STAV
 * --------------------------------------------------------------------------*/

/// @brief Interný stav – aktuálny mód enkódera (hlasitosť alebo ladenie).
static radio_mode_t s_mode = RADIO_MODE_VOLUME;

/// @brief Uložená „obľúbená“ frekvencia v kHz (0 = zatiaľ nenastavená).
static int s_favorite_freq = 0;
// static bool s_has_favorite = false;  // prípadný dodatočný príznak

/// @brief Interný stav – či je rádio zapnuté (true) alebo vypnuté (false).
static bool s_radio_on = true;   // po štarte v setup() rádio zapneme

/* --------------------------------------------------------------------------
 * Inicializácia UI vrstvy rádia
 * --------------------------------------------------------------------------*/

/**
 * @brief Inicializuje interný stav UI logiky pre rádio.
 *
 * Nastaví predvolený režim enkódera na ::RADIO_MODE_VOLUME a
 * označí rádio ako zapnuté (predpokladá sa, že v `setup()` bolo
 * zavolané `radio.start()` alebo ekvivalentná inicializácia tunera).
 */
void radio_ui_init(void)
{
    // Default: enkóder ovláda hlasitosť
    s_mode = RADIO_MODE_VOLUME;
    // Voliteľne: tu môžeš nastaviť nejaké default hodnoty
    s_radio_on = true;   // v setup() voláme radio.start(), takže je zapnuté
}

/* --------------------------------------------------------------------------
 * Getter na aktuálny mód (aby ho vedel použiť napr. displej)
 * --------------------------------------------------------------------------*/

/**
 * @brief Vráti aktuálny režim enkódera.
 *
 * @return Aktuálny režim enkódera (::RADIO_MODE_VOLUME alebo ::RADIO_MODE_TUNE).
 */
radio_mode_t radio_ui_get_mode(void)
{
    return s_mode;
}

/**
 * @brief Zistí, či je rádio podľa UI logiky aktuálne zapnuté.
 *
 * Funkcia vracia vnútorný stav @ref s_radio_on, ktorý sa mení najmä
 * v rámci @ref radio_ui_handle_event pri udalosti ::UI_BTN_DOWN_LONG.
 *
 * @return @c true ak je rádio ON, @c false ak je vypnuté (po `radio.powerDown()`).
 */
bool radio_ui_is_on(void)
{
    return s_radio_on;
}

/// @brief Pomocná funkcia na prepnutie napájania rádia (ON/OFF).
///
/// Ak je rádio zapnuté, zavolá `radio.powerDown()` a nastaví @ref s_radio_on na false.
/// Ak je vypnuté, zavolá `radio.powerUp()` a nastaví @ref s_radio_on na true.
static void radio_toggle_power(void)
{
    if (s_radio_on) {
        // Rádio je zapnuté -> vypneme
        radio.powerDown();   // funkcia z knižnice Si4703 
        s_radio_on = false;
    } else {
        // Rádio je vypnuté -> znovu zapneme
        radio.powerUp();     // alebo radio.start(), ak chceš komplet re-init
        s_radio_on = true;
    }
}


/* --------------------------------------------------------------------------
 * Hlavná funkcia – spracuje jednu udalosť z UI
 * Volaj ju z main() vždy, keď ti input vrstva vráti nejaký ui_event_t.
 * --------------------------------------------------------------------------*/

/**
 * @brief Spracuje jednu udalosť z používateľského rozhrania.
 *
 * Podľa hodnoty @p ev vykoná:
 * - krok seeku hore/dole (ľavé/pravé tlačidlo),
 * - naladenie/uloženie obľúbenej frekvencie (horné tlačidlo),
 * - prepnutie mute, resp. zapnutie/vypnutie rádia (dolné tlačidlo),
 * - zmenu hlasitosti alebo frekvencie (otáčanie enkódera),
 * - prepnutie režimu enkódera VOLUME/TUNE (klik na enkóder).
 *
 * Funkcia predstavuje čistú UI logiku nad globálnym objektom `radio`
 * (Si4703 tuner) a OLED displejom.
 *
 * @param ev Udalosť prijatá z vrstvy vstupu (tlačidlá/enkóder).
 */
void radio_ui_handle_event(ui_event_t ev)
{
    switch (ev) {

    /* ------------ ŠTYRI TLAČIDLÁ ------------ */

    case UI_BTN_LEFT:
    {
        // Preladenie o jeden kanál nižšie (seek smerom nadol)
        radio.seekDown();
        break;
    }

    case UI_BTN_RIGHT:
    {
        // Preladenie o jeden kanál vyššie (seek smerom nahor)
        radio.seekUp();
        break;
    }

    // ───────── HORNÝ BUTTON – krátky stisk = naladiť obľúbenú ─────────
    case UI_BTN_UP_SHORT:
    {
        if (s_favorite_freq != 0) {
            // Ak máme uloženú obľúbenú, naladíme ju
            radio.setChannel(s_favorite_freq);
        }
        // Ak obľúbená ešte nie je, správanie je aktuálne „nič nerobiť“
        // (alternatíva: možnosť vrátiť pôvodné správanie, napr. seekUp()).
        break;
    }

    // ───────── HORNÝ BUTTON – dlhý stisk = uložiť obľúbenú ─────────
    case UI_BTN_UP_LONG: 
    {
        int current = radio.getChannel(); // aktuálna frekvencia v kHz
        s_favorite_freq = current;        // uložíme ako obľúbenú
        // s_has_favorite = true; // ak by sa používal samostatný flag

        // Informácia pre používateľa – zobrazí sa na spodnom riadku OLED
        oled_show_favorite_saved_bottom(current);
        break;
    }

    // ───────── DOLNÝ BUTTON – krátky stisk = MUTE/UNMUTE ─────────
    case UI_BTN_DOWN_SHORT:
    {   
        if (!s_radio_on) {
            // Ak je rádio vypnuté, mute nedáva zmysel – nič nerobíme
            break;
        }

        // Preklopenie stavu mute
        bool muted = radio.getMute();  // zisti aktuálny stav
        radio.setMute(!muted);         // nastav opačný
        break;
    }

    // ───────── DOLNÝ BUTTON – dlhý stisk = POWER ON/OFF ─────────
    case UI_BTN_DOWN_LONG:
    {
        // Dlhé stlačenie DOWN -> prepnutie napájania rádia (ON/OFF)
        radio_toggle_power();
        break;
    }

    /* ------------ ENKÓDER – OTÁČANIE ------------ */

    case UI_ENC_STEP_CW:
    {
        if (s_mode == RADIO_MODE_VOLUME) {
            // Režim hlasitosti – krokovo zvyšuj volume
            radio.incVolume();
        } else {
            // Režim manuálneho ladenia – krok nahor vo frekvencii
            radio.incChannel();
        }
        break;
    }

    case UI_ENC_STEP_CCW:
    {
        if (s_mode == RADIO_MODE_VOLUME) {
            // Režim hlasitosti – krokovo znižuj volume
            radio.decVolume();
        } else {
            // Režim manuálneho ladenia – krok nadol vo frekvencii
            radio.decChannel();
        }
        break;
    }

    /* ------------ ENKÓDER – STLAČENIE ------------ */

    case UI_ENC_CLICK:
    {
        // Prepnutie medzi režimom hlasitosti a ladenia
        if (s_mode == RADIO_MODE_VOLUME) {
            s_mode = RADIO_MODE_TUNE;
        } else {
            s_mode = RADIO_MODE_VOLUME;
        }
        // Tu sa dá neskôr doplniť vizuálna indikácia režimu na OLED
        break;
    }

    /* ------------ ŽIADNA / NEZNÁMA UDALOSŤ ------------ */

    case UI_EVENT_NONE:
    {
    default:
        // Žiadna akcia – ignorujeme
        break;
    }
}   }
