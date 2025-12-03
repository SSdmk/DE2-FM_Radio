#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include "twi.h"
#include "oled.h"

#define OLED_ADDR 0x3C
#define OLED_CMD  0x00
#define OLED_DATA 0x40


// --- Font ---
typedef struct {
    char c;
    uint8_t data[5];
} Font5x7Char;

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


#define FONT_TABLE_SIZE (sizeof(font_table)/sizeof(Font5x7Char))

static const uint8_t* font_get_char(char c) {
    for (uint8_t i = 0; i < FONT_TABLE_SIZE; i++) {
        if (font_table[i].c == c) {
            return font_table[i].data;
        }
    }
    // fallback – medzera
    return font_table[0].data;
}


// --- OLED I2C ---
static void oled_send_command(uint8_t cmd) {
    twi_start();
    twi_write((OLED_ADDR << 1) | 0);
    twi_write(OLED_CMD);
    twi_write(cmd);
    twi_stop();
}
static void oled_send_data_start(void) {
    twi_start();
    twi_write((OLED_ADDR << 1) | 0);
    twi_write(OLED_DATA);
}
static void oled_send_data_byte(uint8_t b) { twi_write(b); }
static void oled_send_data_stop(void) { twi_stop(); }

static void oled_set_pos(uint8_t page, uint8_t col) {
    oled_send_command(0xB0 | (page & 0x07));
    oled_send_command(0x00 | (col & 0x0F));
    oled_send_command(0x10 | ((col >> 4) & 0x0F));
}

// --- Čistenie ---
void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_set_pos(page, 0);
        oled_send_data_start();
        for (uint8_t i = 0; i < 132; i++) oled_send_data_byte(0x00);
        oled_send_data_stop();
    }
}

// --- Textové funkcie ---
void oled_draw_char(uint8_t page, uint8_t *col, char c) {
    const uint8_t *glyph = font_get_char(c);
    oled_set_pos(page, *col);
    oled_send_data_start();
    for (uint8_t i = 0; i < 5; i++) oled_send_data_byte(glyph[i]);
    oled_send_data_byte(0x00);
    oled_send_data_stop();
    *col += 6;
}

void oled_draw_string(uint8_t page, uint8_t col, const char *s) {
    while (*s && col <= 122) oled_draw_char(page, &col, *s++);
}

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

void oled_draw_string_big(uint8_t page, uint8_t col, const char *s) {
    while (*s && col <= 118) oled_draw_char_big(page, &col, *s++);
}

// --- Inicializácia OLED ---
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

static void oled_clear_page(uint8_t page)
{
    oled_set_pos(page, 0);
    oled_send_data_start();
    for (uint8_t i = 0; i < 132; i++)
        oled_send_data_byte(0x00);
    oled_send_data_stop();
}

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

// --- Hlavná obrazovka ---
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
