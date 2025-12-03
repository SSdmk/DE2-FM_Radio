#include <stdbool.h>
#include "button_function.h"
#include "oled.h"

// !!! Uprav podľa reálneho názvu hlavičky s funkciami rádia
// Na screenshote boli funkcie ako incChannel(), seekUp(), incVolume()...
#include "Si4703.h"   // alebo napr. "radio.h"
extern Si4703 radio;
extern int current_freq_khz;
/* --------------------------------------------------------------------------
 * UDALOSTI Z UI (tlačidlá + enkóder)
 * Tieto hodnoty má vracať tvoj input modul (debounce + enkóder logika).
 * --------------------------------------------------------------------------*/

/* --------------------------------------------------------------------------
 * REŽIM ENKÓDERA
 * --------------------------------------------------------------------------*/

// Interný stav – aktuálny mód enkódera (VOLUME / TUNE)
static radio_mode_t s_mode = RADIO_MODE_VOLUME;

// defaultna hodnota a definicia oblubenej stanice
static int s_favorite_freq = 0;
//static bool s_has_favorite = false;

static bool s_radio_on = true;   // po starte v setup() rádio zapneme

/* --------------------------------------------------------------------------
 * Inicializácia UI vrstvy rádia
 * --------------------------------------------------------------------------*/
void radio_ui_init(void)
{
    // Default: enkóder ovláda hlasitosť
    s_mode = RADIO_MODE_VOLUME;
    // Voliteľne: tu môžeš nastaviť nejaké default hodnoty,
    s_radio_on = true;   // v setup() voláme radio.start(), takže je zapnuté
}

/* --------------------------------------------------------------------------
 * Getter na aktuálny mód (aby ho vedel použiť napr. displej)
 * --------------------------------------------------------------------------*/
radio_mode_t radio_ui_get_mode(void)
{
    return s_mode;
}

//pomocna funkcia pre zapnutie a vypnutie radia
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
void radio_ui_handle_event(ui_event_t ev)
{
    switch (ev) {

    /* ------------ ŠTYRI TLAČIDLÁ ------------ */

    case UI_BTN_LEFT:
    {
        // preladenie o jeden kanál nižšie
        radio.seekDown();
        break;
    }

    case UI_BTN_RIGHT:
    {
        // preladenie o jeden kanál vyššie
        radio.seekUp();
        break;
    }

     // ───────── HORNÝ BUTTON – krátky stisk = naladiť obľúbenú ─────────
    case UI_BTN_UP_SHORT:
    {
        if (s_favorite_freq != 0) {
            // ak máme uloženú obľúbenú, naladíme ju
            radio.setChannel(s_favorite_freq);
        }
        // ak obľúbená ešte nie je, môžeš:
        // - buď nerobiť nič
        // - alebo použiť pôvodné správanie (seekUp())
        break;
    }
    // ───────── HORNÝ BUTTON – dlhý stisk = uložiť obľúbenú ─────────
    case UI_BTN_UP_LONG: 
    {
        int current = radio.getChannel(); // aktuálna frekvencia v kHz
        s_favorite_freq = current; // uložíme ako obľúbenú
        // s_has_favorite = true; // ak používaš flag
        // sem prípadne neskôr dáš nejaký indikátor na displej
        // >>> tu pridaj volanie na OLED <<<
        oled_show_favorite_saved_bottom(current);
        break;
    }

    case UI_BTN_DOWN_SHORT:
    {   
        if (!s_radio_on) {
        // ak je rádio vypnuté, mute preskocime
        break;
        }
        // automatické vyhľadanie najbližšej nižšej stanice
        bool muted = radio.getMute();
        // zisti aktuálny stav
        radio.setMute(!muted);
        // nastav opačný 
        break;
    }
    /* ------------ ENKÓDER – OTÁČANIE ------------ */

    case UI_BTN_DOWN_LONG:
    {
        // dlhe stlačenie DOWN -> toggle ON/OFF
        radio_toggle_power();
        break;
    }

    case UI_ENC_STEP_CW:
    {
        if (s_mode == RADIO_MODE_VOLUME) {
            // režim hlasitosti
            radio.incVolume();
        } else {
            // režim manuálneho ladenia
            radio.incChannel();
        }
        break;
    }

    case UI_ENC_STEP_CCW:
    {
        if (s_mode == RADIO_MODE_VOLUME) {
            radio.decVolume();
        } else {
            radio.decChannel();
        }
        break;
    }
    /* ------------ ENKÓDER – STLAČENIE ------------ */

    case UI_ENC_CLICK:
    {
        // prepnutie medzi režimom hlasitosti a ladenia
        if (s_mode == RADIO_MODE_VOLUME) {
            s_mode = RADIO_MODE_TUNE;
        } else {
            s_mode = RADIO_MODE_VOLUME;
        }
        // sem neskôr môžete pridať volanie na displej,
        // napr. display_show_mode(s_mode);
        break;
    }
    /* ------------ ŽIADNA / NEZNÁMA UDALOSŤ ------------ */

    case UI_EVENT_NONE:
    {
    default:
        // nič nerobíme
        break;
    }
}   }

