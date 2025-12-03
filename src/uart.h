#ifndef UART_H
# define UART_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
*  Title:    Interrupt UART library with receive/transmit circular buffers
*  Author:   Peter Fleury <pfleury@gmx.ch>  http://tinyurl.com/peterfleury
*  File:     $Id: uart.h,v 1.13 2015/01/11 13:53:25 peter Exp $
*  Software: AVR-GCC 4.x, AVR Libc 1.4 or higher
*  Hardware: any AVR with built-in UART/USART
*  Usage:    see Doxygen manual
*
*  LICENSE:
*   Copyright (C) 2015 Peter Fleury, GNU General Public License Version 3
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 3 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
************************************************************************/

/**
 * @file uart.h
 * @defgroup pfleury_uart UART knižnica <uart.h>
 * @code #include <uart.h> @endcode
 *
 * @brief Prerušeniami riadená UART knižnica s kruhovými buffermi pre príjem a odosielanie.
 *
 * Táto knižnica poskytuje funkcie na odosielanie a prijímanie dát cez
 * vstavané UART/USART periférie mikrokontrolérov AVR. Na pozadí využíva
 * prerušenia a kruhové buffery pre RX aj TX, takže hlavný program môže
 * pracovať bez čakania na dokončenie prenosu.
 *
 * Pri prijme a odosielaní bajtov sa generujú prerušenia, v ktorých
 * obslužné rutiny zapisujú/čítajú dáta do/z kruhových bufferov.
 *
 * Veľkosť kruhových bufferov je daná konštantami #UART_RX_BUFFER_SIZE
 * a #UART_TX_BUFFER_SIZE, ktoré musia byť mocninou 2. Tieto veľkosti
 * je možné prispôsobiť projektu napríklad pridaním
 * `CDEFS += -DUART_RX_BUFFER_SIZE=nn -DUART_TX_BUFFER_SIZE=nn`
 * do Makefile.
 *
 * @note Implementácia a pôvodná dokumentácia sú prevzaté z knižnice
 *       Petra Fleuryho (pozri hlavičku vyššie). Tento súbor je použitý
 *       ako súčasť projektu a môže obsahovať len minimálne úpravy
 *       konfigurácie alebo komentárov.
 */

#include <avr/pgmspace.h>

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 405
# error "This library requires AVR-GCC 4.5 or later, update to newer AVR-GCC compiler !"
#endif


/**@{*/  /**< Začiatok skupiny funkcií a makier UART knižnice. */


/*
** Konštanty a makrá
*/

/**
 * @brief Výpočet hodnoty registra pre nastavenie baudrate v štandardnom režime.
 *
 * Makro prepočíta požadovanú prenosovú rýchlosť (baudrate) a frekvenciu
 * systémového oscilátora na hodnotu, ktorá sa má zapísať do baudrate registra
 * (UBRR) pri bežnom (16×) vzorkovaní.
 *
 * @param xtalCpu Frekvencia systémového oscilátora v Hz
 *                (napr. 4000000UL pre 4 MHz).
 * @param baudRate Požadovaná prenosová rýchlosť v bps
 *                 (napr. 1200, 2400, 9600).
 */
#define UART_BAUD_SELECT(baudRate, xtalCpu) (((xtalCpu) + 8UL * (baudRate)) / (16UL * (baudRate)) - 1UL)

/**
 * @brief Výpočet hodnoty registra pre nastavenie baudrate v režime dvojnásobnej rýchlosti.
 *
 * Makro prepočíta hodnotu baudrate pre režim „double speed“ (U2X = 1),
 * kde UART používa 8× vzorkovanie. Výsledkom je hodnota pre UBRR
 * s nastaveným najvyšším bitom, ktorý indikuje použitie dvojnásobnej rýchlosti.
 *
 * @param xtalCpu Frekvencia systémového oscilátora v Hz
 *                (napr. 4000000UL pre 4 MHz).
 * @param baudRate Požadovaná prenosová rýchlosť v bps
 *                 (napr. 1200, 2400, 9600).
 */
#define UART_BAUD_SELECT_DOUBLE_SPEED(baudRate, xtalCpu) ( ((((xtalCpu) + 4UL * (baudRate)) / (8UL * (baudRate)) - 1UL)) | 0x8000)

/**
 * @brief Veľkosť kruhového prijímacieho bufferu v bajtoch (mocnina 2).
 *
 * Ak veľkosť nevyhovuje potrebám aplikácie, je možné ju zmeniť
 * pridaním prepínača `-DUART_RX_BUFFER_SIZE=nn` do Makefile (cez CDEFS).
 */
#ifndef UART_RX_BUFFER_SIZE
# define UART_RX_BUFFER_SIZE 64
#endif

/**
 * @brief Veľkosť kruhového vysielacieho bufferu v bajtoch (mocnina 2).
 *
 * Ak veľkosť nevyhovuje potrebám aplikácie, je možné ju zmeniť
 * pridaním prepínača `-DUART_TX_BUFFER_SIZE=nn` do Makefile (cez CDEFS).
 */
#ifndef UART_TX_BUFFER_SIZE
# define UART_TX_BUFFER_SIZE 64
#endif

/* test, či sa súčet veľkostí bufferov vojde do SRAM */
#if ( (UART_RX_BUFFER_SIZE + UART_TX_BUFFER_SIZE) >= (RAMEND - 0x60 ) )
# error "size of UART_RX_BUFFER_SIZE + UART_TX_BUFFER_SIZE larger than size of SRAM"
#endif

/*
** Kódy chýb pre funkciu uart_getc()
*/

/** @brief Príznak rámcovej chyby (framing error) prijímača UART. */
#define UART_FRAME_ERROR     0x1000
/** @brief Príznak pretečenia (overrun) vnútorného UART prijímača. */
#define UART_OVERRUN_ERROR   0x0800
/** @brief Príznak parity chyby prijatého bajtu. */
#define UART_PARITY_ERROR    0x0400
/** @brief Príznak pretečenia (overflow) softvérového prijímacieho kruhového bufferu. */
#define UART_BUFFER_OVERFLOW 0x0200
/** @brief Príznak, že nie sú k dispozícii žiadne prijaté dáta. */
#define UART_NO_DATA         0x0100


/*
** Deklarácie funkcií UART knižnice
*/

/**
 * @brief Inicializuje UART a nastaví prenosovú rýchlosť (baudrate).
 *
 * Funkcia nastaví UART perifériu podľa parametra @p baudrate. Hodnota
 * parametra sa typicky získa pomocou makra #UART_BAUD_SELECT() alebo
 * #UART_BAUD_SELECT_DOUBLE_SPEED().
 *
 * @param baudrate Hodnota pre UBRR register, vypočítaná cez príslušné makro.
 */
extern void uart_init(unsigned int baudrate);


/**
 * @brief Získa prijatý bajt z prijímacieho kruhového bufferu.
 *
 * Funkcia vracia 16-bitovú hodnotu, kde:
 * - v **nižšom bajte** je prijatý znak,
 * - vo **vyššom bajte** je stavový kód poslednej chyby prijímača.
 *
 * Ak nie sú k dispozícii žiadne dáta, je vrátený kód #UART_NO_DATA.
 *
 * @return 16-bitová hodnota:
 *   - nižší bajt: prijatý znak,
 *   - vyšší bajt: stav prijímača:
 *     - 0 – prijatie prebehlo bez chyby,
 *     - #UART_NO_DATA – nie sú k dispozícii žiadne prijaté dáta,
 *     - #UART_BUFFER_OVERFLOW – softvérový prijímací buffer pretečie,
 *     - #UART_OVERRUN_ERROR – hardvérový prijímač UART pretečie (dáta neboli včas prečítané),
 *     - #UART_FRAME_ERROR – rámcová chyba (nesprávny formát alebo dĺžka rámca).
 */
extern unsigned int uart_getc(void);


/**
 * @brief Zapíše jeden bajt do vysielacieho kruhového bufferu UART.
 *
 * Funkcia vloží bajt @p data do TX bufferu. Skutočné odoslanie prebieha
 * na pozadí pomocou prerušení. Ak je buffer plný, funkcia môže blokovať,
 * kým sa neuvoľní miesto.
 *
 * @param data Bajt, ktorý sa má odoslať cez UART.
 */
extern void uart_putc(unsigned char data);


/**
 * @brief Zapíše textový reťazec z RAM do vysielacieho bufferu UART.
 *
 * Funkcia prechádza znakovým reťazcom @p s, ukladá jeho znaky do TX
 * kruhového bufferu a odosielanie prebieha postupne v prerušení.
 * Ak sa celý reťazec nezmestí naraz, funkcia môže blokovať,
 * pokým sa v buffere neuvoľní miesto.
 *
 * @param s Nulou ukončený reťazec v RAM, ktorý sa má odoslať.
 */
extern void uart_puts(const char *s);


/**
 * @brief Zapíše textový reťazec z programovej pamäte (Flash) do vysielacieho bufferu UART.
 *
 * Reťazec je uložený v programovej pamäti (PROGMEM). Funkcia postupne
 * prenáša znaky do TX kruhového bufferu a odosielanie vykonáva UART
 * pomocou prerušení. Ak sa celý reťazec nezmestí naraz, funkcia môže
 * blokovať, pokým sa v buffere neuvoľní miesto.
 *
 * @param s Ukazovateľ na reťazec v programovej pamäti (PROGMEM).
 *
 * @see uart_puts
 */
extern void uart_puts_p(const char *s);

/**
 * @brief Pomocné makro pre odoslanie reťazca z programovej pamäte.
 *
 * Makro automaticky umiestni zadaný textový literál do programovej pamäte
 * a zavolá funkciu #uart_puts_p().
 *
 * @param __s Textový literál (string constant), ktorý sa má odoslať.
 */
#define uart_puts_P(__s) uart_puts_p(PSTR(__s))


/**
 * @brief Inicializuje perifériu USART1 (len na vybraných mikrokontroléroch).
 *
 * Funkcia nastaví druhé UART/USART rozhranie (USART1) podobne ako #uart_init().
 *
 * @param baudrate Hodnota pre UBRR register, vypočítaná cez príslušné makro.
 * @see uart_init
 */
extern void uart1_init(unsigned int baudrate);

/**
 * @brief Získa prijatý bajt z prijímacieho bufferu USART1.
 *
 * Správa sa rovnako ako #uart_getc(), ale pre druhé rozhranie UART (USART1).
 *
 * @return 16-bitová hodnota so znakom v nižšom bajte a stavovým kódom vo vyššom bajte.
 * @see uart_getc
 */
extern unsigned int uart1_getc(void);

/**
 * @brief Zapíše jeden bajt do vysielacieho bufferu USART1.
 *
 * Správa sa rovnako ako #uart_putc(), ale pre druhé rozhranie UART (USART1).
 *
 * @param data Bajt, ktorý sa má odoslať cez USART1.
 * @see uart_putc
 */
extern void uart1_putc(unsigned char data);

/**
 * @brief Zapíše reťazec z RAM do vysielacieho bufferu USART1.
 *
 * Správa sa rovnako ako #uart_puts(), ale pre druhé rozhranie UART (USART1).
 *
 * @param s Nulou ukončený reťazec v RAM, ktorý sa má odoslať cez USART1.
 * @see uart_puts
 */
extern void uart1_puts(const char *s);

/**
 * @brief Zapíše reťazec z programovej pamäte do vysielacieho bufferu USART1.
 *
 * Správa sa rovnako ako #uart_puts_p(), ale pre druhé rozhranie UART (USART1).
 *
 * @param s Ukazovateľ na reťazec v programovej pamäti (PROGMEM).
 * @see uart_puts_p
 */
extern void uart1_puts_p(const char *s);

/**
 * @brief Pomocné makro pre odoslanie reťazca z programovej pamäte cez USART1.
 *
 * @param __s Textový literál (string constant), ktorý sa má odoslať.
 */
#define uart1_puts_P(__s) uart1_puts_p(PSTR(__s))

/**@}*/ /**< Koniec skupiny pfleury_uart. */

#ifdef __cplusplus
}
#endif

#endif // UART_H
