/* 
 * GPIO library for AVR-GCC.
 * (c) 2019-2024 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and AVR 8-bit Toolchain 3.6.2.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

// -- Includes -------------------------------------------------------
#include <gpio.h>


// -- Function definitions -------------------------------------------

/**
 * @brief  Nastaví jeden pin ako výstupný.
 *
 * Funkcia nastaví príslušný bit v registri smeru portu (DDR) na 1,
 * čím sa pin nakonfiguruje ako výstup. Následne je možné používať
 * @ref gpio_write_high a @ref gpio_write_low na zapisovanie úrovní.
 *
 * @param  reg Adresa registra smeru portu (Data Direction Register),
 *             napr. &DDRB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_mode_output(volatile uint8_t *reg, uint8_t pin)
{
    *reg = *reg | (1<<pin);
}


/**
 * @brief  Nastaví jeden pin ako vstupný s povoleným interným pull-upom.
 *
 * Najprv vynuluje príslušný bit v DDR (pin ako vstup),
 * potom posunie ukazovateľ na PORTx a nastaví tam bit na 1,
 * čím povolí interný pull-up rezistor. Pin je tak v kľude v logickej
 * úrovni HIGH.
 *
 * @param  reg Adresa registra smeru portu (Data Direction Register),
 *             napr. &DDRB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_mode_input_pullup(volatile uint8_t *reg, uint8_t pin)
{
    *reg = *reg & ~(1<<pin);  // Data Direction Register (vstup)
    reg++;                    // Posun na výstupný register PORTx
    *reg = *reg | (1<<pin);   // PORTx – zapnutie pull-upu
}


/**
 * @brief  Zapíše na daný pin logickú úroveň LOW.
 *
 * Funkcia v príslušnom PORTx vynuluje bit pinu, čím sa na výstupnom
 * pine objaví logická úroveň 0 (LOW).
 *
 * @param  reg Adresa výstupného registra portu (PORTx),
 *             napr. &PORTB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_write_low(volatile uint8_t *reg, uint8_t pin)
{
    *reg = *reg & ~(1<<pin);
}


/**
 * @brief  Zapíše na daný pin logickú úroveň HIGH.
 *
 * Funkcia v príslušnom PORTx nastaví bit pinu na 1, čím sa na výstupnom
 * pine objaví logická úroveň 1 (HIGH).
 *
 * @param  reg Adresa výstupného registra portu (PORTx),
 *             napr. &PORTB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_write_high(volatile uint8_t *reg, uint8_t pin)
{
    *reg = *reg | (1<<pin);
}


/**
 * @brief  Prečíta logickú hodnotu zo vstupného pinu.
 *
 * Funkcia číta príslušný bit z registra PINx a podľa jeho hodnoty
 * vráti 0 (LOW) alebo 1 (HIGH).
 *
 * @param  reg Adresa vstupného registra portu (PINx),
 *             napr. &PINB
 * @param  pin Číslo pinu v intervale 0 až 7
 * @return 0, ak je pin v logickej úrovni LOW, inak 1
 */
uint8_t gpio_read(volatile uint8_t *reg, uint8_t pin)
{
    uint8_t temp;

    temp = *reg & (1<<pin);

    if (temp != 0) {
        return 1;
    }
    else {
        return 0;
    }
}


/**
 * @brief  Nastaví pin ako vstupný bez interného pull-upu.
 *
 * Funkcia:
 * - vynuluje príslušný bit v DDR (pin ako vstup),
 * - posunie ukazovateľ na PORTx a tam bit vynuluje,
 * takže interný pull-up rezistor je vypnutý.
 *
 * @param  reg Adresa registra smeru portu (Data Direction Register),
 *             napr. &DDRB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_mode_input_nopull(volatile uint8_t *reg, uint8_t pin)
{
    *reg = *reg & ~(1<<pin);  // Data Direction Register – vstup
    reg++;                    // Posun na PORTx
    *reg = *reg & ~(1<<pin);  // PORTx – pull-up vypnutý
}


/**
 * @brief  Preklopí (toggle) logickú hodnotu na danom pine.
 *
 * Funkcia invertuje bit v PORTx: ak bol HIGH, nastaví LOW, a naopak.
 * Vhodné napríklad na jednoduché blikanie LED alebo generovanie
 * občasných impulzov.
 *
 * @param  reg Adresa výstupného registra portu (PORTx),
 *             napr. &PORTB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_toggle(volatile uint8_t *reg, uint8_t pin)
{
    *reg = *reg ^ (1<<pin);
}
