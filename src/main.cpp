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

// ------------------- millis -------------------
volatile unsigned long millis_counter = 0;

ISR(TIMER0_OVF_vect) {
    millis_counter++;
}

unsigned long timer_millis() {
    unsigned long m;
    uint8_t old = SREG;
    cli();
    m = millis_counter;
    SREG = old;
    return m;
}

// ------------------- Buttons -------------------
Button UpButton(&DDRD, &PIND, 5);
Button DownButton(&DDRD, &PIND, 6);
Button LeftButton(&DDRD, &PIND, 7);
Button RightButton(&DDRB, &PINB, 0);

// ------------------- Encoder (SW = A1 / PC1) -------------------
RotaryEncoder encoder(
    &DDRD, &PIND, 2,   // CLK
    &DDRD, &PIND, 3,   // DT
    &DDRC, &PINC, 1    // SW (A1, PC1)
);

// ------------------- Radio -------------------
extern Si4703 radio;

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

        oled_show_radio_screen(freq, vol, rssi, !muted);
    }

    return 0;
}
