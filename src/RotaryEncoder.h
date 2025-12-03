#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h>
#include "gpio.h"

/**
 * @file
 * @brief Rozhranie pre obsluhu rotačného enkódera (otáčanie + tlačidlo).
 */

/**
 * @enum EncoderEvent
 * @brief Typ udalosti z enkódera.
 *
 * Udalosti, ktoré môže enkóder generovať:
 * - EVENT_NONE   – žiadna zmena,
 * - EVENT_CW     – otočenie po smere hodinových ručičiek,
 * - EVENT_CCW    – otočenie proti smeru hodinových ručičiek,
 * - EVENT_BUTTON – stlačenie tlačidla enkódera.
 */
enum EncoderEvent {
    EVENT_NONE,
    EVENT_CW,
    EVENT_CCW,
    EVENT_BUTTON
};

/**
 * @class RotaryEncoder
 * @brief Trieda na obsluhu rotačného enkódera vrátane tlačidla.
 *
 * Trieda RotaryEncoder obsluhuje všetky tri piny enkódera:
 * - CLK (signál A),
 * - DT  (signál B),
 * - SW  (tlačidlo).
 *
 * Vnútorne uchováva ukazovatele na registre DDR a PIN a bitové pozície
 * pre každý z troch pinov, aby bolo možné enkóder pripojiť na ľubovoľné piny.
 */
class RotaryEncoder {
private:
    /// @brief Konfigurácia pinu CLK – smer a čítanie stavu.
    volatile uint8_t *_ddrCLK;
    volatile uint8_t *_pinRegCLK;
    uint8_t _bitCLK;

    /// @brief Konfigurácia pinu DT – smer a čítanie stavu.
    volatile uint8_t *_ddrDT;
    volatile uint8_t *_pinRegDT;
    uint8_t _bitDT;

    /// @brief Konfigurácia pinu SW (tlačidlo enkódera).
    volatile uint8_t *_ddrSW;
    volatile uint8_t *_pinRegSW;
    uint8_t _bitSW;

    /// @brief Posledný stav signálu CLK, používa sa na detekciu otočenia.
    uint8_t _lastCLKState;

    /// @brief Čas posledného stlačenia tlačidla (na debounce).
    unsigned long _lastButtonPress;

    /// @brief Čas poslednej rotačnej udalosti (napr. na obmedzenie rýchlych otočení).
    unsigned long _lastRotaryEvent;

public:
    /**
     * @brief Konštruktor enkódera – uloží ukazovatele na registre a bity pre CLK, DT a SW.
     *
     * @param ddrCLK    Ukazovateľ na DDR register pre pin CLK.
     * @param pinRegCLK Ukazovateľ na PIN register pre pin CLK.
     * @param bitCLK    Bitová pozícia pinu CLK v danom porte.
     * @param ddrDT     Ukazovateľ na DDR register pre pin DT.
     * @param pinRegDT  Ukazovateľ na PIN register pre pin DT.
     * @param bitDT     Bitová pozícia pinu DT v danom porte.
     * @param ddrSW     Ukazovateľ na DDR register pre pin SW (tlačidlo).
     * @param pinRegSW  Ukazovateľ na PIN register pre pin SW.
     * @param bitSW     Bitová pozícia pinu SW v danom porte.
     */
    RotaryEncoder(
        volatile uint8_t *ddrCLK, volatile uint8_t *pinRegCLK, uint8_t bitCLK,
        volatile uint8_t *ddrDT,  volatile uint8_t *pinRegDT,  uint8_t bitDT,
        volatile uint8_t *ddrSW,  volatile uint8_t *pinRegSW,  uint8_t bitSW
    );

    /**
     * @brief Inicializuje piny enkódera ako vstupy s interným pull-up rezistorom.
     *
     * Nastaví všetky tri piny (CLK, DT, SW) do režimu INPUT_PULLUP,
     * aby bolo možné spoľahlivo detegovať otočenie aj stlačenie tlačidla.
     */
    void begin();

    /**
     * @brief Skontroluje aktuálny stav enkódera a vráti prípadnú udalosť.
     *
     * Funkcia:
     * - sleduje zmenu stavu signálu CLK/DT na detekciu smeru otáčania,
     * - sleduje tlačidlo SW vrátane debounce,
     * - vracia jednu z hodnôt @ref EncoderEvent podľa zistenej udalosti.
     *
     * @return Hodnota typu EncoderEvent podľa zistenej udalosti.
     */
    EncoderEvent checkEvent();
};

/**
 * @brief Externá funkcia vracajúca počet milisekúnd od štartu programu.
 *
 * Funkcia je implementovaná v @c main.cpp a používa sa na meranie času
 * (debounce tlačidla, obmedzenie frekvencie čítania otáčania a pod.).
 *
 * @return Počet milisekúnd od spustenia programu.
 */
extern unsigned long timer_millis();

#endif
