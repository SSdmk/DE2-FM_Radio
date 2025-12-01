#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h> 
#include "gpio.h"   // Tvoja GPIO knižnica

enum EncoderEvent {
    EVENT_NONE,
    EVENT_CW,
    EVENT_CCW,
    EVENT_BUTTON
};

class RotaryEncoder {
private:
    // Ukladáme si pointery na registre (napr. &DDRD) a číslo bitu
    volatile uint8_t *_ddrCLK;
    volatile uint8_t *_pinRegCLK;
    uint8_t _bitCLK;

    volatile uint8_t *_ddrDT;
    volatile uint8_t *_pinRegDT;
    uint8_t _bitDT;

    volatile uint8_t *_ddrSW;
    volatile uint8_t *_pinRegSW;
    uint8_t _bitSW;

    uint8_t _lastCLKState;
    
    // Časovače pre debouncing
    unsigned long _lastButtonPress;
    unsigned long _lastRotaryEvent;

public:
    // Konštruktor prijíma registre a bity
    RotaryEncoder(
        volatile uint8_t *ddrCLK, volatile uint8_t *pinRegCLK, uint8_t bitCLK,
        volatile uint8_t *ddrDT,  volatile uint8_t *pinRegDT,  uint8_t bitDT,
        volatile uint8_t *ddrSW,  volatile uint8_t *pinRegSW,  uint8_t bitSW
    );

    void begin();
    EncoderEvent checkEvent();
};

// Deklarácia externej funkcie pre čas (musí byť implementovaná v main.cpp)
extern unsigned long timer_millis(); 

#endif