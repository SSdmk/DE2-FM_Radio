#include "Button.h"

Button::Button(volatile uint8_t *ddr, volatile uint8_t *pinReg, uint8_t bit) {
    _ddr = ddr;
    _pinReg = pinReg;
    _bit = bit;
    
    _stableState = 1;       // 1 = HIGH (nestlačené)
    _lastFlickerableState = 1;
    _lastDebounceTime = 0;
    
    _isPressed = false;
    _longPressReported = false;
    _pressStartTime = 0;
}

void Button::begin() {
    gpio_mode_input_pullup(_ddr, _bit);
    _stableState = gpio_read(_pinReg, _bit);
}

ButtonEvent Button::checkEvent() {
    uint8_t currentState = gpio_read(_pinReg, _bit);
    unsigned long currentTime = timer_millis();

    // --- 1. DEBOUNCING LOGIKA ---
    // Ak sa stav zmenil (kmitanie), resetujeme časovač debouncu
    if (currentState != _lastFlickerableState) {
        _lastDebounceTime = currentTime;
    }
    _lastFlickerableState = currentState;

    // Ak je stav stabilný aspoň 50ms
    if ((currentTime - _lastDebounceTime) > 50) {
        
        // Detekcia zmeny stabilného stavu (Stlačenie alebo Pustenie)
        if (currentState != _stableState) {
            _stableState = currentState;

            // -> Tlačidlo bolo práve STLAČENÉ (Falling edge: 1 -> 0)
            if (_stableState == 0) {
                _pressStartTime = currentTime;
                _isPressed = true;
                _longPressReported = false; // Reset flagu
            }
            
            // -> Tlačidlo bolo práve PUSTENÉ (Rising edge: 0 -> 1)
            else {
                _isPressed = false;

                // Ak sme ešte nenahlásili Long Press, skontrolujeme trvanie
                if (!_longPressReported) {
                    unsigned long duration = currentTime - _pressStartTime;
                    
                    // Podmienka pre KRÁTKE stlačenie (< 2500 ms)
                    if (duration < 2500) {
                        return BTN_EVENT_SHORT;
                    }
                    // Poznámka: Ak je to medzi 2.5s a 3.0s, nevráti sa nič (ignorovalo sa)
                }
            }
        }
    }

    // --- 2. LOGIKA PRE DLHÉ STLAČENIE (počas držania) ---
    if (_isPressed && !_longPressReported) {
        unsigned long duration = currentTime - _pressStartTime;
        
        // Podmienka pre DLHÉ stlačenie (> 3000 ms)
        if (duration > 3000) {
            _longPressReported = true; // Aby sme to poslali len raz
            return BTN_EVENT_LONG;
        }
    }

    return BTN_EVENT_NONE;
}