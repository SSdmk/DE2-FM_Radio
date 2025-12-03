#include "RotaryEncoder.h" 
#include "uart.h"

/**
 * @file
 * @brief Implementácia triedy RotaryEncoder na detekciu otáčania a stlačenia tlačidla.
 */

/**
 * @brief Konštruktor enkódera – uloží ukazovatele na registre a bity pinov.
 *
 * Konštruktor si uloží informácie o:
 * - dátových smerových registroch (DDR),
 * - vstupných registroch (PIN),
 * - bitových pozíciách pinov CLK, DT a SW.
 *
 * Vďaka tomu môže enkóder pracovať na ľubovoľných pinoch mikrokontroléra.
 */
RotaryEncoder::RotaryEncoder(
    volatile uint8_t *ddrCLK, volatile uint8_t *pinRegCLK, uint8_t bitCLK,
    volatile uint8_t *ddrDT,  volatile uint8_t *pinRegDT,  uint8_t bitDT,
    volatile uint8_t *ddrSW,  volatile uint8_t *pinRegSW,  uint8_t bitSW
) {
    _ddrCLK = ddrCLK; _pinRegCLK = pinRegCLK; _bitCLK = bitCLK;
    _ddrDT  = ddrDT;  _pinRegDT  = pinRegDT;  _bitDT  = bitDT;
    _ddrSW  = ddrSW;  _pinRegSW  = pinRegSW;  _bitSW  = bitSW;

    _lastCLKState = 0;
    _lastButtonPress = 0;
    _lastRotaryEvent = 0;
}

/**
 * @brief Inicializuje piny enkódera ako vstupy s interným pull-up rezistorom.
 *
 * Nastaví piny:
 * - CLK, DT a SW do režimu INPUT_PULLUP,
 * - uloží počiatočný stav pinu CLK na detekciu neskoršej zmeny.
 *
 * V kľudovom stave sú piny log. 1 (HIGH) a pri akcii (otočenie/stlačenie)
 * padajú na log. 0 (LOW).
 */
void RotaryEncoder::begin() {
    gpio_mode_input_pullup(_ddrCLK, _bitCLK);
    gpio_mode_input_pullup(_ddrDT,  _bitDT);
    gpio_mode_input_pullup(_ddrSW,  _bitSW);

    _lastCLKState = gpio_read(_pinRegCLK, _bitCLK);
}

/**
 * @brief Zistí aktuálnu udalosť z enkódera (otáčanie alebo stlačenie).
 *
 * Funkcia:
 * - deteguje stlačenie tlačidla SW s jednoduchým debounce,
 * - dekóduje otáčanie mechanického enkódera pomocou Grayovho kódu,
 * - na základe prechodovej tabuľky určí smer otáčania,
 * - vracia hodnotu typu ::EncoderEvent.
 *
 * Implementácia je prispôsobená bežným EC11 enkóderom a snaží sa:
 * - minimalizovať falošné kroky,
 * - neprehadzovať smer pri rýchlom otáčaní.
 *
 * @return
 *  - EVENT_BUTTON – detegované stlačenie tlačidla enkódera,
 *  - EVENT_CW     – otočenie v smere hodinových ručičiek,
 *  - EVENT_CCW    – otočenie proti smeru hodinových ručičiek,
 *  - EVENT_NONE   – žiadna nová udalosť.
 */
EncoderEvent RotaryEncoder::checkEvent() {
    unsigned long now = timer_millis();

    // -----------------------------------------------------
    // 1. Detekcia stlačenia tlačidla (SW) s debounce
    // -----------------------------------------------------
    uint8_t sw = gpio_read(_pinRegSW, _bitSW);

    if (sw == 0) {  // tlačidlo je stlačené pri log. 0
        if (now - _lastButtonPress > 250) {   // debounce ~250 ms
            _lastButtonPress = now;

            // Po kliknutí mechanický enkóder často spraví krátky
            // „záškub“, preto blokujeme spracovanie rotácie
            // na krátky čas (150 ms).
            _lastRotaryEvent = now + 150;

            return EVENT_BUTTON;
        }
    }

    // -----------------------------------------------------
    // 2. Detekcia rotácie enkódera pomocou Grayovho kódu
    // -----------------------------------------------------

    // Načítame aktuálny stav signálov CLK a DT
    uint8_t clk = gpio_read(_pinRegCLK, _bitCLK);
    uint8_t dt  = gpio_read(_pinRegDT,  _bitDT);

    // Zloženie dvojbitového stavu: (CLK, DT) → 0..3
    uint8_t rawState = (clk << 1) | dt;

    /*
        Grayov kód používaný enkóderom má nasledujúce poradie:
        - pre CW:  0 → 1 → 3 → 2 → 0
        - pre CCW: 0 → 2 → 3 → 1 → 0
    */
    static const uint8_t GRAY_MAP[4] = {0, 1, 3, 2};
    uint8_t newState = GRAY_MAP[rawState];

    // Posledný stav si pamätáme medzi volaniami funkcie
    static uint8_t lastState = 0;

    /*
        Prechodová tabuľka:
        - vstup: [starý stav][nový stav]
        - výstup: +1 = CW, -1 = CCW, 0 = žiadny platný pohyb
    */
    static const int8_t ROTARY_TABLE[4][4] = {
        // new:   0    1    3    2
        {    0,  +1,   0,  -1 },  // old 0
        {   -1,   0,  +1,   0 },  // old 1
        {    0,  -1,   0,  +1 },  // old 2
        {   +1,   0,  -1,   0 }   // old 3
    };

    // Výsledný pohyb (+1 = CW, -1 = CCW, 0 = nič)
    int8_t movement = ROTARY_TABLE[lastState][newState];

    // Uložíme nový stav pre ďalšie volanie
    lastState = newState;

    // -----------------------------------------------------
    // 3. Mapa výsledku na logiku aplikácie (CW/CCW ↔ +/−)
    // -----------------------------------------------------
    /*
        V tejto aplikácii chceš otočiť logiku:
        - ak dekodér hlási CW (movement = +1), vrátime EVENT_CCW,
        - ak dekodér hlási CCW (movement = -1), vrátime EVENT_CW.
    */

    if (movement == +1) return EVENT_CCW;
    if (movement == -1) return EVENT_CW;

    return EVENT_NONE;
}
