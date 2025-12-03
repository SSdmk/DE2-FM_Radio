#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "timer.h"
#include "gpio.h"
#include "Button.h"
#include "RotaryEncoder.h"
#include "button_function.h"
#include "oled.h"
#include "Si4703.h"

extern "C" {
    #include "uart.h"
}

/**
 * @file main.cpp
 * @brief Hlavný program FM rádia s enkóderom, tlačidlami a OLED displejom.
 *
 * Aplikácia beží na ATmega328P pri 16 MHz a využíva:
 * - Timer0 overflow interrupt na generovanie milisekundového čítača (@ref timer_millis),
 * - knižnicu @ref uart.h na debug výpisy cez UART,
 * - triedu @ref Button na obsluhu štyroch tlačidiel,
 * - triedu @ref RotaryEncoder na čítanie rotačného enkódera (otáčanie + klik),
 * - triedu @ref Si4703 na ovládanie FM tunera,
 * - funkcie z @ref oled.h na vykresľovanie UI rádia na OLED displeji.
 */

// ------------------- millis -------------------

/**
 * @brief Globálny čítač milisekúnd, inkrementovaný v prerušení TIMER0_OVF.
 *
 * Každé pretečenie 8-bitového časovača 0 (pri nastavenom preskaleri)
 * zodpovedá ~1 ms. Premenná sa používa v @ref timer_millis a v debounce
 * logike enkódera a tlačidiel.
 */
volatile unsigned long millis_counter = 0;

/**
 * @brief Obsluha prerušení overflowu časovača 0.
 *
 * ISR je vyvolaná pri každom pretečení Timer/Counter0
 * (TIMER0_OVF_vect) a inkrementuje @ref millis_counter.
 */
ISR(TIMER0_OVF_vect) {
    millis_counter++;
}

/**
 * @brief Bezpečne vráti aktuálnu hodnotu milisekundového čítača.
 *
 * Funkcia:
 * - dočasne zakáže prerušenia (CLI),
 * - prečíta @ref millis_counter do lokálnej premennej,
 * - obnoví stav registra SREG (povolenie prerušenia),
 * - vráti prečítanú hodnotu.
 *
 * @return Aktuálny čas v milisekundách od štartu programu.
 */
unsigned long timer_millis() {
    unsigned long m;
    uint8_t old = SREG;
    cli();
    m = millis_counter;
    SREG = old;
    return m;
}

// ------------------- Buttons -------------------

/**
 * @brief Tlačidlo „UP“ na porte D, pin 5.
 */
Button UpButton(&DDRD, &PIND, 5);
/**
 * @brief Tlačidlo „DOWN“ na porte D, pin 6.
 */
Button DownButton(&DDRD, &PIND, 6);
/**
 * @brief Tlačidlo „LEFT“ na porte D, pin 7.
 */
Button LeftButton(&DDRD, &PIND, 7);
/**
 * @brief Tlačidlo „RIGHT“ na porte B, pin 0.
 */
Button RightButton(&DDRB, &PINB, 0);

// ------------------- Encoder (SW = A1 / PC1) -------------------

/**
 * @brief Globálna inštancia rotačného enkódera.
 *
 * Pripojenie:
 * - CLK: PD2,
 * - DT : PD3,
 * - SW : PC1 (A1).
 */
RotaryEncoder encoder(
    &DDRD, &PIND, 2,   // CLK
    &DDRD, &PIND, 3,   // DT
    &DDRC, &PINC, 1    // SW (A1, PC1)
);

// ------------------- Radio -------------------

/**
 * @brief Globálna inštancia FM tunera Si4703 (definovaná v Si4703.cpp).
 */
extern Si4703 radio;

/**
 * @brief Hlavná funkcia programu.
 *
 * Postup:
 * - inicializácia UART @ref uart_init pre debug (9600 baud),
 * - inicializácia OLED displeja @ref oled_init,
 * - nastavenie časovača 0 na overflow každú 1 ms a povolenie prerušení,
 * - inicializácia tlačidiel @ref Button::begin,
 * - inicializácia enkódera @ref RotaryEncoder::begin,
 * - globálne povolenie prerušení @ref sei,
 * - inicializácia a nastavenie tunera Si4703:
 *   - @ref Si4703::start,
 *   - @ref Si4703::setChannel na 107.00 MHz,
 *   - @ref Si4703::setVolume na hodnotu 10,
 *   - @ref Si4703::powerDown a následne @ref Si4703::powerUp,
 * - nekonečná slučka:
 *   - čítanie udalostí z tlačidiel a enkódera,
 *   - mapovanie na UI udalosti cez @ref radio_ui_handle_event,
 *   - debug výpis smeru enkódera cez UART,
 *   - čítanie aktuálneho stavu rádia (frekvencia, RSSI, hlasitosť, mute),
 *   - prekreslenie hlavnej obrazovky rádia na OLED
 *     pomocou @ref oled_show_radio_screen.
 *
 * @return V praxi nikdy nevracia, formálne 0.
 */
int main(void)
{
    // UART
    uart_init(UART_BAUD_SELECT(9600, F_CPU));

    // OLED
    oled_init();

    // Timer
    tim0_ovf_1ms();
    tim0_ovf_enable();

    // Buttons init
    UpButton.begin();
    DownButton.begin();
    LeftButton.begin();
    RightButton.begin();

    // Encoder init
    encoder.begin();

    sei();

    // RADIO INIT
    radio.start();
    radio.setChannel(10700);
    radio.setVolume(10);
    radio.powerDown();
    radio.powerUp();

    while (1)
    {
        // ---------------- Button UP ----------------
        ButtonEvent upEv = UpButton.checkEvent();
        if (upEv == BTN_EVENT_SHORT) radio_ui_handle_event(UI_BTN_UP_SHORT);
        else if (upEv == BTN_EVENT_LONG) radio_ui_handle_event(UI_BTN_UP_LONG);

        // ---------------- Button DOWN ----------------
        ButtonEvent dnEv = DownButton.checkEvent();
        if (dnEv == BTN_EVENT_SHORT) radio_ui_handle_event(UI_BTN_DOWN_SHORT);
        else if (dnEv == BTN_EVENT_LONG) radio_ui_handle_event(UI_BTN_DOWN_LONG);

        // ---------------- Button LEFT ----------------
        if (LeftButton.checkEvent() == BTN_EVENT_SHORT)
            radio_ui_handle_event(UI_BTN_LEFT);

        // ---------------- Button RIGHT ----------------
        if (RightButton.checkEvent() == BTN_EVENT_SHORT)
            radio_ui_handle_event(UI_BTN_RIGHT);

        // ---------------- Encoder ----------------
        EncoderEvent ev = encoder.checkEvent();
        
        // volanie eventov pre enkoder
        switch (ev)
        {
            case EVENT_CW:
                radio_ui_handle_event(UI_ENC_STEP_CW);
                uart_puts("CW\r\n");
                break;

            case EVENT_CCW:
                radio_ui_handle_event(UI_ENC_STEP_CCW);
                uart_puts("CCW\r\n");
                break;

            case EVENT_BUTTON:
                radio_ui_handle_event(UI_ENC_CLICK);
                uart_puts("CLICK\r\n");
                break;

            default:
                break;
        }

        // ---------------- OLED UPDATE ----------------
                
        int freq  = radio.getChannel();
        int rssi  = radio.getRSSI();
        int vol   = radio.getVolume();
        bool muted = radio.getMute();

        // ak je rádiový modul vypnutý, zobrazíme „power off“ hlášku
        if (!radio_ui_is_on()) {
            oled_show_power_off();
        } else {
            // inak štandardná obrazovka (mute alebo normál)
            oled_show_radio_screen(freq, vol, rssi, !muted);
        }

    }

    return 0;
}
