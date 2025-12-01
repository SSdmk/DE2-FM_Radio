#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "twi.h"
#include "oled.h"

// =======================
// OLED SSD1306 (I2C, 128x64)
// =======================
#define OLED_ADDR 0x3C   // bežná adresa SSD1306
#define OLED_CMD  0x00
#define OLED_DATA 0x40

// Jednoduchý 5x7 font len pre znaky, ktoré používame
typedef struct {
    char c;
    uint8_t data[5];
} Font5x7Char;

static const Font5x7Char font_table[] = {
    { ' ', {0x00,0x00,0x00,0x00,0x00} },
    { '.', {0x00,0x00,0x00,0x18,0x18} },
    { ':', {0x00,0x18,0x18,0x00,0x18} },

    // 0–9
    { '0', {0x3E,0x51,0x49,0x45,0x3E} },
    { '1', {0x00,0x42,0x7F,0x40,0x00} },
    { '2', {0x42,0x61,0x51,0x49,0x46} },
    { '3', {0x21,0x41,0x45,0x4B,0x31} },
    { '4', {0x18,0x14,0x12,0x7F,0x10} },
    { '5', {0x27,0x45,0x45,0x45,0x39} },
    { '6', {0x3C,0x4A,0x49,0x49,0x30} },
    { '7', {0x01,0x71,0x09,0x05,0x03} },
    { '8', {0x36,0x49,0x49,0x49,0x36} },
    { '9', {0x06,0x49,0x49,0x29,0x1E} },

    // písmená pre texty: "FM RADIO", "VOL", "R", "MHz"
    { 'A', {0x7E,0x09,0x09,0x09,0x7E} },
    { 'D', {0x7F,0x41,0x41,0x22,0x1C} },
    { 'F', {0x7F,0x09,0x09,0x09,0x01} },
    { 'H', {0x7F,0x08,0x08,0x08,0x7F} },
    { 'I', {0x00,0x41,0x7F,0x41,0x00} },
    { 'L', {0x7F,0x40,0x40,0x40,0x40} },
    { 'M', {0x7F,0x02,0x0C,0x02,0x7F} },
    { 'O', {0x3E,0x41,0x41,0x41,0x3E} },
    { 'R', {0x7F,0x09,0x19,0x29,0x46} },
    { 'V', {0x07,0x38,0x40,0x38,0x07} },

    // malé písmená pre "MHz" (h,z), keby si chcela:
    { 'h', {0x7F,0x08,0x08,0x08,0x70} },
    { 'z', {0x44,0x64,0x54,0x4C,0x44} },
};

#define FONT_TABLE_SIZE (sizeof(font_table) / sizeof(Font5x7Char))

static const uint8_t* font_get_char(char c)
{
    for (uint8_t i = 0; i < FONT_TABLE_SIZE; i++) {
        if (font_table[i].c == c) {
            return font_table[i].data;
        }
    }
    return font_table[0].data; // medzera, keď nepoznáme znak
}

// -----------------------
// OLED low-level cez twi.h
// -----------------------
static void oled_send_command(uint8_t cmd)
{
    twi_start();
    twi_write((OLED_ADDR << 1) | TWI_WRITE);
    twi_write(OLED_CMD);
    twi_write(cmd);
    twi_stop();
}

static void oled_send_data_start(void)
{
    twi_start();
    twi_write((OLED_ADDR << 1) | TWI_WRITE);
    twi_write(OLED_DATA);
}

static void oled_send_data_byte(uint8_t data)
{
    twi_write(data);
}

static void oled_send_data_stop(void)
{
    twi_stop();
}

static void oled_set_pos(uint8_t page, uint8_t col)
{
    oled_send_command(0xB0 | (page & 0x07));         // page 0–7
    oled_send_command(0x00 | (col & 0x0F));          // lower column
    oled_send_command(0x10 | ((col >> 4) & 0x0F));   // higher column
}

// -----------------------
// Verejné funkcie
// -----------------------
void oled_clear(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        oled_set_pos(page, 0);
        oled_send_data_start();
        for (uint8_t col = 0; col < 128; col++) {
            oled_send_data_byte(0x00);
        }
        oled_send_data_stop();
    }
}

// OLED text – 5x7
void oled_draw_char(uint8_t page, uint8_t *col, char c)
{
    const uint8_t *glyph = font_get_char(c);

    oled_set_pos(page, *col);
    oled_send_data_start();
    for (uint8_t i = 0; i < 5; i++) {
        oled_send_data_byte(glyph[i]);
    }
    oled_send_data_byte(0x00);  // medzera medzi znakmi
    oled_send_data_stop();

    *col += 6; // posun kurzora
}

void oled_draw_string(uint8_t page, uint8_t col, const char *s)
{
    while (*s && col <= 122) {
        oled_draw_char(page, &col, *s++);
    }
}

// Inicializácia SSD1306
void oled_init(void)
{
    // TWI sa volá aj v Si4703::start(), ale viac krát to nevadí
    twi_init();
    _delay_ms(100);

    oled_send_command(0xAE); // display off
    oled_send_command(0x20); oled_send_command(0x00); // horizontal addressing
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
    oled_send_command(0xAF); // display on

    oled_clear();
}

// Obrazovka rádia na OLED
void oled_show_radio_screen(int freq_khz, int volume, int rssi, bool muted)
{
    // freq_khz je v kHz (napr. 10700 == 107.0 MHz)
    int mhz  = freq_khz / 100;
    int dec  = (freq_khz % 100) / 10;   // jedno desatinné miesto

    char line1[20];
    char line2[16];
    char line3[16];

    // Horný riadok
   // snprintf(line1, sizeof(line1), "FM RADIO");
    // Horný riadok – keď je mute, zobraz "FM RADIO M"
    if (muted) {
        snprintf(line1, sizeof(line1), "FM RADIO Muted");
    } else {
        snprintf(line1, sizeof(line1), "FM RADIO");
    }

    // Frekvencia: "107.0 MHz"
    snprintf(line2, sizeof(line2), "%3d.%1d MHz", mhz, dec);

    // Volume a RSSI: "VOL:10 R:15"
    if (volume < 0)  volume = 0;
    if (volume > 15) volume = 15;
    if (rssi   < 0)  rssi   = 0;
    if (rssi   > 99) rssi   = 99;
    snprintf(line3, sizeof(line3), "VOL:%2d R:%2d", volume, rssi);

    oled_clear();
    oled_draw_string(0, 3, line1);  // page 0
    oled_draw_string(2, 3, line2);  // page 2
    oled_draw_string(4, 3, line3);  // page 4
}
