/*
 * I2C/TWI library for AVR-GCC.
 * (c) 2018-2024 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and AVR 8-bit Toolchain 3.6.2.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

/**
 * @file
 * @brief Implementácia knižnice pre I2C/TWI komunikáciu.
 *
 * Súbor obsahuje implementáciu základných funkcií pre prácu s TWI (I2C)
 * na mikrokontroléroch AVR:
 *  - inicializáciu TWI jednotky,
 *  - generovanie START/STOP podmienok,
 *  - zápis a čítanie jedného bajtu,
 *  - test prítomnosti zariadenia na zbernici,
 *  - čítanie bloku dát z pamäte periférie.
 *
 * Funkcie používajú vnútorný TWI modul mikrokontroléra a predpokladajú
 * vhodne nastavené konštanty F_CPU, F_SCL a TWI_BIT_RATE_REG (pozri twi.h).
 */

// -- Includes -------------------------------------------------------
#include <twi.h>


// -- Functions ------------------------------------------------------

/**
 * @brief Inicializuje TWI jednotku, zapne interné pull-upy a nastaví frekvenciu SCL.
 *
 * Funkcia:
 *  - nastaví piny SDA a SCL ako vstupy s internými pull-up rezistormi,
 *  - nastaví pred-deľič TWI a hodnotu registra TWBR podľa @ref TWI_BIT_RATE_REG,
 *    čím určí výslednú frekvenciu TWI zbernice (SCL).
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_init(void)
{
    /* Zapnutie interných pull-up rezistorov na SDA a SCL */
    DDR(TWI_PORT) &= ~((1<<TWI_SDA_PIN) | (1<<TWI_SCL_PIN));
    TWI_PORT |= (1<<TWI_SDA_PIN) | (1<<TWI_SCL_PIN);

    /* Nastavenie frekvencie SCL:
       - TWPS1 a TWPS0 = 0 => pred-deľič 1
       - TWBR nastavený na hodnotu TWI_BIT_RATE_REG */
    TWSR &= ~((1<<TWPS1) | (1<<TWPS0));
    TWBR = TWI_BIT_RATE_REG;
}


/**
 * @brief Vygeneruje podmienku Start na I2C/TWI zbernici.
 *
 * Funkcia nastaví bity v registri TWCR tak, aby TWI modul vyslal Start
 * podmienku na zbernicu a čaká, kým je operácia dokončená (nastaví sa TWINT).
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_start(void)
{
    /* Odoslanie START podmienky:
       - TWINT = 1 (vynulovanie príznaku zápisom 1),
       - TWSTA = 1 (generovanie START),
       - TWEN = 1 (povolenie TWI). */
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

    /* Čakanie na dokončenie operácie (TWINT sa nastaví na 1) */
    while ((TWCR & (1<<TWINT)) == 0);
}


/**
 * @brief Zapíše jeden bajt na I2C/TWI zbernicu.
 *
 * Funkcia odošle bajt @p data (SLA+W, SLA+R alebo obyčajný dátový bajt) na
 * TWI zbernicu a po dokončení prenosu skontroluje stavový register TWSR.
 *
 * @param data Bajt, ktorý sa má odoslať na zbernicu.
 *
 * @return ACK/NACK od Slave zariadenia:
 * @retval 0 ACK bol prijatý (úspešný zápis)
 * @retval 1 NACK bol prijatý (zariadenie nepotvrdilo prenos)
 */
uint8_t twi_write(uint8_t data)
{
    uint8_t twi_status;

    /* Zapísanie SLA+R, SLA+W alebo dátového bajtu do dátového registra TWI */
    TWDR = data;

    /* Spustenie prenosu: 
       - TWINT = 1 (vynulovanie príznaku),
       - TWEN = 1 (povolenie TWI). */
    TWCR = (1<<TWINT) | (1<<TWEN);

    /* Čakanie na dokončenie prenosu (TWINT = 1) */
    while ((TWCR & (1<<TWINT)) == 0);

    /* Kontrola stavového registra TWSR (iba horných 5 bitov) */
    twi_status = TWSR & 0xf8;

    /* Stavové kódy:
         - 0x18: SLA+W odoslané a prijaté ACK
         - 0x28: dátový bajt odoslaný a prijaté ACK
         - 0x40: SLA+R odoslané a prijaté ACK */
    if (twi_status == 0x18 || twi_status == 0x28 || twi_status == 0x40)
        return 0;   /* ACK prijaté */
    else
        return 1;   /* NACK prijaté */
}


/**
 * @brief Prečíta jeden bajt z I2C/TWI zbernice a potvrdí ho ACK alebo NACK.
 *
 * Funkcia pripraví TWI modul na prijatie jedného bajtu a po jeho prijatí
 * odošle buď ACK (ak chce Master čítať ďalej), alebo NACK (ak ide o posledný bajt).
 *
 * @param ack Hodnota ACK/NACK, ktorá sa má odoslať po prijatí bajtu.
 *            Použi @ref TWI_ACK alebo @ref TWI_NACK.
 *
 * @return Prijatý bajt z I2C/TWI zbernice.
 */
uint8_t twi_read(uint8_t ack)
{
    if (ack == TWI_ACK)
        /* Čítanie s odoslaním ACK po prijatí bajtu (pokračujeme v čítaní) */
        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
    else
        /* Čítanie s odoslaním NACK po prijatí bajtu (posledný bajt) */
        TWCR = (1<<TWINT) | (1<<TWEN);

    /* Čakanie na dokončenie prijmu (TWINT = 1) */
    while ((TWCR & (1<<TWINT)) == 0);

    /* Vrátenie prijatého bajtu z registra TWDR */
    return (TWDR);
}


/**
 * @brief Vygeneruje podmienku Stop na I2C/TWI zbernici.
 *
 * Funkcia odošle Stop podmienku (TWSTO) a tým uvoľní zbernicu.
 * Stop sa odošle po ukončení prenosu dát alebo po chybe.
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_stop(void)
{
    /* Odoslanie STOP podmienky:
       - TWINT = 1 (vynulovanie príznaku),
       - TWSTO = 1 (generovanie STOP),
       - TWEN = 1 (povolenie TWI). */
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}


/**
 * @brief Otestuje prítomnosť jedného I2C zariadenia na zbernici.
 *
 * Funkcia:
 *  - vygeneruje START podmienku,
 *  - odošle SLA+W (adresu zariadenia s bitom zápisu),
 *  - sleduje, či zariadenie odpovie ACK,
 *  - vygeneruje STOP podmienku.
 *
 * @param addr 7-bitová slave adresa zariadenia (bez R/W bitu).
 *
 * @return ACK/NACK od zariadenia:
 * @retval 0 ACK bol prijatý – zariadenie odpovedá
 * @retval 1 NACK bol prijatý – zariadenie neodpovedá
 */
uint8_t twi_test_address(uint8_t addr)
{
    uint8_t ack;  // ACK odpoveď od Slave

    twi_start();
    ack = twi_write((addr<<1) | TWI_WRITE);
    twi_stop();

    return ack;
}


/**
 * @brief Prečíta blok dát z pamäte periférie do bufferu, počnúc danou adresou.
 *
 * Typické použitie pri zariadeniach, ktoré majú vnútorné registre alebo pamäť
 * (napr. senzory, EEPROM):
 *  - odošle sa SLA+W a počiatočná adresa @p memaddr,
 *  - odošle sa opakovaný START a SLA+R,
 *  - prečíta sa @p nbytes bajtov do @p buf.
 *
 * @param addr    Slave adresa zariadenia.
 * @param memaddr Počiatočná vnútorná adresa (register/pamäť), odkiaľ sa má čítať.
 * @param buf     Ukazovateľ na buffer, do ktorého sa budú ukladať prijaté dáta.
 * @param nbytes  Počet bajtov, ktoré sa majú prečítať.
 *
 * @return Funkcia nevracia žiadnu hodnotu.
 */
void twi_readfrom_mem_into(uint8_t addr, uint8_t memaddr, volatile uint8_t *buf, uint8_t nbytes)
{
    /* Začiatok zápisu počiatočnej adresy do zariadenia (write fáza) */
    twi_start();
    if (twi_write((addr<<1) | TWI_WRITE) == 0)
    {
        // Nastavenie počiatočnej adresy vnútornej pamäte/registra
        twi_write(memaddr);
        twi_stop();

        // Čítanie dát z periférie (read fáza)
        twi_start();
        twi_write((addr<<1) | TWI_READ);

        if (nbytes >= 2)
        {
            // Čítanie všetkých okrem posledného bajtu s ACK (pokračujeme v čítaní)
            for (uint8_t i=0; i<(nbytes-1); i++)
            {
                *buf++ = twi_read(TWI_ACK);
            }
        }

        // Posledný bajt čítame s NACK (ukončenie čítania)
        *buf = twi_read(TWI_NACK);
        twi_stop();
    }
    else
    {
        // Ak zariadenie neodpovedalo na SLA+W, ukončíme komunikáciu
        twi_stop();
    }
}
