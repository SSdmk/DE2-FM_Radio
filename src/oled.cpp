#include <avr/io.h> 
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include "twi.h"
#include "oled.h"

/**
 * @file
 * @brief Implementácia funkcií na ovládanie OLED displeja (I2C, 128x64).
 */

/**
 * @brief I2C adresa OLED displeja.
 */
#define OLED_ADDR 0x3C
/**
 * @brief Hodnota na označenie príkazových bajtov pri I2C prenose.
 */
#define OLED_CMD  0x00
/**
 * @brief Hodnota na označenie dátových bajtov pri I2C prenose.
 */
#define OLED_DATA 0x40


// --- Font --- //

/**
 * @brief Štruktúra jedného znaku 5x7 v bitmapovej podobe.
 *
 * Každý znak má:
 * - ASCII kód @ref c,
 * - pole 5 stĺpcov @ref data, kde každý bit reprezentuje pixel v stĺpci.
 */
typedef struct {
    char c;          ///< ASCII znak
    uint8_t data[5]; ///< Bitmapa znaku (5 vertikálnych stĺpcov)
} Font5x7Char;

/**
 * @brief Tabuľka znakov fontu 5x7 (číslice, veľké a malé písmená).
 *
 * Tabuľka obsahuje:
 * - medzeru a bodku,
 * - číslice 0–9,
 * - veľké písmená A–Z,
 * - malé písmená a–z.
 *
 * Pri vykresľovaní sa podľa znaku vyhľadá zodpovedajúca bitmapa.
 */
static const Font5x7Char font_table[] = {
    // medzera + bodka
    { ' ', {0x00,0x00,0x00,0x00,0x00} },
    { '.', {0x00,0x00,0x00,0x18,0x18} },

    // číslice
    { '0',{0x3E,0x51,0x49,0x45,0x3E} },
    { '1',{0x00,0x42,0x7F,0x40,0x00} },
    { '2',{0x42,0x61,0x51,0x49,0x46} },
    { '3',{0x21,0x41,0x45,0x4B,0x31} },
    { '4',{0x18,0x14,0x12,0x7F,0x10} },
    { '5',{0x27,0x45,0x45,0x45,0x39} },
    { '6',{0x3C,0x4A,0x49,0x49,0x30} },
    { '7',{0x01,0x71,0x09,0x05,0x03} },
    { '8',{0x36,0x49,0x49,0x49,0x36} },
    { '9',{0x06,0x49,0x49,0x29,0x1E} },

    // veľké písmená A–Z
    { 'A',{0x7E,0x09,0x09,0x09,0x7E} },
    { 'B',{0x7F,0x49,0x49,0x49,0x36} },
    { 'C',{0x3E,0x41,0x41,0x41,0x22} },
    { 'D',{0x7F,0x41,0x41,0x22,0x1C} },
    { 'E',{0x7F,0x49,0x49,0x49,0x41} },
    { 'F',{0x7F,0x09,0x09,0x09,0x01} },
    { 'G',{0x3E,0x41,0x49,0x49,0x3A} },
    { 'H',{0x7F,0x08,0x08,0x08,0x7F} },
    { 'I',{0x00,0x41,0x7F,0x41,0x00} },
    { 'J',{0x20,0x40,0x41,0x3F,0x01} },
    { 'K',{0x7F,0x08,0x14,0x22,0x41} },
    { 'L',{0x7F,0x40,0x40,0x40,0x40} },
    { 'M',{0x7F,0x02,0x0C,0x02,0x7F} },
    { 'N',{0x7F,0x06,0x18,0x60,0x7F} },
    { 'O',{0x3E,0x41,0x41,0x41,0x3E} },
    { 'P',{0x7F,0x09,0x09,0x09,0x06} },
    { 'Q',{0x3E,0x41,0x51,0x21,0x5E} },
    { 'R',{0x7F,0x09,0x19,0x29,0x46} },
    { 'S',{0x26,0x49,0x49,0x49,0x32} },
    { 'T',{0x01,0x01,0x7F,0x01,0x01} },
    { 'U',{0x3F,0x40,0x40,0x40,0x3F} },
    { 'V',{0x07,0x38,0x40,0x38,0x07} },
    { 'W',{0x7F,0x20,0x18,0x20,0x7F} },
    { 'X',{0x63,0x14,0x08,0x14,0x63} },
    { 'Y',{0x03,0x04,0x78,0x04,0x03} },
    { 'Z',{0x61,0x51,0x49,0x45,0x43} },

    // malé písmená a–z
    { 'a',{0x20,0x54,0x54,0x54,0x78} },
    { 'b',{0x7F,0x48,0x44,0x44,0x38} },
    { 'c',{0x38,0x44,0x44,0x44,0x20} },
    { 'd',{0x38,0x44,0x44,0x48,0x7F} },
    { 'e',{0x38,0x54,0x54,0x54,0x18} },
    { 'f',{0x08,0x7E,0x09,0x01,0x02} },
    { 'g',{0x0C,0x52,0x52,0x52,0x3E} },
    { 'h',{0x7F,0x08,0x04,0x04,0x78} },
    { 'i',{0x00,0x44,0x7D,0x40,0x00} },
    { 'j',{0x20,0x40,0x44,0x3D,0x00} },
    { 'k',{0x7F,0x10,0x28,0x44,0x00} },
    { 'l',{0x00,0x41,0x7F,0x40,0x00} },
    { 'm',{0x7C,0x04,0x18,0x04,0x78} },
    { 'n',{0x7C,0x08,0x04,0x04,0x78} },
    { 'o',{0x38,0x44,0x44,0x44,0x38} },
    { 'p',{0x7C,0x14,0x14,0x14,0x08} },
    { 'q',{0x08,0x14,0x14,0x18,0x7C} },
    { 'r',{0x7C,0x08,0x04,0x04,0x08} },
    { 's',{0x48,0x54,0x54,0x54,0x20} },
    { 't',{0x04,0x3F,0x44,0x40,0x20} },
    { 'u',{0x3C,0x40,0x40,0x20,0x7C} },
    { 'v',{0x1C,0x20,0x40,0x20,0x1C} },
    { 'w',{0x3C,0x40,0x30,0x40,0x3C} },
    { 'x',{0x44,0x28,0x10,0x28,0x44} },
    { 'y',{0x0C,0x50,0x50,0x50,0x3C} },
    { 'z',{0x44,0x64,0x54,0x4C,0x44} },
};

/**
 * @brief Počet položiek v tabuľke fontu @ref font_table.
 */
#define FONT_TABLE_SIZE (sizeof(font_table)/sizeof(Font5x7Char))

/**
 * @brief Vyhľadá bitmapu znaku v tabuľke fontu.
 *
 * Funkcia prejde @ref font_table a vráti ukazovateľ na bitmapu
 * zodpovedajúceho znaku. Ak znak v tabuľke nie je, vráti bitmapu
 * medzery (prvý prvok tabuľky).
 *
 * @param c Znak, ktorého bitmapu chceme získať.
 * @return Ukazovateľ na pole 5 bajtov reprezentujúcich bitmapu znaku.
 */
static const uint8_t* font_get_char(char c) {
    for (uint8_t i = 0; i < FONT_TABLE_SIZE; i++) {
        if (font_table[i].c == c) {
            return font_table[i].data;
        }
    }
    // fallback – medzera
    return font_table[0].data;
}


// --- OLED I2C pomocné funkcie --- //

/**
 * @brief Pošle jeden príkazový bajt OLED displeju cez I2C.
 *
 * Vykoná sekvenciu:
 * - START,
 * - SLA+W,
 * - označenie príkazu @ref OLED_CMD,
 * - samotný príkaz @p cmd,
 * - STOP.
 *
 * @param cmd Príkazový bajt pre OLED.
 */
static void oled_send_command(uint8_t cmd) {
    twi_start();
    twi_write((OLED_ADDR << 1) | 0);
    twi_write(OLED_CMD);
    twi_write(cmd);
    twi_stop();
}

/**
 * @brief Začne I2C prenos dát pre OLED (bez ukončenia STOP).
 *
 * Po zavolaní je možné posielať dáta funkciou @ref oled_send_data_byte
 * a prenos sa ukončí až volaním @ref oled_send_data_stop.
 */
static void oled_send_data_start(void) {
    twi_start();
    twi_write((OLED_ADDR << 1) | 0);
    twi_write(OLED_DATA);
}

/**
 * @brief Pošle jeden dátový bajt OLED displeju.
 *
 * Pred volaním musí byť pripravený dátový režim
 * pomocou @ref oled_send_data_start.
 *
 * @param b Dátový bajt (stĺpec pixelov).
 */
static void oled_send_data_byte(uint8_t b) { twi_write(b); }

/**
 * @brief Ukončí aktuálny I2C prenos dát (vygeneruje STOP).
 */
static void oled_send_data_stop(void) { twi_stop(); }

/**
 * @brief Nastaví pozíciu kurzora na danú stránku a stĺpec.
 *
 * OLED displej má:
 * - stránkový režim (page 0–7) – vertikálne bloky po 8 pixeloch,
 * - 128 stĺpcov (adresované dolnými a hornými 4 bitmi).
 *
 * Funkcia nastaví:
 * - číslo stránky (0xB0 | page),
 * - dolných 4 bity stĺpca,
 * - horných 4 bity stĺpca.
 *
 * @param page Číslo stránky (0–7).
 * @param col  Stĺpec, od ktorého sa bude kresliť.
 */
static void oled_set_pos(uint8_t page, uint8_t col) {
    oled_send_command(0xB0 | (page & 0x07));
    oled_send_command(0x00 | (col & 0x0F));
    oled_send_command(0x10 | ((col >> 4) & 0x0F));
}


// --- Čistenie displeja --- //

/**
 * @brief Vymaže celý OLED displej (všetky stránky a stĺpce).
 *
 * Pre každú stránku (0–7):
 * - nastaví stĺpec na 0,
 * - zapíše 132 bajtov hodnôt 0x00 (čierne pixely).
 */
void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_set_pos(page, 0);
        oled_send_data_start();
        for (uint8_t i = 0; i < 132; i++) oled_send_data_byte(0x00);
        oled_send_data_stop();
    }
}


// --- Textové funkcie --- //

/**
 * @brief Vykreslí jeden znak fontu 5x7 na danú stránku a stĺpec.
 *
 * Postup:
 * - získa bitmapu znaku cez @ref font_get_char,
 * - nastaví pozíciu kurzora na @p page a @p *col,
 * - postupne pošle 5 stĺpcov bitmapy,
 * - pridá jeden prázdny stĺpec ako medzeru medzi znakmi,
 * - po vykreslení posunie @p *col o 6 stĺpcov.
 *
 * @param page Stránka (0–7), na ktorej sa má znak vykresliť.
 * @param col  Ukazovateľ na aktuálny stĺpec; po vykreslení bude posunutý.
 * @param c    Znak, ktorý sa má vykresliť.
 */
void oled_draw_char(uint8_t page, uint8_t *col, char c) {
    const uint8_t *glyph = font_get_char(c);
    oled_set_pos(page, *col);
    oled_send_data_start();
    for (uint8_t i = 0; i < 5; i++) oled_send_data_byte(glyph[i]);
    oled_send_data_byte(0x00);
    oled_send_data_stop();
    *col += 6;
}

/**
 * @brief Vykreslí C-reťazec na danú stránku od zvoleného stĺpca.
 *
 * Iteratívne volá @ref oled_draw_char, kým:
 * - nenarazí na koniec reťazca,
 * - alebo by ďalší znak presiahol šírku displeja (kontrola @p col <= 122).
 *
 * @param page Stránka (0–7), na ktorej sa text vykresľuje.
 * @param col  Počiatočný stĺpec.
 * @param s    Nulou ukončený C-reťazec.
 */
void oled_draw_string(uint8_t page, uint8_t col, const char *s) {
    while (*s && col <= 122) oled_draw_char(page, &col, *s++);
}

/**
 * @brief Vykreslí jeden „zväčšený“ znak – jednoduchý väčší font.
 *
 * Základný znak 5x7 je roztiahnutý horizontálne:
 * - každý druhý stĺpec sa duplikuje,
 * - výsledná šírka znaku je väčšia (viac pixelov).
 *
 * Používa sa pre výrazné zobrazenie frekvencie rádia.
 *
 * @param page Stránka, kde sa má znak vykresliť.
 * @param col  Ukazovateľ na stĺpec; po vykreslení sa posunie o 9.
 * @param c    Znak, ktorý sa má vykresliť.
 */
void oled_draw_char_big(uint8_t page, uint8_t *col, char c) {
    const uint8_t *glyph = font_get_char(c);
    oled_set_pos(page, *col);
    oled_send_data_start();
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t d = glyph[i];
        oled_send_data_byte(d);
        if (i % 2 == 0) oled_send_data_byte(d);
    }
    oled_send_data_byte(0);
    oled_send_data_stop();
    *col += 9;
}

/**
 * @brief Vykreslí reťazec vo „väčšom“ fonte (používa @ref oled_draw_char_big).
 *
 * Vhodné pre zobrazenie frekvencie (napr. „107.0MHz“) viac na veľko.
 *
 * @param page Stránka, kde sa má text vykresliť.
 * @param col  Počiatočný stĺpec.
 * @param s    Nulou ukončený C-reťazec.
 */
void oled_draw_string_big(uint8_t page, uint8_t col, const char *s) {
    while (*s && col <= 118) oled_draw_char_big(page, &col, *s++);
}


// --- Inicializácia OLED --- //

/**
 * @brief Inicializuje OLED displej (I2C rozhranie a základná konfigurácia).
 *
 * Kroky:
 * - inicializuje TWI/I2C volaním @ref twi_init,
 * - počká cca 100 ms po napájaní,
 * - pošle sériu inicializačných príkazov podľa datasheetu,
 * - zapne displej (0xAF),
 * - vymaže obrazovku volaním @ref oled_clear.
 */
void oled_init(void) {
    twi_init();
    _delay_ms(100);
    oled_send_command(0xAE);
    oled_send_command(0x20); oled_send_command(0x00);
    oled_send_command(0xB0);
    oled_send_command(0xC8);
    oled_send_command(0x00);
    oled_send_command(0x10);
    oled_send_command(0x40);
    oled_send_command(0x81); oled_send_command(0x7F);
    oled_send_command(0xA1);
    oled_send_command(0xA6);
    oled_send_command(0xA8); oled_send_command(0x3F);
    oled_send_command(0xA4);
    oled_send_command(0xD3); oled_send_command(0x00);
    oled_send_command(0xD5); oled_send_command(0xF0);
    oled_send_command(0xD9); oled_send_command(0x22);
    oled_send_command(0xDA); oled_send_command(0x12);
    oled_send_command(0xDB); oled_send_command(0x20);
    oled_send_command(0x8D); oled_send_command(0x14);
    oled_send_command(0xAF);
    oled_clear();
}

/**
 * @brief Vyčistí jednu konkrétnu stránku OLED displeja.
 *
 * Používa sa napríklad pri mazaní spodného riadku s hláškou.
 *
 * @param page Číslo stránky (0–7), ktorá sa má vymazať.
 */
static void oled_clear_page(uint8_t page)
{
    oled_set_pos(page, 0);
    oled_send_data_start();
    for (uint8_t i = 0; i < 132; i++)
        oled_send_data_byte(0x00);
    oled_send_data_stop();
}

/**
 * @brief Zobrazí krátku hlášku o uložení obľúbenej stanice v spodnom riadku.
 *
 * Postup:
 * - prepočíta @p freq_khz na formát MHz (napr. 10700 → 107.0),
 * - pripraví text v tvare `"FAVORITE 107.0MHz"`,
 * - vymaže spodnú stránku (page 7),
 * - vykreslí text s menším horizontálnym odsadením,
 * - nechá správu svietiť ~3 sekundy,
 * - znovu vymaže spodný riadok.
 *
 * @param freq_khz Frekvencia uložená ako obľúbená, v kHz.
 */
void oled_show_favorite_saved_bottom(int freq_khz)
{
    int mhz = freq_khz / 100;
    int dec = (freq_khz % 100) / 10;

    char line[24];
    // opäť – použité len veľké písmená, ktoré máš vo fonte
    snprintf(line, sizeof(line), "FAVORITE %3d.%1dMHz", mhz, dec);

    uint8_t x_offset = 4;

    // vyčisti spodný riadok (page 7 – nikde inde ju nepoužívaš)
    oled_clear_page(7);

    // vypíš hlášku na spodok
    oled_draw_string(7, x_offset, line);

    // nech to svieti cca 3 sekundy
    for (uint8_t i = 0; i < 3; i++) {
        _delay_ms(1000);   // radšej po sekundách, ako jednu veľkú hodnotu
    }

    // po 3 sekundách spodný riadok zmažeme
    oled_clear_page(7);
}


// --- Hlavná obrazovka rádia --- //

/**
 * @brief Zobrazí hlavnú obrazovku FM rádia (frekvencia, hlasitosť, RSSI, mute).
 *
 * Obrazovka obsahuje:
 * - horný riadok – hlavičku:
 *   - „FM Radio is Mute“, ak je @p muted = true,
 *   - „FM Radio“, inak (s predchádzajúcim riadkom vyčisteným),
 * - stred – veľkým fontom frekvencia v tvare „107.0MHz“,
 * - spodný riadok – text „Vol:xx  RSSI:yy“.
 *
 * @param freq_khz Frekvencia v kHz (napr. 10700 → 107.0 MHz).
 * @param volume   Hlasitosť (0–15 alebo podľa tunera).
 * @param rssi     RSSI hodnota, úroveň signálu.
 * @param muted    @c true ak je zvuk aktuálne stlmený, inak @c false.
 */
void oled_show_radio_screen(int freq_khz, int volume, int rssi, bool muted)
{
    int mhz = freq_khz / 100;
    int dec = (freq_khz % 100) / 10;

    char freq_line[16];
    char line3[20];

    snprintf(freq_line, sizeof(freq_line), "%3d.%1dMHz", mhz, dec);
    snprintf(line3, sizeof(line3), "Vol:%2d  RSSI:%2d", volume, rssi);

    uint8_t x_offset = 4;

    // Zobraz hlavičku
    if (muted)
        oled_draw_string(0, x_offset, "FM Radio is Mute");
    else {
        // Vyčisti starú hlavičku (ak predtým bolo mute)
        oled_set_pos(0, 0);
        oled_send_data_start();
        for (uint8_t i = 0; i < 132; i++) oled_send_data_byte(0x00);
        oled_send_data_stop();

        oled_draw_string(0, x_offset, "FM Radio");
    }

    // Frekvencia a info
    oled_draw_string_big(3, x_offset, freq_line);
    oled_draw_string(5, x_offset, line3);
}

/**
 * @brief Zobrazí hlavičku pre stav, že FM rádio je vypnuté (power off).
 *
 * Funkcia:
 * - vyčistí horný riadok (page 0), kde predtým mohol byť text
 *   „FM Radio“ alebo „FM Radio is Mute“,
 * - vykreslí text „FM Radio is power off“ s malým odsadením.
 *
 * Používa sa pri dlhom stlačení dolného tlačidla, keď sa modul rádia
 * vypne volaním @c radio.powerDown().
 */
void oled_show_power_off(void)
{
    uint8_t x_offset = 4;

    // vyčisti horný riadok (page 0), aby tam nezostal „FM Radio“ alebo „FM Radio is Mute“
    oled_set_pos(0, 0);
    oled_send_data_start();
    for (uint8_t i = 0; i < 132; i++) {
        oled_send_data_byte(0x00);
    }
    oled_send_data_stop();

    // zobraz text „FM Radio is power off“ v hornom riadku
    oled_draw_string(0, x_offset, "FM Radio is power off");
}
