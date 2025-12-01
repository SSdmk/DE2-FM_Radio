#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(
    volatile uint8_t *ddrCLK, volatile uint8_t *pinRegCLK, uint8_t bitCLK,
    volatile uint8_t *ddrDT,  volatile uint8_t *pinRegDT,  uint8_t bitDT,
    volatile uint8_t *ddrSW,  volatile uint8_t *pinRegSW,  uint8_t bitSW
) {
    _ddrCLK = ddrCLK;       _pinRegCLK = pinRegCLK;       _bitCLK = bitCLK;
    _ddrDT  = ddrDT;        _pinRegDT  = pinRegDT;        _bitDT  = bitDT;
    _ddrSW  = ddrSW;        _pinRegSW  = pinRegSW;        _bitSW  = bitSW;

    _lastButtonPress = 0;
    _lastRotaryEvent = 0;
}

void RotaryEncoder::begin() {
    // Nastavenie INPUT_PULLUP pomocou tvojej knižnice
    gpio_mode_input_pullup(_ddrCLK, _bitCLK);
    gpio_mode_input_pullup(_ddrDT,  _bitDT);
    gpio_mode_input_pullup(_ddrSW,  _bitSW);
    
    _lastCLKState = gpio_read(_pinRegCLK, _bitCLK);
}

EncoderEvent RotaryEncoder::checkEvent() {
    unsigned long currentTime = timer_millis();
    uint8_t currentCLKState = gpio_read(_pinRegCLK, _bitCLK);
    uint8_t btnReading = gpio_read(_pinRegSW, _bitSW);

    // --- 1. KONTROLA OTÁČANIA ---
    
    if (currentCLKState != _lastCLKState && currentCLKState == 1) {
        if (currentTime - _lastRotaryEvent > 50) { // Debounce 40ms
            if (currentTime - _lastButtonPress > 20) { // Blokovanie pri stlačení
                
                EncoderEvent direction;
                if (gpio_read(_pinRegDT, _bitDT) != currentCLKState) {
                    direction = EVENT_CW;
                } else {
                    direction = EVENT_CCW;
                }
                _lastRotaryEvent = currentTime;
                _lastCLKState = currentCLKState;
                return direction;
            }
        }
    }
   // Always update the last CLK state (prevents false double triggers)


    _lastCLKState = currentCLKState;

    // --- 2. KONTROLA TLAČIDLA ---
    if (btnReading == 0) { // 0 = LOW (stlačené)
        if (currentTime - _lastButtonPress > 20) { // Debounce 200ms
            if (currentTime - _lastRotaryEvent > 20) { // Blokovanie pri točení
                
                _lastButtonPress = currentTime;
                return EVENT_BUTTON;
            }
        }
    }
    return EVENT_NONE;
}