#ifndef OLED_H
#define OLED_H

#include <stdint.h>

// Inicializácia displeja
void oled_init(void);

// Vymazanie celého displeja
void oled_clear(void);

// Vykreslenie jedného znaku 5x7 na danú page a column (col sa posunie ďalej)
void oled_draw_char(uint8_t page, uint8_t *col, char c);

// Vykreslenie C-stringu na danú page od stĺpca col
void oled_draw_string(uint8_t page, uint8_t col, const char *s);

// Špecifická obrazovka pre rádio: frekvencia, hlasitosť, RSSI
void oled_show_radio_screen(int freq_khz, int volume, int rssi, bool muted);

void oled_show_radio_screen(int freq_khz, int volume, int rssi, bool muted);

void oled_show_favorite_saved_bottom(int freq_khz);


#endif
