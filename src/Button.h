#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "gpio.h" 

/**
 * @file
 * @brief Trieda pre obsluhu jedného tlačidla s debouncingom a detekciou krátkeho/dlhého stlačenia.
 *
 * Tlačidlo je pripojené na zvolený pin mikrokontroléra a táto trieda:
 * - realizuje softvérový debouncing (odfiltrovanie kmitania kontaktov),
 * - rozlišuje krátke a dlhé stlačenie,
 * - vracia udalosti typu ::ButtonEvent, ktoré ďalej spracúva UI logika rádia.
 */

/**
 * @brief Typy udalostí tlačidla.
 *
 * Udalosť vracaná metódou Button::checkEvent().
 */
enum ButtonEvent {
    BTN_EVENT_NONE,   /**< Žiadna nová udalosť (tlačidlo je buď v kľude, alebo už spracované). */
    BTN_EVENT_SHORT,  /**< Krátke stlačenie tlačidla (kratšie než definovaný časový prah). */
    BTN_EVENT_LONG    /**< Dlhé stlačenie tlačidla (dlhšie než definovaný časový prah). */
};

/**
 * @brief Trieda na obsluhu jedného tlačidla s debouncingom a detekciou dĺžky stlačenia.
 *
 * Trieda zapuzdruje:
 * - informácie o konkrétnom pine (DDR, PIN register, bit),
 * - vnútorné premenné pre debouncing (stabilný stav, posledný nestabilný stav, čas poslednej zmeny),
 * - časovanie stlačenia (začiatok stlačenia, príznak dlhého stlačenia).
 *
 * Metóda Button::checkEvent() sa volá periodicky v hlavnej slučke a podľa
 * aktuálneho stavu vráti jednu z udalostí ::ButtonEvent.
 */
class Button {
private:
    /** @brief Adresa registra smeru portu (DDR) príslušného pinu tlačidla. */
    volatile uint8_t *_ddr;
    /** @brief Adresa vstupného registra portu (PINx) pre čítanie stavu tlačidla. */
    volatile uint8_t *_pinReg;
    /** @brief Číslo bitu pinu v rámci portu (0–7). */
    uint8_t _bit;

    /**
     * @brief Posledný stabilný logický stav tlačidla po debouncingu.
     *
     * Typicky 0 = stlačené (pri použití INPUT_PULLUP) a 1 = uvoľnené.
     */
    uint8_t _stableState;

    /**
     * @brief Posledný "surový" stav, ktorý sa môže ešte meniť (pred potvrdením).
     *
     * Slúži na detekciu zmeny, ktorá sa potom časovo overuje (debounce).
     */
    uint8_t _lastFlickerableState;

    /**
     * @brief Čas poslednej zmeny nestabilného stavu v milisekundách.
     *
     * Používa sa pri debouncingu na určenie, či stav zostal nezmenený
     * dostatočne dlho a môže sa považovať za stabilný.
     */
    unsigned long _lastDebounceTime;

    /**
     * @brief Čas, kedy bolo tlačidlo potvrdene stlačené (v ms).
     *
     * Od tohto momentu sa počíta dĺžka stlačenia pre rozlíšenie
     * krátkeho a dlhého stlačenia.
     */
    unsigned long _pressStartTime;

    /**
     * @brief Logická informácia, či je tlačidlo v aktuálne spracovanej chvíli stlačené.
     *
     * Používa sa na detekciu prechodov (stlačenie → uvoľnenie) a na
     * časovanie dĺžky stlačenia.
     */
    bool _isPressed;

    /**
     * @brief Príznak, že udalosť dlhého stlačenia už bola nahlásená.
     *
     * Zabezpečí, že počas jedného dlhého stlačenia sa ::BTN_EVENT_LONG
     * vygeneruje len raz a nie opakovane v každom cykle.
     */
    bool _longPressReported;

public:
    /**
     * @brief Konštruktor tlačidla.
     *
     * Uloží ukazovatele na registre DDR a PIN, ako aj číslo bitu pinu.
     * Samotná inicializácia smeru pinu sa vykonáva v Button::begin().
     *
     * @param ddr    Adresa registra smeru portu (DDR), napr. &DDRD
     * @param pinReg Adresa vstupného registra portu (PINx), napr. &PIND
     * @param bit    Číslo pinu v intervale 0 až 7
     */
    Button(volatile uint8_t *ddr, volatile uint8_t *pinReg, uint8_t bit);

    /**
     * @brief Inicializácia pinu tlačidla.
     *
     * Nastaví pin ako vstup s interným pull-up rezistorom pomocou
     * @ref gpio_mode_input_pullup. Zároveň inicializuje vnútorné
     * premenné pre debouncing a časovanie stlačení.
     */
    void begin();

    /**
     * @brief Hlavná funkcia na spracovanie stavu tlačidla.
     *
     * Funkcia:
     * - vykoná debouncing vstupného signálu,
     * - sleduje dĺžku stlačenia,
     * - podľa dĺžky stlačenia vracia:
     *   - ::BTN_EVENT_SHORT – krátke stlačenie,
     *   - ::BTN_EVENT_LONG – dlhé stlačenie,
     *   - ::BTN_EVENT_NONE – žiadna nová udalosť.
     *
     * Funkciu je potrebné volať pravidelne v hlavnej nekonečnej slučke.
     *
     * @return Udalosť typu ::ButtonEvent podľa aktuálneho stavu tlačidla.
     */
    ButtonEvent checkEvent();
};

/**
 * @brief Globálna funkcia poskytujúca aktuálny čas v milisekundách.
 *
 * Implementácia je v @c main.cpp (počítanie cez Timer0 ISR).
 * Trieda Button túto funkciu používa na debouncing a meranie dĺžky stlačenia.
 *
 * @return Počet milisekúnd od štartu systému.
 */
extern unsigned long timer_millis(); 

#endif
