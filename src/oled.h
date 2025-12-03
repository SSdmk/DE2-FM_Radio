#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/**
 * @file
 * @brief Rozhranie pre ovládanie OLED displeja (I2C, 128x64).
 *
 * Tento modul poskytuje základné funkcie na:
 * - inicializáciu OLED displeja,
 * - mazanie obrazovky,
 * - vykresľovanie znakov a textových reťazcov,
 * - zobrazenie špeciálnej obrazovky pre FM rádio
 *   (frekvencia, hlasitosť, RSSI, stav mute),
 * - krátke informačné hlášky (napr. uložená stanica),
 * - zobrazenie stavu vypnutého rádia.
 */

/**
 * @brief Inicializuje OLED displej a pripraví ho na použitie.
 *
 * Nastaví komunikačný režim (TWI/I2C), pošle základné inicializačné
 * príkazy do displeja a na záver vymaže obrazovku volaním @ref oled_clear.
 * Funkciu je potrebné zavolať raz po štarte programu.
 */
void oled_init(void);

/**
 * @brief Vymaže celý obsah OLED displeja.
 *
 * Prejde všetky stránky (pages) displeja a do všetkých stĺpcov zapíše nulu,
 * čím vyčistí celý obraz. Po zavolaní je displej prázdny.
 */
void oled_clear(void);

/**
 * @brief Vykreslí jeden znak fontu 5×7 na zadanú page a stĺpec.
 *
 * Znak je vykreslený na:
 * - vertikálnej stránke @p page (0–7),
 * - aktuálnej pozícii @p *col v rámci šírky displeja.
 *
 * Po vykreslení:
 * - sa medzi znaky pridáva jedna prázdna kolóna,
 * - ukazovateľ @p col je automaticky posunutý za vykreslený znak
 *   (typicky o 6 pixelových stĺpcov).
 *
 * @param page Stránka (vertikálny blok) displeja, 0–7.
 * @param col  Ukazovateľ na aktuálny stĺpec; po vykreslení bude posunutý.
 * @param c    Znak, ktorý sa má vykresliť.
 */
void oled_draw_char(uint8_t page, uint8_t *col, char c);

/**
 * @brief Vykreslí C-reťazec na danú stránku od zadaného stĺpca.
 *
 * Postupne volá @ref oled_draw_char pre každý znak reťazca, kým:
 * - nenarazí na koniec reťazca (`'\0'`),
 * - alebo nenarazí na koniec riadku (šírku displeja).
 *
 * @param page Stránka (vertikálny blok) displeja, 0–7.
 * @param col  Počiatočný stĺpec, odkiaľ sa začne text vykresľovať.
 * @param s    Ukazovateľ na nulou ukončený C-reťazec.
 */
void oled_draw_string(uint8_t page, uint8_t col, const char *s);

/**
 * @brief Zobrazí hlavnú obrazovku rádia (frekvencia, hlasitosť, RSSI, mute).
 *
 * Funkcia typicky:
 * - vykreslí hlavičku („FM Radio“ alebo informáciu o mute),
 * - vo väčšom fonte zobrazí aktuálnu frekvenciu (napr. `107.0MHz`),
 * - v spodnej časti zobrazí hlasitosť a RSSI.
 *
 * @param freq_khz Frekvencia v kHz (napr. 10700 → 107.0 MHz).
 * @param volume   Aktuálna hlasitosť (0–15 alebo podľa implementácie).
 * @param rssi     Hodnota RSSI (úroveň signálu z tunera).
 * @param muted    @c true ak je zvuk stlmený, inak @c false.
 */
void oled_show_radio_screen(int freq_khz, int volume, int rssi, bool muted);

/**
 * @brief Krátko zobrazí informáciu o uložení obľúbenej stanice v spodnej časti.
 *
 * Funkcia vykreslí text v štýle „FAVORITE 107.0MHz“ na spodnej časti
 * displeja tak, aby neprepísala hlavnú časť obrazovky rádia. Typicky sa používa
 * po úspešnom uložení aktuálnej frekvencie medzi obľúbené stanice.
 *
 * @param freq_khz Frekvencia v kHz, ktorá bola práve uložená.
 */
void oled_show_favorite_saved_bottom(int freq_khz);

/**
 * @brief Zobrazí informáciu, že FM rádio je vypnuté (power off).
 *
 * Funkcia vyčistí horný riadok displeja a zobrazí text
 * „FM Radio is power off“. Je určená na použitie v stave, keď je
 * tuner vypnutý (`radio.powerDown()`) a nechceme zobrazovať ani mute,
 * ani bežnú hlavnú obrazovku rádia.
 */
void oled_show_power_off(void);

#endif
