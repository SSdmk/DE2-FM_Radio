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

// --- Millis implementácia ---
volatile unsigned long millis_counter = 0;
ISR(TIMER0_OVF_vect) { millis_counter++; }
unsigned long timer_millis() {
    unsigned long m;
    uint8_t oldSREG = SREG;
    cli();
    m = millis_counter;
    SREG = oldSREG;
    return m;
}

// Inicializácia tlačidla na pine D5 (prispôsob si podľa potreby)
// DDRD, PIND, bit 5
Button UpButton(&DDRD, &PIND, 5);
Button DownButton(&DDRD, &PIND, 6);
Button LeftButton(&DDRD, &PIND, 7);
Button RightButton(&DDRB, &PINB, 0);


// --- 2. Nastavenie Enkódera ---
// CLK -> PD2, DT -> PD3, SW -> PD4
RotaryEncoder encoder(
    &DDRD, &PIND, 2, 
    &DDRD, &PIND, 3, 
    &DDRB, &PINB, 1  
);


// =======================
// Rádio Si4703 – globálny objekt
// =======================
extern Si4703 radio;



int main(void) {
    // 1. Init UART
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    oled_init();
    // 2. Init Timer
    tim0_ovf_1ms();
    tim0_ovf_enable();

    // 3. Init Button
    UpButton.begin();
    DownButton.begin();
    LeftButton.begin();
    RightButton.begin();
    LeftButton.begin();


    sei(); // Povol prerušenia


    radio.start();            // inicializácia Si4703 (volá aj twi_init vo vnútri)
    radio.setChannel(10700);  // napr. 107.0 MHz
    radio.setVolume(10);      // hlasitosť 0–15
    
    //uart_puts("Test Short/Long Press Start...\r\n");

    while (1) {
        // Kontrola udalostí
        
        ButtonEvent Upevent = UpButton.checkEvent();

        if (Upevent == BTN_EVENT_SHORT) 
        {
           radio_ui_handle_event(UI_BTN_UP_SHORT);
            
        } 
        else if (Upevent == BTN_EVENT_LONG) 
        {
            radio_ui_handle_event(UI_BTN_UP_LONG);
            // Tu vykonaj akciu pre dlhé stlačenie
        }
        

                // Kontrola udalostí
        ButtonEvent Downevent = DownButton.checkEvent();

        if (Downevent == BTN_EVENT_SHORT) 
        {
            radio_ui_handle_event(UI_BTN_DOWN_SHORT);

            
            
            // Tu vykonaj akciu pre krátke stlačenie
        } 
        else if (Downevent == BTN_EVENT_LONG) 
        {
            radio_ui_handle_event(UI_BTN_DOWN_LONG);
            // Tu vykonaj akciu pre dlhé stlačenie
        }



                // Kontrola udalostí
        ButtonEvent Leftevent = LeftButton.checkEvent();

        if (Leftevent == BTN_EVENT_SHORT) 
        {
            radio_ui_handle_event(UI_BTN_LEFT);
            // Tu vykonaj akciu pre krátke stlačenie
        } 


                // Kontrola udalostí
        ButtonEvent Rightevent = RightButton.checkEvent();

        if (Rightevent == BTN_EVENT_SHORT) 
        {
            radio_ui_handle_event(UI_BTN_RIGHT);
            // Tu vykonaj akciu pre krátke stlačenie
        } 
        
        
                // Kontrola udalosti
        EncoderEvent event = encoder.checkEvent();
        switch (event) 
        {
                case EVENT_CCW:
                    radio_ui_handle_event(UI_ENC_STEP_CW); // Vpravo
                    uart_puts("R\r\n");
                    break;

                case EVENT_CW:
                    radio_ui_handle_event(UI_ENC_STEP_CCW); // Vlevo
                    uart_puts("L\r\n");
                    break;

                case EVENT_BUTTON:
                    radio_ui_handle_event(UI_ENC_CLICK); // Tlačidlo
                    uart_puts("B\r\n");
                    break;

                default:
                    // nic
                    break;
        }
         /*
        if (event == EVENT_CCW) 
        {
            radio_ui_handle_event(UI_ENC_STEP_CW); // Vpravo + nový riadok
            uart_puts("R\r\n");
        } 
        if (event == EVENT_CW) 
        {
            radio_ui_handle_event(UI_ENC_STEP_CCW); // Vľavo + nový riadok
            uart_puts("L\r\n"); // Vľavo + nový riadok
        } 
        if (event == EVENT_BUTTON) 
        {
            radio_ui_handle_event(UI_ENC_CLICK); // Tlačidlo + nový riadok
            uart_puts("B\r\n"); // Tlačidlo + nový riadok
        }*/   
        int freq  = radio.getChannel();  // kHz
        int rssi  = radio.getRSSI();
        int vol   = radio.getVolume();
        bool muted = radio.getMute();
        // OLED – zobraz všetko vrátane MUTE
        oled_show_radio_screen(freq, vol, rssi, !muted);
        
    
    
    }

    return 0;
}