#ifndef TWI_H
# define TWI_H

#ifdef __cplusplus
extern "C" {
#endif
/*
 * I2C/TWI library for AVR-GCC.
 * (c) 2018-2024 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and AVR 8-bit Toolchain 3.6.2.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

/**
 * @file 
 * @defgroup fryza_twi TWI knižnica <twi.h>
 * @code #include <twi.h> @endcode
 *
 * @brief Knižnica pre I2C/TWI komunikáciu pre AVR-GCC.
 *
 * Táto knižnica definuje funkcie pre komunikáciu cez TWI (I2C) z pohľadu
 * Master zariadenia. Funkcie využívajú vnútorný TWI modul mikrokontroléra AVR
 * a umožňujú jednoducho komunikovať so Slave perifériami na I2C/TWI zbernici.
 *
 * Implementované sú režimy Master transmit a Master receive podľa dokumentácie
 * mikrokontrolérov ATmega16 a ATmega328P.
 *
 * @note Knižnica predpokladá použitie vnútorného TWI modulu a vhodné
 *       pull-up rezistory na SDA a SCL. Konštanty F_CPU a F_SCL
 *       určujú výslednú bitovú rýchlosť.
 * @copyright (c) 2018-2024 Tomas Fryza, MIT license
 * @{
 */

// -- Includes -------------------------------------------------------
#include <avr/io.h>


// -- Defines --------------------------------------------------------
/**
 * @name Definícia frekvencií
 * @{
 */
#ifndef F_CPU
# define F_CPU 16000000 /**< @brief Frekvencia CPU v Hz, potrebná pre výpočet TWI_BIT_RATE_REG. */
#endif

#define F_SCL 100000 /**< @brief I2C/TWI bitová rýchlosť v Hz. Musí byť väčšia ako približne 31 kHz. */

/**
 * @brief Hodnota registra rýchlosti TWI (TWBR) podľa vzťahu
 *        \f$ f_{SCL} = \frac{f_{CPU}}{16 + 2 \cdot TWBR} \f$.
 */
#define TWI_BIT_RATE_REG ((F_CPU/F_SCL - 16) / 2) /**< @brief Hodnota pre TWI bit rate register. */
/** @} */


/**
 * @name Definícia portov a pinov
 * @{
 */

/**
 * @brief Port, na ktorom sú piny TWI (SDA, SCL).
 *
 * Pre väčšinu ATmega328P je to port C (PC4 = SDA, PC5 = SCL).
 */
#define TWI_PORT PORTC

/** @brief Pin SDA linky TWI na porte @ref TWI_PORT. */
#define TWI_SDA_PIN 4

/** @brief Pin SCL linky TWI na porte @ref TWI_PORT. */
#define TWI_SCL_PIN 5
/** @} */


/**
 * @name Ďalšie definície
 * @{
 */

/** @brief Režim zápisu na I2C/TWI zbernicu. Používa sa pri SLA+W. */
#define TWI_WRITE 0

/** @brief Režim čítania z I2C/TWI zbernice. Používa sa pri SLA+R. */
#define TWI_READ 1

/** @brief Hodnota ACK pri odpovedi na prijatý bajt na I2C/TWI zbernici. */
#define TWI_ACK 0

/** @brief Hodnota NACK pri odpovedi na prijatý bajt na I2C/TWI zbernici. */
#define TWI_NACK 1

/**
 * @brief Makro na získanie adresy registra smeru portu (DDR) z jeho PORT registra.
 *
 * @param _x PORT register, napr. PORTC.
 */
#define DDR(_x) (*(&_x - 1))

/**
 * @brief Makro na získanie adresy vstupného registra portu (PIN) z jeho PORT registra.
 *
 * @param _x PORT register, napr. PORTC.
 */
#define PIN(_x) (*(&_x - 2))
/** @} */


// -- Function prototypes --------------------------------------------

/**
 * @brief Inicializuje TWI jednotku, zapne interné pull-up rezistory a nastaví frekvenciu SCL.
 *
 * Funkcia:
 *  - zapne interné pull-up rezistory na pinoch @ref TWI_SDA_PIN a @ref TWI_SCL_PIN,
 *  - nastaví bitovú rýchlosť TWI podľa @ref TWI_BIT_RATE_REG a konštánt F_CPU a F_SCL,
 *  - povolí TWI modul mikrokontroléra.
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_init(void);


/**
 * @brief Vygeneruje podmienku Start na I2C/TWI zbernici.
 *
 * Funkcia odošle Start podmienku na zbernicu a čaká, kým je prenos
 * dokončený (TWI nastaví príslušný stav v TWSR).
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_start(void);


/**
 * @brief Zapíše jeden bajt na I2C/TWI zbernicu.
 *
 * Funkcia odošle bajt @p data na I2C/TWI zbernicu (typicky SLA+R/SLA+W alebo
 * dátový bajt) a následne čaká na dokončenie prenosu. Výsledkom je hodnota,
 * ktorá indikuje, či bol od Slave zariadenia prijatý ACK alebo NACK.
 *
 * @param  data Bajt, ktorý sa má odoslať na I2C/TWI zbernicu.
 *
 * @return ACK/NACK od Slave zariadenia:
 * @retval 0 ACK bol prijatý
 * @retval 1 NACK bol prijatý
 *
 * @note Funkcia vracia 0, ak je detegovaný stavový kód TWI 0x18, 0x28 alebo 0x40:
 *       - 0x18: SLA+W bol odoslaný a ACK prijatý\n
 *       - 0x28: Dátový bajt bol odoslaný a ACK prijatý\n
 *       - 0x40: SLA+R bol odoslaný a ACK prijatý\n
 */
uint8_t twi_write(uint8_t data);


/**
 * @brief Prečíta jeden bajt z I2C/TWI zbernice a potvrdí ho ACK alebo NACK.
 *
 * Po prijatí bajtu z I2C/TWI zbernice odošle Master buď ACK (ak chce čítať
 * ďalšie dáta), alebo NACK (ak ide o posledný prijímaný bajt).
 *
 * @param  ack Hodnota ACK/NACK, ktorá sa odošle po prijatí bajtu.
 *             Použi @ref TWI_ACK alebo @ref TWI_NACK.
 *
 * @return Prijatý dátový bajt.
 */
uint8_t twi_read(uint8_t ack);


/**
 * @brief Vygeneruje podmienku Stop na I2C/TWI zbernici.
 *
 * Funkcia odošle Stop podmienku, čím uvoľní zbernicu. Po vykonaní Stop
 * môže iný Master zariadenie prevziať kontrolu nad I2C/TWI zbernicou.
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_stop(void);


/**
 * @brief Otestuje prítomnosť I2C zariadenia na zbernici.
 *
 * Funkcia odošle na zbernicu slave adresu @p addr (spolu s bitom W) a sleduje,
 * či zariadenie odpovie ACK. Vhodné napríklad na zisťovanie, či je daná
 * periféria pripojená a pripravená na komunikáciu.
 *
 * @param  addr Slave adresa zariadenia (7-bitová adresa v horných bitoch).
 *
 * @return ACK/NACK od zariadenia:
 * @retval 0 ACK bol prijatý – zariadenie odpovedá
 * @retval 1 NACK bol prijatý – zariadenie neodpovedá
 */
uint8_t twi_test_address(uint8_t addr);


/**
 * @brief Prečíta blok dát z pamäte periférie do bufferu, počnúc danou adresou.
 *
 * Funkcia vykoná typickú sekvenciu čítania z „register-based“ I2C zariadenia:
 *  - odošle SLA+W a po ňom počiatočnú adresu @p memaddr,
 *  - opätovne odošle Start a SLA+R,
 *  - prečíta @p nbytes bajtov do @p buf.
 *
 * Používa sa napríklad pri čítaní z pamäťových zariadení, senzorov s
 * vnútornými registrami a podobne.
 *
 * @param addr    Slave adresa zariadenia.
 * @param memaddr Počiatočná vnútorná adresa (register) v zariadení, odkiaľ sa má čítať.
 * @param buf     Ukazovateľ na buffer, do ktorého sa majú zapisovať prijaté dáta.
 * @param nbytes  Počet bajtov, ktoré sa majú prečítať.
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_readfrom_mem_into(uint8_t addr, uint8_t memaddr, volatile uint8_t *buf, uint8_t nbytes);

/** @} */  /* koniec skupiny fryza_twi */


#ifdef __cplusplus
}
#endif

#endif
