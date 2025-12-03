#include "Button.h"

/**
 * @brief Konštruktor tlačidla.
 *
 * Uloží adresy registrov a bit konkrétneho pinu a nastaví východzie
 * hodnoty pre debouncing a časovanie stlačenia.
 *
 * @param ddr    Adresa registra smeru portu (DDR), napr. &DDRD
 * @param pinReg Adresa vstupného registra portu (PINx), napr. &PIND
 * @param bit    Číslo pinu v intervale 0 až 7
 */
Button::Button(volatile uint8_t *ddr, volatile uint8_t *pinReg, uint8_t bit) {
    _ddr = ddr;
    _pinReg = pinReg;
    _bit = bit;
    
    _stableState = 1;       // 1 = HIGH (nestlačené, pri použití INPUT_PULLUP)
    _lastFlickerableState = 1;
    _lastDebounceTime = 0;
    
    _isPressed = false;
    _longPressReported = false;
    _pressStartTime = 0;
}

/**
 * @brief Inicializácia pinu tlačidla.
 *
 * Nastaví pin ako vstup s interným pull-up rezistorom a načíta
 * počiatočný stabilný stav vstupu.
 */
void Button::begin() {
    gpio_mode_input_pullup(_ddr, _bit);
    _stableState = gpio_read(_pinReg, _bit);
}

/**
 * @brief Spracovanie stavu tlačidla, debouncing a detekcia udalostí.
 *
 * Funkciu je potrebné pravidelne volať v hlavnej slučke. Interné kroky:
 * - vykoná softvérový debouncing (cca 50 ms),
 * - sleduje dĺžku stlačenia,
 * - pri uvoľnení tlačidla môže vrátiť krátke stlačenie,
 * - pri dlhom držaní vráti dlhé stlačenie (len raz).
 *
 * @return Typ udalosti podľa aktuálneho stavu tlačidla:
 *         - ::BTN_EVENT_SHORT – krátke stlačenie (< 2500 ms),
 *         - ::BTN_EVENT_LONG – dlhé stlačenie (> 3000 ms),
 *         - ::BTN_EVENT_NONE – žiadna nová udalosť.
 */
ButtonEvent Button::checkEvent() {
    uint8_t currentState = gpio_read(_pinReg, _bit);
    unsigned long currentTime = timer_millis();

    // --- 1. DEBOUNCING LOGIKA ---
    // Ak sa stav zmenil (kmitanie kontaktov), resetujeme časovač debouncu
    if (currentState != _lastFlickerableState) {
        _lastDebounceTime = currentTime;
    }
    _lastFlickerableState = currentState;

    // Ak je stav stabilný aspoň 50 ms, môžeme ho považovať za platný
    if ((currentTime - _lastDebounceTime) > 50) {
        
        // Detekcia zmeny stabilného stavu (stlačenie alebo pustenie)
        if (currentState != _stableState) {
            _stableState = currentState;

            // -> Tlačidlo bolo práve STLAČENÉ (Falling edge: 1 -> 0)
            if (_stableState == 0) {
                _pressStartTime = currentTime;
                _isPressed = true;
                _longPressReported = false; // Reset flagu pre ďalšie dlhé stlačenie
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
                    // Poznámka: Ak je to medzi 2.5 s a 3.0 s, nevráti sa nič (ignoruje sa)
                }
            }
        }
    }

    // --- 2. LOGIKA PRE DLHÉ STLAČENIE (počas držania) ---
    if (_isPressed && !_longPressReported) {
        unsigned long duration = currentTime - _pressStartTime;
        
        // Podmienka pre DLHÉ stlačenie (> 3000 ms)
        if (duration > 3000) {
            _longPressReported = true; // Aby sme udalosť poslali len raz
            return BTN_EVENT_LONG;
        }
    }

    // Žiadna nová udalosť
    return BTN_EVENT_NONE;
}
