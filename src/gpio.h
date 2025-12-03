#ifndef GPIO_H
# define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * GPIO library for AVR-GCC.
 * (c) 2019-2024 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and AVR 8-bit Toolchain 3.6.2.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

/**
 * @file 
 * @defgroup fryza_gpio GPIO Library <gpio.h>
 * @code #include <gpio.h> @endcode
 *
 * @brief GPIO knižnica pre AVR-GCC.
 *
 * Knižnica obsahuje funkcie na ovládanie GPIO pinov mikrokontrolérov AVR
 * (nastavenie smeru pinu, zapnutie interného pull-upu, zápis logickej
 * hodnoty na výstup a čítanie vstupu).
 *
 * @note Založené na AVR Libc Reference Manual; implementácia je pripravená
 *       pre 8-bitové AVR (napr. ATmega328P).
 * @copyright (c) 2019-2024 Tomas Fryza, MIT license
 * @{
 */

// -- Includes -------------------------------------------------------
#include <avr/io.h>


// -- Function prototypes --------------------------------------------

/**
 * @brief  Nastaví jeden pin ako výstupný.
 *
 * Smer pinu sa nastaví na výstup (DDR bit = 1), takže je možné naň
 * zapisovať logické úrovne pomocou @ref gpio_write_high a
 * @ref gpio_write_low.
 *
 * @param  reg Adresa registra smeru portu (Data Direction Register),
 *             napr. &DDRB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_mode_output(volatile uint8_t *reg, uint8_t pin);


/**
 * @brief  Nastaví jeden pin ako vstupný s povoleným interným pull-up rezistorom.
 *
 * Funkcia:
 * - nastaví daný pin do vstupného režimu (DDR bit = 0),
 * - súčasne povolí interný pull-up (PORT bit = 1),
 * takže pin je v kľude v logickej úrovni HIGH.
 *
 * @param  reg Adresa registra smeru portu (Data Direction Register),
 *             napr. &DDRB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_mode_input_pullup(volatile uint8_t *reg, uint8_t pin);


/**
 * @brief  Zapíše na daný pin logickú úroveň LOW.
 *
 * Funkcia predpokladá, že pin je nastavený ako výstup.
 *
 * @param  reg Adresa výstupného registra portu (PORTx),
 *             napr. &PORTB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_write_low(volatile uint8_t *reg, uint8_t pin);


/**
 * @brief  Zapíše na daný pin logickú úroveň HIGH.
 *
 * Funkcia predpokladá, že pin je nastavený ako výstup.
 *
 * @param  reg Adresa výstupného registra portu (PORTx),
 *             napr. &PORTB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_write_high(volatile uint8_t *reg, uint8_t pin);


/**
 * @brief  Prečíta logickú hodnotu zo vstupného pinu.
 *
 * Funkcia číta stav z registra PINx a vráti 0 alebo nenulovú hodnotu
 * podľa aktuálnej úrovne na pine.
 *
 * @param  reg Adresa vstupného registra portu (PINx),
 *             napr. &PIND
 * @param  pin Číslo pinu v intervale 0 až 7
 * @return 0, ak je pin v logickej úrovni LOW, inak nenulová hodnota
 */
uint8_t gpio_read(volatile uint8_t *reg, uint8_t pin);


/**
 * @brief  Nastaví pin ako vstupný bez interného pull-upu.
 *
 * Pin je nastavený do vstupného režimu (DDR bit = 0) a príslušný bit
 * v PORTx je vynulovaný, takže sa interný pull-up nepoužíva.
 *
 * @param  reg Adresa registra smeru portu (Data Direction Register),
 *             napr. &DDRB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_mode_input_nopull(volatile uint8_t *reg, uint8_t pin);


/**
 * @brief  Preklopí (toggle) logickú hodnotu na danom pine.
 *
 * Funkcia invertuje aktuálny stav pinu na výstupe – ak bol HIGH,
 * nastaví LOW a naopak. Vhodné pre jednoduché blikanie LED a pod.
 *
 * @param  reg Adresa výstupného registra portu (PORTx),
 *             napr. &PORTB
 * @param  pin Číslo pinu v intervale 0 až 7
 */
void gpio_toggle(volatile uint8_t *reg, uint8_t pin);


/** @} */

#ifdef __cplusplus
}
#endif
#endif
