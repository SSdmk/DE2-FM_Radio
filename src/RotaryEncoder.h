#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h>
#include "gpio.h"

/*
    Enum, ktorý reprezentuje typ udalosti z enkódera:
    - EVENT_NONE  → nič sa nestalo
    - EVENT_CW    → otočené po smere hodinových ručičiek
    - EVENT_CCW   → otočené proti smeru
    - EVENT_BUTTON → stlačenie tlačidla enkódera
*/
enum EncoderEvent {
    EVENT_NONE,
    EVENT_CW,
    EVENT_CCW,
    EVENT_BUTTON
};

/*
    Trieda RotaryEncoder dokáže obsluhovať všetky 3 piny enkódera:
    - CLK (signál A)
    - DT  (signál B)
    - SW  (tlačidlo)

    V triede uchovávame registre DDR, PIN a bitové pozície
    pre každý z troch pinov, aby encoder fungoval na ľubovoľných pinoch.
*/
class RotaryEncoder {
private:
    // CLK pin
    volatile uint8_t *_ddrCLK;
    volatile uint8_t *_pinRegCLK;
    uint8_t _bitCLK;

    // DT pin
    volatile uint8_t *_ddrDT;
    volatile uint8_t *_pinRegDT;
    uint8_t _bitDT;

    // SW tlačidlo
    volatile uint8_t *_ddrSW;
    volatile uint8_t *_pinRegSW;
    uint8_t _bitSW;

    // posledný stav CLK (potrebné na detekciu zmien)
    uint8_t _lastCLKState;

    // čas posledného kliknutia (debounce tlačidla)
    unsigned long _lastButtonPress;

    // používame pri blokovaní otáčania po kliknutí
    unsigned long _lastRotaryEvent;

public:
    // Konštruktor — prijíma adresy registrov a bity pre CLK/DT/SW
    RotaryEncoder(
        volatile uint8_t *ddrCLK, volatile uint8_t *pinRegCLK, uint8_t bitCLK,
        volatile uint8_t *ddrDT,  volatile uint8_t *pinRegDT,  uint8_t bitDT,
        volatile uint8_t *ddrSW,  volatile uint8_t *pinRegSW,  uint8_t bitSW
    );

    // Inicializácia pinov na INPUT_PULLUP
    void begin();

    // Hlavná funkcia, ktorá zisťuje otočenie aj klik
    EncoderEvent checkEvent();
};

// funkcia na získanie millis() – definovaná v main.cpp
extern unsigned long timer_millis();

#endif
