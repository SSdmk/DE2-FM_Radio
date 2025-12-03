/*************************************************************************
*  Title:    Interrupt UART library with receive/transmit circular buffers
*  Author:   Peter Fleury <pfleury@gmx.ch>   http://tinyurl.com/peterfleury
*  File:     $Id: uart.c,v 1.15.2.4 2015/09/05 18:33:32 peter Exp $
*  Software: AVR-GCC 4.x
*  Hardware: any AVR with built-in UART,
*  License:  GNU General Public License
*
*  DESCRIPTION:
*   An interrupt is generated when the UART has finished transmitting or
*   receiving a byte. The interrupt handling routines use circular buffers
*   for buffering received and transmitted data.
*
*   The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE variables define
*   the buffer size in bytes. Note that these variables must be a
*   power of 2.
*
*  USAGE:
*   Refere to the header file uart.h for a description of the routines.
*   See also example test_uart.c.
*
*  NOTES:
*   Based on Atmel Application Note AVR306
*
*  LICENSE:
*   Copyright (C) 2015 Peter Fleury, GNU General Public License Version 3
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*************************************************************************/

/**
 * @file uart.c
 * @brief Implementácia prerušeniami riadenej UART knižnice s kruhovými buffermi.
 *
 * Tento súbor obsahuje implementáciu funkcií deklarovaných v @ref uart.h.
 * Prijímanie aj odosielanie dát prebieha cez prerušenia a údaje sa ukladajú
 * do kruhových bufferov v SRAM. Hlavný program tak nemusí čakať na dokončenie
 * prenosu – vystačí si s čítaním/zápisom z/do bufferov.
 *
 * Veľkosť prijímacieho a vysielacieho bufferu je určená konštantami
 * #UART_RX_BUFFER_SIZE a #UART_TX_BUFFER_SIZE, ktoré musia byť mocninou 2.
 *
 * Implementácia vychádza z aplikačnej poznámky Atmel AVR306 a knižnice
 * Petra Fleuryho. V projekte je tento súbor použitý bez zásahu do logiky
 * kódu, iba s doplnenými/slovenskými komentármi.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "uart.h"


/*
 *  Konštanty a makrá pre prácu s kruhovými buffermi
 */

/* masky pre RX/TX buffre (predpokladá sa veľkosť ako mocnina 2) */
#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1)

/* kontrola, či je veľkosť RX/TX bufferov skutočne mocnina 2 */
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
# error RX buffer size is not a power of 2
#endif
#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
# error TX buffer size is not a power of 2
#endif


/* 
 * Makrá mapujúce registre a príznaky podľa konkrétneho typu AVR.
 * Jednotlivé vetvy #if/#elif zabezpečujú, aby knižnica fungovala
 * na rôznych rodinách mikrokontrolérov s odlišnými názvami registrov.
 */

#if defined(__AVR_AT90S2313__) || defined(__AVR_AT90S4414__) || defined(__AVR_AT90S8515__) || \
    defined(__AVR_AT90S4434__) || defined(__AVR_AT90S8535__) || \
    defined(__AVR_ATmega103__)
/* staršie AVR alebo ATmega103 s jedným UARTom */
# define UART0_RECEIVE_INTERRUPT  UART_RX_vect
# define UART0_TRANSMIT_INTERRUPT UART_UDRE_vect
# define UART0_STATUS             USR
# define UART0_CONTROL            UCR
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRR
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
#elif defined(__AVR_AT90S2333__) || defined(__AVR_AT90S4433__)
/* staršie AVR classic s jedným UARTom */
# define UART0_RECEIVE_INTERRUPT  UART_RX_vect
# define UART0_TRANSMIT_INTERRUPT UART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRR
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
#elif defined(__AVR_AT90PWM216__) || defined(__AVR_AT90PWM316__)
/* AT90PWM216/316 s jedným USARTom */
# define UART0_RECEIVE_INTERRUPT  USART_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_CONTROLC           UCSRC
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRRL
# define UART0_UBRRH              UBRRH
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
# define UART0_BIT_UCSZ0          UCSZ0
# define UART0_BIT_UCSZ1          UCSZ1
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega8A__) || \
    defined(__AVR_ATmega16__) || defined(__AVR_ATmega16A__) || \
    defined(__AVR_ATmega32__) || defined(__AVR_ATmega32A__) || \
    defined(__AVR_ATmega323__)
/* ATmega s jedným USARTom */
# define UART0_RECEIVE_INTERRUPT  USART_RXC_vect
# define UART0_TRANSMIT_INTERRUPT USART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_CONTROLC           UCSRC
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRRL
# define UART0_UBRRH              UBRRH
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
# define UART0_BIT_UCSZ0          UCSZ0
# define UART0_BIT_UCSZ1          UCSZ1
# define UART0_BIT_URSEL          URSEL
#elif defined(__AVR_ATmega8515__) || defined(__AVR_ATmega8535__)
/* ATmega s jedným USARTom */
# define UART0_RECEIVE_INTERRUPT  USART_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_CONTROLC           UCSRC
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRRL
# define UART0_UBRRH              UBRRH
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
# define UART0_BIT_UCSZ0          UCSZ0
# define UART0_BIT_UCSZ1          UCSZ1
# define UART0_BIT_URSEL          URSEL
#elif defined(__AVR_ATmega163__)
/* ATmega163 s jedným UARTom */
# define UART0_RECEIVE_INTERRUPT  UART_RX_vect
# define UART0_TRANSMIT_INTERRUPT UART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRR
# define UART0_UBRRH              UBRRHI
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
#elif defined(__AVR_ATmega162__)
/* ATmega s dvoma USART rozhraniami */
# define ATMEGA_USART1
# define UART0_RECEIVE_INTERRUPT  USART0_RXC_vect
# define UART1_RECEIVE_INTERRUPT  USART1_RXC_vect
# define UART0_TRANSMIT_INTERRUPT USART0_UDRE_vect
# define UART1_TRANSMIT_INTERRUPT USART1_UDRE_vect
# define UART0_STATUS             UCSR0A
# define UART0_CONTROL            UCSR0B
# define UART0_CONTROLC           UCSR0C
# define UART0_DATA               UDR0
# define UART0_UDRIE              UDRIE0
# define UART0_UBRRL              UBRR0L
# define UART0_UBRRH              UBRR0H
# define UART0_BIT_URSEL          URSEL0
# define UART0_BIT_U2X            U2X0
# define UART0_BIT_RXCIE          RXCIE0
# define UART0_BIT_RXEN           RXEN0
# define UART0_BIT_TXEN           TXEN0
# define UART0_BIT_UCSZ0          UCSZ00
# define UART0_BIT_UCSZ1          UCSZ01
# define UART1_STATUS             UCSR1A
# define UART1_CONTROL            UCSR1B
# define UART1_CONTROLC           UCSR1C
# define UART1_DATA               UDR1
# define UART1_UDRIE              UDRIE1
# define UART1_UBRRL              UBRR1L
# define UART1_UBRRH              UBRR1H
# define UART1_BIT_URSEL          URSEL1
# define UART1_BIT_U2X            U2X1
# define UART1_BIT_RXCIE          RXCIE1
# define UART1_BIT_RXEN           RXEN1
# define UART1_BIT_TXEN           TXEN1
# define UART1_BIT_UCSZ0          UCSZ10
# define UART1_BIT_UCSZ1          UCSZ11
#elif defined(__AVR_ATmega161__)
/* ATmega s UART – aktuálne nepodporované touto knižnicou */
# error "AVR ATmega161 currently not supported by this libaray !"
#elif defined(__AVR_ATmega169__)
/* ATmega s jedným USARTom */
# define UART0_RECEIVE_INTERRUPT  USART0_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART0_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_CONTROLC           UCSRC
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRRL
# define UART0_UBRRH              UBRRH
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
# define UART0_BIT_UCSZ0          UCSZ0
# define UART0_BIT_UCSZ1          UCSZ1
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48A__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega48PA__) || defined(__AVR_ATmega48PB__) || \
    defined(__AVR_ATmega88__) || defined(__AVR_ATmega88A__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega88PA__) || defined(__AVR_ATmega88PB__) || \
    defined(__AVR_ATmega168__) || defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168PA__) || defined(__AVR_ATmega168PB__) || \
    defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega3250__) || defined(__AVR_ATmega3290__) || defined(__AVR_ATmega6450__) || defined(__AVR_ATmega6490__)
/* ATmega s jedným USARTom (napr. ATmega328P) */
# define UART0_RECEIVE_INTERRUPT  USART_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART_UDRE_vect
# define UART0_STATUS             UCSR0A
# define UART0_CONTROL            UCSR0B
# define UART0_CONTROLC           UCSR0C
# define UART0_DATA               UDR0
# define UART0_UDRIE              UDRIE0
# define UART0_UBRRL              UBRR0L
# define UART0_UBRRH              UBRR0H
# define UART0_BIT_U2X            U2X0
# define UART0_BIT_RXCIE          RXCIE0
# define UART0_BIT_RXEN           RXEN0
# define UART0_BIT_TXEN           TXEN0
# define UART0_BIT_UCSZ0          UCSZ00
# define UART0_BIT_UCSZ1          UCSZ01
#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__)
/* ATtiny s jedným USARTom */
# define UART0_RECEIVE_INTERRUPT  USART_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_CONTROLC           UCSRC
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRRL
# define UART0_UBRRH              UBRRH
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
# define UART0_BIT_UCSZ0          UCSZ0
# define UART0_BIT_UCSZ1          UCSZ1
#elif defined(__AVR_ATmega329__) || defined(__AVR_ATmega649__) || defined(__AVR_ATmega3290__) || defined(__AVR_ATmega6490__) || \
    defined(__AVR_ATmega169A__) || defined(__AVR_ATmega169PA__) || \
    defined(__AVR_ATmega329A__) || defined(__AVR_ATmega329PA__) || defined(__AVR_ATmega3290A__) || defined(__AVR_ATmega3290PA__) || \
    defined(__AVR_ATmega649A__) || defined(__AVR_ATmega649P__) || defined(__AVR_ATmega6490A__) || defined(__AVR_ATmega6490P__) || \
    defined(__AVR_ATmega165__) || defined(__AVR_ATmega325__) || defined(__AVR_ATmega645__) || defined(__AVR_ATmega3250__) || defined(__AVR_ATmega6450__) || \
    defined(__AVR_ATmega165A__) || defined(__AVR_ATmega165PA__) || \
    defined(__AVR_ATmega325A__) || defined(__AVR_ATmega325PA__) || defined(__AVR_ATmega3250A__) || defined(__AVR_ATmega3250PA__) || \
    defined(__AVR_ATmega645A__) || defined(__AVR_ATmega645PA__) || defined(__AVR_ATmega6450A__) || defined(__AVR_ATmega6450PA__) || \
    defined(__AVR_ATmega644__)
/* ATmega s jedným USARTom */
# define UART0_RECEIVE_INTERRUPT  USART0_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART0_UDRE_vect
# define UART0_STATUS             UCSR0A
# define UART0_CONTROL            UCSR0B
# define UART0_CONTROLC           UCSR0C
# define UART0_DATA               UDR0
# define UART0_UDRIE              UDRIE0
# define UART0_UBRRL              UBRR0L
# define UART0_UBRRH              UBRR0H
# define UART0_BIT_U2X            U2X0
# define UART0_BIT_RXCIE          RXCIE0
# define UART0_BIT_RXEN           RXEN0
# define UART0_BIT_TXEN           TXEN0
# define UART0_BIT_UCSZ0          UCSZ00
# define UART0_BIT_UCSZ1          UCSZ01
#elif defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__) || \
    defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || \
    defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644P__) ||  \
    defined(__AVR_ATmega164A__) || defined(__AVR_ATmega164PA__) || defined(__AVR_ATmega324A__) || defined(__AVR_ATmega324PA__) || \
    defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || \
    defined(__AVR_ATtiny1634__)
/* ATmega s dvoma USART rozhraniami */
# define ATMEGA_USART1
# define UART0_RECEIVE_INTERRUPT  USART0_RX_vect
# define UART1_RECEIVE_INTERRUPT  USART1_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART0_UDRE_vect
# define UART1_TRANSMIT_INTERRUPT USART1_UDRE_vect
# define UART0_STATUS             UCSR0A
# define UART0_CONTROL            UCSR0B
# define UART0_CONTROLC           UCSR0C
# define UART0_DATA               UDR0
# define UART0_UDRIE              UDRIE0
# define UART0_UBRRL              UBRR0L
# define UART0_UBRRH              UBRR0H
# define UART0_BIT_U2X            U2X0
# define UART0_BIT_RXCIE          RXCIE0
# define UART0_BIT_RXEN           RXEN0
# define UART0_BIT_TXEN           TXEN0
# define UART0_BIT_UCSZ0          UCSZ00
# define UART0_BIT_UCSZ1          UCSZ01
# define UART1_STATUS             UCSR1A
# define UART1_CONTROL            UCSR1B
# define UART1_CONTROLC           UCSR1C
# define UART1_DATA               UDR1
# define UART1_UDRIE              UDRIE1
# define UART1_UBRRL              UBRR1L
# define UART1_UBRRH              UBRR1H
# define UART1_BIT_U2X            U2X1
# define UART1_BIT_RXCIE          RXCIE1
# define UART1_BIT_RXEN           RXEN1
# define UART1_BIT_TXEN           TXEN1
# define UART1_BIT_UCSZ0          UCSZ10
# define UART1_BIT_UCSZ1          UCSZ11
#elif defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega32U2__) || \
    defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__) || \
    defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__) || \
    defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1287__)
/* AVR USB série s USARTom mapovaným na UART1 registre */
# define UART0_RECEIVE_INTERRUPT  USART1_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART1_UDRE_vect
# define UART0_STATUS             UCSR1A
# define UART0_CONTROL            UCSR1B
# define UART0_CONTROLC           UCSR1C
# define UART0_DATA               UDR1
# define UART0_UDRIE              UDRIE1
# define UART0_UBRRL              UBRR1L
# define UART0_UBRRH              UBRR1H
# define UART0_BIT_U2X            U2X1
# define UART0_BIT_RXCIE          RXCIE1
# define UART0_BIT_RXEN           RXEN1
# define UART0_BIT_TXEN           TXEN1
# define UART0_BIT_UCSZ0          UCSZ10
# define UART0_BIT_UCSZ1          UCSZ11
#else
/* žiadna podporovaná definícia UART pre daný MCU */
# error "no UART definition for MCU available"
#endif


/*
 *  Modulové globálne premenne
 *
 *  Kruhové buffre (RX/TX) a indexy hláv/chvostov sú definované
 *  ako volatile, keďže sú používané aj v ISR (prerušeniach).
 */
static volatile unsigned char UART_TxBuf[UART_TX_BUFFER_SIZE];
static volatile unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile unsigned char UART_TxHead;
static volatile unsigned char UART_TxTail;
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;
static volatile unsigned char UART_LastRxError;

#if defined( ATMEGA_USART1 )
static volatile unsigned char UART1_TxBuf[UART_TX_BUFFER_SIZE];
static volatile unsigned char UART1_RxBuf[UART_RX_BUFFER_SIZE];
static volatile unsigned char UART1_TxHead;
static volatile unsigned char UART1_TxTail;
static volatile unsigned char UART1_RxHead;
static volatile unsigned char UART1_RxTail;
static volatile unsigned char UART1_LastRxError;
#endif


/**
 * @brief Prerušovacia rutina: UART0 prijal znak (Receive Complete).
 *
 * ISR sa vyvolá pri prijatí bajtu na rozhraní UART0. Rutina:
 *  - prečíta stavový register a prijaté dáta,
 *  - extrahuje príznaky chýb (frame, overrun, parity),
 *  - uloží prijatý bajt do kruhového prijímacieho bufferu,
 *  - pri pretečení buffera nastaví chybový príznak.
 */
ISR(UART0_RECEIVE_INTERRUPT)
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError = 0;

    /* prečítanie stavového registra UART a registra s prijatými dátami */
    usr  = UART0_STATUS;
    data = UART0_DATA;

    /* zistenie chýb prijímača: Frame Error, Data OverRun, Parity Error */
    #if defined(FE) && defined(DOR) && defined(UPE)
    lastRxError = usr & (_BV(FE) | _BV(DOR) | _BV(UPE) );
    #elif defined(FE0) && defined(DOR0) && defined(UPE0)
    lastRxError = usr & (_BV(FE0) | _BV(DOR0) | _BV(UPE0) );
    #elif defined(FE1) && defined(DOR1) && defined(UPE1)
    lastRxError = usr & (_BV(FE1) | _BV(DOR1) | _BV(UPE1) );
    #elif defined(FE) && defined(DOR)
    lastRxError = usr & (_BV(FE) | _BV(DOR) );
    #endif

    /* výpočet ďalšieho indexu v kruhovom prijímacom buffere */
    tmphead = ( UART_RxHead + 1) & UART_RX_BUFFER_MASK;

    if (tmphead == UART_RxTail)
    {
        /* chyba: prijímací buffer je plný – pretečenie buffera */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        /* uloženie nového indexu hlavy prijímacieho buffera */
        UART_RxHead = tmphead;
        /* uloženie prijatého bajtu do buffera */
        UART_RxBuf[tmphead] = data;
    }
    /* akumulácia chybového stavu, ktorý si neskôr prečíta uart_getc() */
    UART_LastRxError |= lastRxError;
}


/**
 * @brief Prerušovacia rutina: UART0 pripravený odoslať ďalší bajt (Data Register Empty).
 *
 * ISR sa vyvolá, keď je vysielací register UART0 prázdny a je možné
 * zapísať ďalší bajt. Rutina:
 *  - skontroluje, či sú v TX buffere ešte dáta,
 *  - ak áno, odčíta ďalší bajt a odošle ho,
 *  - ak nie, zakáže prerušovanie UDRE (aby ISR nebežalo zbytočne).
 */
ISR(UART0_TRANSMIT_INTERRUPT)
{
    unsigned char tmptail;

    if (UART_TxHead != UART_TxTail)
    {
        /* výpočet nového indexu chvosta TX buffera */
        tmptail     = (UART_TxTail + 1) & UART_TX_BUFFER_MASK;
        UART_TxTail = tmptail;
        /* načítanie bajtu z buffera a zápis do dátového registra UART (spustenie prenosu) */
        UART0_DATA = UART_TxBuf[tmptail]; /* začiatok odosielania */
    }
    else
    {
        /* TX buffer je prázdny – zakážeme UDRE prerušenie, kým nepribudnú nové dáta */
        UART0_CONTROL &= ~_BV(UART0_UDRIE);
    }
}


/**
 * @brief Inicializuje UART0 a nastaví prenosovú rýchlosť.
 *
 * Funkcia nastaví interné indexy kruhových bufferov, konfiguruje rýchlosť
 * podľa parametra @p baudrate (vypočítaného pomocou makra #UART_BAUD_SELECT()
 * alebo #UART_BAUD_SELECT_DOUBLE_SPEED()) a povolí prijímač, vysielač
 * a prerušenie pri dokončení prijmu.
 *
 * @param baudrate Hodnota pre UBRR register, získaná cez výpočtové makro.
 */
void uart_init(unsigned int baudrate)
{
    /* inicializácia indexov TX/RX bufferov */
    UART_TxHead = 0;
    UART_TxTail = 0;
    UART_RxHead = 0;
    UART_RxTail = 0;

    #ifdef UART_TEST
    # ifndef UART0_BIT_U2X
    #  warning "UART0_BIT_U2X not defined"
    # endif
    # ifndef UART0_UBRRH
    #  warning "UART0_UBRRH not defined"
    # endif
    # ifndef UART0_CONTROLC
    #  warning "UART0_CONTROLC not defined"
    # endif
    # if defined(URSEL) || defined(URSEL0)
    #  ifndef UART0_BIT_URSEL
    #   warning "UART0_BIT_URSEL not defined"
    #  endif
    # endif
    #endif /* ifdef UART_TEST */

    /* Nastavenie baud rate – možný režim 2× rýchlosť (U2X) podľa najvyššieho bitu */
    if (baudrate & 0x8000)
    {
        #if UART0_BIT_U2X
        UART0_STATUS = (1 << UART0_BIT_U2X); // povoliť 2× rýchlosť UART
        #endif
    }
    #if defined(UART0_UBRRH)
    UART0_UBRRH = (unsigned char) ((baudrate >> 8) & 0x80);
    #endif
    UART0_UBRRL = (unsigned char) (baudrate & 0x00FF);

    /* Povolíme prijímač, vysielač a prerušenie pri dokončení prijmu (RX Complete) */
    UART0_CONTROL = _BV(UART0_BIT_RXCIE) | (1 << UART0_BIT_RXEN) | (1 << UART0_BIT_TXEN);

    /* Nastavenie formátu rámca: asynchrónny mód, 8 dátových bitov, bez parity, 1 stop bit */
    #ifdef UART0_CONTROLC
    # ifdef UART0_BIT_URSEL
    UART0_CONTROLC = (1 << UART0_BIT_URSEL) | (1 << UART0_BIT_UCSZ1) | (1 << UART0_BIT_UCSZ0);
    # else
    UART0_CONTROLC = (1 << UART0_BIT_UCSZ1) | (1 << UART0_BIT_UCSZ0);
    # endif
    #endif
}/* uart_init */


/**
 * @brief Získa bajt z prijímacieho kruhového bufferu UART0.
 *
 * Ak sú dáta k dispozícii, vráti sa 16-bitová hodnota, kde:
 *  - nižší bajt obsahuje prijatý znak,
 *  - vyšší bajt obsahuje stav prijímača (chybové príznaky).
 *
 * Ak nie sú k dispozícii žiadne dáta, vráti sa kód #UART_NO_DATA.
 *
 * @return 16-bitová hodnota:
 *   - nižší bajt: prijatý znak,
 *   - vyšší bajt: chybový kód alebo 0, ak prijatie prebehlo bez chyby.
 */
unsigned int uart_getc(void)
{
    unsigned char tmptail;
    unsigned char data;
    unsigned char lastRxError;

    /* ak sú indexy hlavy a chvosta rovnaké, buffer je prázdny */
    if (UART_RxHead == UART_RxTail)
    {
        return UART_NO_DATA; /* žiadne dáta nie sú k dispozícii */
    }

    /* výpočet nového indexu chvosta prijímacieho buffera */
    tmptail = (UART_RxTail + 1) & UART_RX_BUFFER_MASK;

    /* načítanie dát z prijímacieho buffera */
    data        = UART_RxBuf[tmptail];
    lastRxError = UART_LastRxError;

    /* uloženie nového indexu chvosta */
    UART_RxTail = tmptail;

    /* po prečítaní poslednej chyby ju vynulujeme */
    UART_LastRxError = 0;

    /* kombinácia chybového kódu (vyšší bajt) a prijatého znaku (nižší bajt) */
    return (lastRxError << 8) + data;
}/* uart_getc */


/**
 * @brief Zapíše bajt do vysielacieho kruhového bufferu UART0.
 *
 * Funkcia čaká, kým je vo vysielacom buffere voľné miesto, následne
 * uloží bajt @p data a povolí prerušenie UDRE, ktoré spustí odosielanie.
 *
 * @param data Bajt, ktorý sa má odoslať cez UART0.
 */
void uart_putc(unsigned char data)
{
    unsigned char tmphead;

    /* výpočet nového indexu hlavy vysielacieho buffera */
    tmphead = (UART_TxHead + 1) & UART_TX_BUFFER_MASK;

    /* čakanie, kým sa v TX buffere neuvoľní miesto */
    while (tmphead == UART_TxTail)
    {
        ;/* čakaj na voľné miesto v buffere */
    }

    /* uloženie dát do buffera a aktualizácia hlavy */
    UART_TxBuf[tmphead] = data;
    UART_TxHead         = tmphead;

    /* povolenie prerušenia UDRE – UART začne prenášať dáta z buffera */
    UART0_CONTROL |= _BV(UART0_UDRIE);
}/* uart_putc */


/**
 * @brief Odošle nulou ukončený reťazec z RAM cez UART0.
 *
 * Reťazec @p s sa postupne prechádza a každý znak sa odošle cez
 * funkciu #uart_putc(). Odosielanie samotné prebieha na pozadí
 * pomocou prerušení, funkcia sa stará len o napĺňanie TX buffera.
 *
 * @param s Ukazovateľ na nulou ukončený reťazec v RAM.
 */
void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}/* uart_puts */


/**
 * @brief Odošle reťazec uložený v programovej pamäti (PROGMEM) cez UART0.
 *
 * Reťazec @p progmem_s je uložený vo Flash pamäti. Funkcia ho postupne
 * číta pomocou #pgm_read_byte() a jednotlivé znaky odosiela cez #uart_putc().
 *
 * @param progmem_s Ukazovateľ na reťazec v programovej pamäti.
 */
void uart_puts_p(const char *progmem_s)
{
    register char c;

    while ( (c = pgm_read_byte(progmem_s++)) )
        uart_putc(c);
}/* uart_puts_p */


/*
 * Nasledujúce funkcie sú dostupné len na ATmegách s dvoma USART rozhraniami.
 * Rozhranie UART1 (druhé USART) má obdobnú funkcionalitu ako UART0.
 */
#if defined( ATMEGA_USART1 )

/**
 * @brief Prerušovacia rutina: UART1 prijal znak (Receive Complete).
 *
 * Implementácia je analógická k ISR pre UART0, ale pracuje s oddelenými
 * buffermi a registrami UART1.
 */
ISR(UART1_RECEIVE_INTERRUPT)
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;

    /* prečítanie stavového registra UART1 a registra s prijatými dátami */
    usr  = UART1_STATUS;
    data = UART1_DATA;

    /* zistenie chýb prijímača: Frame Error, Data OverRun, Parity Error */
    lastRxError = usr & (_BV(FE1) | _BV(DOR1) | _BV(UPE1) );

    /* výpočet ďalšieho indexu v kruhovom prijímacom buffere UART1 */
    tmphead = ( UART1_RxHead + 1) & UART_RX_BUFFER_MASK;

    if (tmphead == UART1_RxTail)
    {
        /* chyba: prijímací buffer UART1 je plný – pretečenie buffera */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        /* uloženie nového indexu hlavy a prijatého bajtu do buffera */
        UART1_RxHead = tmphead;
        UART1_RxBuf[tmphead] = data;
    }
    /* akumulácia chybového stavu pre UART1 */
    UART1_LastRxError |= lastRxError;
}


/**
 * @brief Prerušovacia rutina: UART1 pripravený odoslať ďalší bajt (Data Register Empty).
 *
 * Analogické správanie ako ISR pre UART0 – odosielanie dát z TX buffera
 * rozhrania UART1 a vypnutie UDRE prerušenia pri vyprázdnení buffera.
 */
ISR(UART1_TRANSMIT_INTERRUPT)
{
    unsigned char tmptail;

    if (UART1_TxHead != UART1_TxTail)
    {
        /* výpočet nového indexu chvosta TX buffera UART1 */
        tmptail      = (UART1_TxTail + 1) & UART_TX_BUFFER_MASK;
        UART1_TxTail = tmptail;
        /* prenesenie bajtu z buffera do dátového registra UART1 */
        UART1_DATA = UART1_TxBuf[tmptail]; /* začiatok odosielania */
    }
    else
    {
        /* TX buffer UART1 je prázdny – zakážeme UDRE prerušenie */
        UART1_CONTROL &= ~_BV(UART1_UDRIE);
    }
}


/**
 * @brief Inicializuje UART1 a nastaví prenosovú rýchlosť.
 *
 * Funkcia pracuje rovnako ako #uart_init(), ale pre druhé USART
 * rozhranie mikrokontroléra (UART1).
 *
 * @param baudrate Hodnota pre UBRR register, vypočítaná cez výpočtové makro.
 */
void uart1_init(unsigned int baudrate)
{
    /* inicializácia indexov TX/RX bufferov UART1 */
    UART1_TxHead = 0;
    UART1_TxTail = 0;
    UART1_RxHead = 0;
    UART1_RxTail = 0;

    # ifdef UART_TEST
    #  ifndef UART1_BIT_U2X
    #   warning "UART1_BIT_U2X not defined"
    #  endif
    #  ifndef UART1_UBRRH
    #   warning "UART1_UBRRH not defined"
    #  endif
    #  ifndef UART1_CONTROLC
    #   warning "UART1_CONTROLC not defined"
    #  endif
    #  if defined(URSEL) || defined(URSEL1)
    #   ifndef UART1_BIT_URSEL
    #    warning "UART1_BIT_URSEL not defined"
    #   endif
    #  endif
    # endif /* ifdef UART_TEST */

    /* Nastavenie baud rate pre UART1 – voliteľný režim 2× rýchlosť (U2X) */
    if (baudrate & 0x8000)
    {
        # if UART1_BIT_U2X
        UART1_STATUS = (1 << UART1_BIT_U2X); // povoliť 2× rýchlosť pre UART1
        # endif
    }
    UART1_UBRRH = (unsigned char) ((baudrate >> 8) & 0x80);
    UART1_UBRRL = (unsigned char) baudrate;

    /* povolenie prijímača, vysielača a prerušenia pri dokončení prijmu */
    UART1_CONTROL = _BV(UART1_BIT_RXCIE) | (1 << UART1_BIT_RXEN) | (1 << UART1_BIT_TXEN);

    /* formát rámca: asynchrónny, 8 dátových bitov, bez parity, 1 stop bit */
    # ifdef UART1_BIT_URSEL
    UART1_CONTROLC = (1 << UART1_BIT_URSEL) | (1 << UART1_BIT_UCSZ1) | (1 << UART1_BIT_UCSZ0);
    # else
    UART1_CONTROLC = (1 << UART1_BIT_UCSZ1) | (1 << UART1_BIT_UCSZ0);
    # endif
}/* uart1_init */


/**
 * @brief Získa bajt z prijímacieho kruhového bufferu UART1.
 *
 * Správanie je totožné s #uart_getc(), ale pre druhé USART rozhranie (UART1).
 *
 * @return 16-bitová hodnota:
 *   - nižší bajt: prijatý znak,
 *   - vyšší bajt: chybový kód alebo 0, ak prijatie prebehlo bez chyby.
 */
unsigned int uart1_getc(void)
{
    unsigned char tmptail;
    unsigned int data;
    unsigned char lastRxError;

    /* ak sú indexy hlavy a chvosta rovnaké, buffer UART1 je prázdny */
    if (UART1_RxHead == UART1_RxTail)
    {
        return UART_NO_DATA; /* žiadne dáta nie sú k dispozícii */
    }

    /* výpočet indexu chvosta prijímacieho buffera UART1 */
    tmptail = (UART1_RxTail + 1) & UART_RX_BUFFER_MASK;

    /* načítanie dát z buffera a chybového stavu */
    data        = UART1_RxBuf[tmptail];
    lastRxError = UART1_LastRxError;

    /* uloženie nového indexu chvosta */
    UART1_RxTail = tmptail;

    /* po prečítaní chyby ju vynulujeme */
    UART1_LastRxError = 0;

    return (lastRxError << 8) + data;
}/* uart1_getc */


/**
 * @brief Zapíše bajt do vysielacieho kruhového bufferu UART1.
 *
 * Funkcia čaká na voľné miesto v TX buffere UART1, uloží bajt
 * a povolí prerušenie UDRE pre odosielanie.
 *
 * @param data Bajt, ktorý sa má odoslať cez UART1.
 */
void uart1_putc(unsigned char data)
{
    unsigned char tmphead;

    /* výpočet nového indexu hlavy TX buffera UART1 */
    tmphead = (UART1_TxHead + 1) & UART_TX_BUFFER_MASK;

    /* čakanie, kým sa v TX buffere neuvoľní miesto */
    while (tmphead == UART1_TxTail)
    {
        ;/* čakaj na voľné miesto v buffere */
    }

    UART1_TxBuf[tmphead] = data;
    UART1_TxHead         = tmphead;

    /* povolenie UDRE prerušenia pre UART1 */
    UART1_CONTROL |= _BV(UART1_UDRIE);
}/* uart1_putc */


/**
 * @brief Odošle nulou ukončený reťazec z RAM cez UART1.
 *
 * Správanie je analogické k #uart_puts(), ale pre rozhranie UART1.
 *
 * @param s Ukazovateľ na nulou ukončený reťazec v RAM.
 */
void uart1_puts(const char *s)
{
    while (*s)
        uart1_putc(*s++);
}/* uart1_puts */


/**
 * @brief Odošle reťazec uložený v programovej pamäti (PROGMEM) cez UART1.
 *
 * Správanie je analogické k #uart_puts_p(), ale pre rozhranie UART1.
 *
 * @param progmem_s Ukazovateľ na reťazec v programovej pamäti.
 */
void uart1_puts_p(const char *progmem_s)
{
    register char c;

    while ( (c = pgm_read_byte(progmem_s++)) )
        uart1_putc(c);
}/* uart1_puts_p */

#endif /* if defined( ATMEGA_USART1 ) */
