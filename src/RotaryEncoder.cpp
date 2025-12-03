#include "RotaryEncoder.h"
#include "uart.h"

/*
    Konštruktor – uložíme si, ktoré registre a piny používame.
    Kód tak funguje na ľubovoľných pinoch, nie iba na D2/D3.
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

/*
    begin() – nastavíme všetky 3 piny na INPUT_PULLUP.
    Vďaka tomu sú v kľude HIGH a pri otočení alebo kliknutí padajú na LOW.
*/
void RotaryEncoder::begin() {
    gpio_mode_input_pullup(_ddrCLK, _bitCLK);
    gpio_mode_input_pullup(_ddrDT,  _bitDT);
    gpio_mode_input_pullup(_ddrSW,  _bitSW);

    _lastCLKState = gpio_read(_pinRegCLK, _bitCLK);
}


/*
 =====================================================================
  checkEvent() — DETEKCIA TLAČIDLA + GRAYOVO DEKÓDOVANIE ROTÁCIE
 =====================================================================

  Tento dekodér je najstabilnejší pre lacné mechanické EC11 enkódery.
  Grayov kód zaručí, že aj pri rýchlom otáčaní:
  - nebude prehadzovať smer
  - nebudú vznikať falošné CW/CCW
*/
EncoderEvent RotaryEncoder::checkEvent() {
    unsigned long now = timer_millis();

    // =====================================================
    //                 1. DETEKCIA STLAČENIA
    // =====================================================
    uint8_t sw = gpio_read(_pinRegSW, _bitSW);

    if (sw == 0) {  // tlačidlo drží LOW pri stlačení
        if (now - _lastButtonPress > 250) {   // debounce 250 ms
            _lastButtonPress = now;

            // Po kliknutí encoder často urobí malý "pohnutie",
            // preto blokujeme rotáciu na 150 ms.
            _lastRotaryEvent = now + 150;

            return EVENT_BUTTON;
        }
    }

    // =====================================================
    //                 2. DETEKCIA ROTÁCIE
    // =====================================================

    // načítame kombináciu bitov CLK a DT
    uint8_t clk = gpio_read(_pinRegCLK, _bitCLK);
    uint8_t dt  = gpio_read(_pinRegDT,  _bitDT);

    // dvojbitový stav: 00, 01, 11, 10 → 0 až 3
    uint8_t rawState = (clk << 1) | dt;

    /*
        Grayov kód má špecifické poradie zmien:
        0 → 1 → 3 → 2 → 0 (CW)
        0 → 2 → 3 → 1 → 0 (CCW)
    */
    static const uint8_t GRAY_MAP[4] = {0, 1, 3, 2};
    uint8_t newState = GRAY_MAP[rawState];

    /*
        lastState si pamätáme medzi dvoma volaniami checkEvent(),
        aby sme vedeli porovnať predchádzajúci a aktuálny stav.
    */
    static uint8_t lastState = 0;

    /*
        Prechodová tabuľka podľa Grayovho kódu.
        Na základe prechodu medzi dvoma stavmi určíme smer.
    */
    static const int8_t ROTARY_TABLE[4][4] = {
        // new:   0    1    3    2
        {    0,  +1,   0,  -1 },  // old 0
        {   -1,   0,  +1,   0 },  // old 1
        {    0,  -1,   0,  +1 },  // old 2
        {   +1,   0,  -1,   0 }   // old 3
    };

    // výsledný pohyb: +1 = CW, -1 = CCW, 0 = nič
    int8_t movement = ROTARY_TABLE[lastState][newState];

    // uložíme nový stav
    lastState = newState;

    // =====================================================
    //           3. VRÁTENIE SPRÁVNE OTOČENEJ UDALOSTI
    // =====================================================
    /*
        Ty chceš opačné správanie:
        → keď encoder hovorí CW, ty chceš UBERAŤ
        → keď encoder hovorí CCW, ty chceš PRIDÁVAŤ

        Preto výsledok OTOČÍME.
    */

    if (movement == +1) return EVENT_CCW; // otočené
    if (movement == -1) return EVENT_CW;  // otočené

    return EVENT_NONE;
}
