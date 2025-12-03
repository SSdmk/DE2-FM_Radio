#ifndef TIMER_H
# define TIMER_H


/* 
 * Timer library for AVR-GCC.
 * (c) 2019-2024 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and AVR 8-bit Toolchain 3.6.2.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

/**
 * @file 
 * @defgroup fryza_timer Knižnica časovačov <timer.h>
 * @code #include <timer.h> @endcode
 *
 * @brief Knižnica makier na ovládanie časovačov pre AVR-GCC.
 *
 * Knižnica obsahuje sadu makier pre konfiguráciu časovačov/čítačov
 * mikrokontroléra (Timer/Counter0, 1 a 2). Umožňuje jednoducho:
 *  - zastaviť časovač,
 *  - nastaviť preddeľovač (prescaler) tak, aby došlo k pretečeniu
 *    v zvolenom intervale,
 *  - povoliť alebo zakázať prerušenie pri pretečení (overflow).
 *
 * Makrá sú navrhnuté pre ATmega328P s F_CPU = 16 MHz. Pri zmene
 * frekvencie CPU sa menia aj reálne časy pretečenia.
 *
 * @note Knižnica je čisto hlavičková – neexistuje žiadny .c súbor,
 *       obsahuje iba makrá nad registrami časovačov.
 * @copyright (c) 2019-2024 Tomas Fryza, MIT license
 * @{
 */

// -- Includes -------------------------------------------------------
#include <avr/io.h>


// -- Defines --------------------------------------------------------
/**
 * @name  Makrá pre 16-bitový Timer/Counter1
 * @note  Čas pretečenia: \f$ t_{OVF} = \frac{1}{F_{CPU}} \cdot \text{prescaler} \cdot 2^{16} \f$,
 *        kde F_CPU = 16 MHz.
 * @{
 */

/**
 * @brief Zastaví Timer1 (prescaler = 0, časovač stojí).
 */
#define tim1_stop() TCCR1B &= ~((1<<CS12) | (1<<CS11) | (1<<CS10));

/**
 * @brief Nastaví Timer1 tak, aby pretečenie nastalo približne každé 4 ms.
 *
 * Zodpovedá nastaveniu prescaler = 1 (bity CS12..CS10 = 001).
 */
#define tim1_ovf_4ms() TCCR1B &= ~((1<<CS12) | (1<<CS11)); TCCR1B |= (1<<CS10);

/**
 * @brief Nastaví Timer1 tak, aby pretečenie nastalo približne každých 33 ms.
 *
 * Zodpovedá nastaveniu prescaler = 8 (bity CS12..CS10 = 010).
 */
#define tim1_ovf_33ms() TCCR1B &= ~((1<<CS12) | (1<<CS10)); TCCR1B |= (1<<CS11);

/**
 * @brief Nastaví Timer1 tak, aby pretečenie nastalo približne každých 262 ms.
 *
 * Zodpovedá nastaveniu prescaler = 64 (bity CS12..CS10 = 011).
 */
#define tim1_ovf_262ms() TCCR1B &= ~(1<<CS12); TCCR1B |= (1<<CS11) | (1<<CS10);

/**
 * @brief Nastaví Timer1 tak, aby pretečenie nastalo približne každú 1 sekundu.
 *
 * Zodpovedá nastaveniu prescaler = 256 (bity CS12..CS10 = 100).
 */
#define tim1_ovf_1sec() TCCR1B &= ~((1<<CS11) | (1<<CS10)); TCCR1B |= (1<<CS12);

/**
 * @brief Nastaví Timer1 tak, aby pretečenie nastalo približne každé 4 sekundy.
 *
 * Zodpovedá nastaveniu prescaler = 1024 (bity CS12..CS10 = 101).
 */
#define tim1_ovf_4sec() TCCR1B &= ~(1<<CS11); TCCR1B |= (1<<CS12) | (1<<CS10);

/**
 * @brief Povolenie prerušenia pri pretečení Timer1.
 *
 * Nastaví bit TOIE1 v registri TIMSK1 na 1.
 */
#define tim1_ovf_enable() TIMSK1 |= (1<<TOIE1);

/**
 * @brief Zakázanie prerušenia pri pretečení Timer1.
 *
 * Vynuluje bit TOIE1 v registri TIMSK1.
 */
#define tim1_ovf_disable() TIMSK1 &= ~(1<<TOIE1);
/** @} */


/**
 * @name  Makrá pre 8-bitový Timer/Counter0
 * @note  Čas pretečenia: \f$ t_{OVF} = \frac{1}{F_{CPU}} \cdot \text{prescaler} \cdot 2^{8} \f$,
 *        kde F_CPU = 16 MHz.
 * @{
 */

/**
 * @brief Zastaví Timer0 (prescaler = 0, časovač stojí).
 */
#define tim0_stop() TCCR0B &= ~((1<<CS02) | (1<<CS01) | (1<<CS00));

/**
 * @brief Nastaví Timer0 tak, aby pretečenie nastalo približne každých 16 µs.
 *
 * Zodpovedá nastaveniu prescaler = 1 (bity CS02..CS00 = 001).
 */
#define tim0_ovf_16us() TCCR0B &= ~((1<<CS02) | (1<<CS01)); TCCR0B |= (1<<CS00);

/**
 * @brief Nastaví Timer0 tak, aby pretečenie nastalo približne každých 128 µs.
 *
 * Zodpovedá nastaveniu prescaler = 8 (bity CS02..CS00 = 010).
 */
#define tim0_ovf_128us() TCCR0B &= ~((1<<CS02) | (1<<CS00)); TCCR0B |= (1<<CS01);

/**
 * @brief Nastaví Timer0 tak, aby pretečenie nastalo približne každých 1 ms.
 *
 * Zodpovedá nastaveniu prescaler = 64 (bity CS02..CS00 = 011).
 */
#define tim0_ovf_1ms() TCCR0B &= ~(1<<CS02); TCCR0B |= (1<<CS01) | (1<<CS00);

/**
 * @brief Nastaví Timer0 tak, aby pretečenie nastalo približne každé 4 ms.
 *
 * Zodpovedá nastaveniu prescaler = 256 (bity CS02..CS00 = 100).
 */
#define tim0_ovf_4ms() TCCR0B &= ~((1<<CS01) | (1<<CS00)); TCCR0B |= (1<<CS02);

/**
 * @brief Nastaví Timer0 tak, aby pretečenie nastalo približne každých 16 ms.
 *
 * Zodpovedá nastaveniu prescaler = 1024 (bity CS02..CS00 = 101).
 */
#define tim0_ovf_16ms() TCCR0B &= ~(1<<CS01); TCCR0B |= (1<<CS02) | (1<<CS00);

/**
 * @brief Povolenie prerušenia pri pretečení Timer0.
 *
 * Nastaví bit TOIE0 v registri TIMSK0 na 1.
 */
#define tim0_ovf_enable() TIMSK0 |= (1<<TOIE0);

/**
 * @brief Zakázanie prerušenia pri pretečení Timer0.
 *
 * Vynuluje bit TOIE0 v registri TIMSK0.
 */
#define tim0_ovf_disable() TIMSK0 &= ~(1<<TOIE0);
/** @} */


/**
 * @name  Makrá pre 8-bitový Timer/Counter2
 * @note  Čas pretečenia: \f$ t_{OVF} = \frac{1}{F_{CPU}} \cdot \text{prescaler} \cdot 2^{8} \f$,
 *        kde F_CPU = 16 MHz.
 * @{
 */

/**
 * @brief Zastaví Timer2 (prescaler = 0, časovač stojí).
 */
#define tim2_stop() TCCR2B &= ~((1<<CS22) | (1<<CS21) | (1<<CS20));

/**
 * @brief Nastaví Timer2 tak, aby pretečenie nastalo približne každých 16 ms.
 *
 * Zodpovedá nastaveniu prescaler = 1024 (bity CS22..CS20 = 111).
 */
#define tim2_ovf_16ms() TCCR2B |= (1<<CS22) | (1<<CS21) | (1<<CS20);

/**
 * @brief Povolenie prerušenia pri pretečení Timer2.
 *
 * Nastaví bit TOIE2 v registri TIMSK2 na 1.
 */
#define tim2_ovf_enable() TIMSK2 |= (1<<TOIE2);

/**
 * @brief Zakázanie prerušenia pri pretečení Timer2.
 *
 * Vynuluje bit TOIE2 v registri TIMSK2.
 */
#define tim2_ovf_disable() TIMSK2 &= ~(1<<TOIE2);
/** @} */


/** @} */  /* koniec skupiny fryza_timer */

#endif
