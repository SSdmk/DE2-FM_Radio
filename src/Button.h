#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "gpio.h" 

// Typy udalostí tlačidla
enum ButtonEvent {
    BTN_EVENT_NONE,
    BTN_EVENT_SHORT, // Krátke stlačenie (< 2.5s)
    BTN_EVENT_LONG   // Dlhé stlačenie (> 3s)
};

class Button {
private:
    // Hardvér
    volatile uint8_t *_ddr;
    volatile uint8_t *_pinReg;
    uint8_t _bit;

    // Debouncing premenné
    uint8_t _stableState;          
    uint8_t _lastFlickerableState; 
    unsigned long _lastDebounceTime;

    // Premenné pre časovanie stlačenia
    unsigned long _pressStartTime; // Kedy sme tlačidlo stlačili
    bool _isPressed;               // Či je momentálne logicky stlačené
    bool _longPressReported;       // Aby sme LONG event poslali len raz

public:
    Button(volatile uint8_t *ddr, volatile uint8_t *pinReg, uint8_t bit);
    void begin();

    // Hlavná funkcia, ktorá vracia udalosť (SHORT, LONG alebo NONE)
    ButtonEvent checkEvent();
};

extern unsigned long timer_millis(); 

#endif