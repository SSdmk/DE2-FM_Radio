/* 
 *  Muthanna Alwahash 2020/21
 *
 */

#include "Si4703.h"
#include "gpio.h"
#include <util/delay.h>
#include "twi.h"

Si4703 radio;

//-----------------------------------------------------------------------------------------------------------------------------------
// Inicializácia triedy Si4703 – uloženie parametrov a nastavení
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Konštruktor triedy Si4703 – nastaví piny MCU a základné parametre rádia.
 *
 * @param rstPin  GPIO pin MCU pripojený na reset Si4703.
 * @param sdioPin GPIO pin MCU pre I2C dátovú linku (SDIO/SDA).
 * @param sclkPin GPIO pin MCU pre I2C hodinovú linku (SCLK/SCL).
 * @param intPin  GPIO pin MCU pre STC/RDS interrupt (ak sa používa).
 * @param band    Pásmo (US/EU/JP) podľa konštánt BAND_*.
 * @param space   Rozstup kanálov podľa konštánt SPACE_*.
 * @param de      De-emfáza (50/75 μs).
 * @param skmode  Režim seekovania (wrap/stop).
 * @param seekth  RSSI prah pre seek.
 * @param skcnt   Prah impulznej detekcie pri seeku.
 * @param sksnr   SNR prah pre seek.
 * @param agcd    Nastavenie AGC (0 = povolené, 1 = zakázané).
 */
Si4703::Si4703(
    int rstPin,
    int sdioPin,
    int sclkPin,
    int intPin,
    int band,
    int space,
    int de,
    int skmode,
    int seekth,
    int skcnt,
    int sksnr,
    int agcd
)
{
  // Výber pinov MCU
  _rstPin   = rstPin;   // Reset pin Si4703
  _sdioPin  = sdioPin;  // I2C dátová linka
  _sclkPin  = sclkPin;  // I2C hodinová linka
  _intPin   = intPin;   // Pin pre STC/RDS interrupt

  // Nastavenia pásma
  _band     = band;	    // Kód pásma
  _space    = space;    // Rozstup kanálov
  _de       =	de;	    // De-emfáza

  // RDS nastavenia (rezervované pre ďalšie úpravy)

  // Tune nastavenia (rezervované pre ďalšie úpravy)

  // Nastavenia seeku
  _skmode   =	skmode;  // Režim seekovania (wrap/stop)
  _seekth   =	seekth;  // RSSI prah pre seek
  _skcnt    =	skcnt;   // Prah impulznej detekcie
  _sksnr    =	sksnr;	// SNR prah pre seek
  _agcd     = agcd;     // AGC disable flag
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Načítanie celého registračného priestoru (0x00 - 0x0F) do shadow štruktúry
// Číta sa v poradí: 0A,0B,0C,0D,0E,0F,00,01,02,03,04,05,06,07,08,09 = 16 slov = 32 bajtov
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Načíta všetkých 16 registrov Si4703 do internej „shadow“ štruktúry.
 *
 * Funkcia:
 *  - otvorí I2C komunikáciu v režime čítania,
 *  - prečíta 32 bajtov v definovanom poradí,
 *  - poskladá z nich 16-bitové slová do poľa @c shadow.word.
 */
void Si4703::getShadow()
{
    // Adresa slave zariadenia posunutá o 1 bit doľava, doplnený R/W bit (1 = READ)
    uint8_t slave_read_addr = (I2C_ADDR << 1) | TWI_READ; 
    
    // Začiatok komunikácie a odoslanie SLA+R
    twi_start();
    if (twi_write(slave_read_addr) != TWI_ACK) {
        // Zariadenie neodpovedalo ACK – ukončíme komunikáciu
        twi_stop();
        return;
    }
    
    // Čítanie 32 bajtov (16 16-bitových registrov)
    for (int i = 0; i < 16; i++) {
        uint8_t msb, lsb;

        // Čítanie horného bajtu (MSB) – všetky okrem posledného LSB dostanú ACK
        msb = twi_read(TWI_ACK);

        // Čítanie dolného bajtu (LSB)
        if (i < 15) {
            // LSB 1. až 15. slova dostanú ACK, aby slave pokračoval ďalším bajtom
            lsb = twi_read(TWI_ACK);
        } else {
            // LSB 16. slova je posledný bajt – posielame NACK
            lsb = twi_read(TWI_NACK);
        }
        
        // Zloženie 16-bitového slova z MSB a LSB
        shadow.word[i] = ((uint16_t)msb << 8) | lsb;
    }

    // Ukončenie I2C komunikácie
    twi_stop();
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zápis riadiacich registrov (0x02 až 0x07) z shadow štruktúry do Si4703
// Čip predpokladá, že prvý zapisovaný register je 0x02 a ďalej sa adresa inkrementuje
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zapíše riadiace registre zo shadow štruktúry do čipu Si4703.
 *
 * Funkcia zapisuje subset registrov:
 *  - postupne od indexu 8 po 13 v poli @c shadow.word,
 *  - každý register ako MSB a LSB bajt cez I2C.
 *
 * @return 0 pri úspechu, nenulový kód pri chybe (NACK po adrese alebo bajte).
 */
uint8_t Si4703::putShadow()
{
    // Adresa slave zariadenia posunutá o 1 bit doľava, R/W bit = zápis
    uint8_t slave_write_addr = (I2C_ADDR << 1) | TWI_WRITE; 
    uint8_t result = 0; // Výsledok posledného twi_write

    // 1. Začiatok komunikácie (START)
    twi_start();
    
    // 2. Odoslanie SLA+W
    result = twi_write(slave_write_addr); 
    
    // Kontrola ACK od slave
    if (result != TWI_ACK) {
        twi_stop();
        return 1; // Chyba: NACK po adrese
    }

    // 3. Odoslanie 6 registrov (12 bajtov) – horný a dolný bajt
    for (int i = 8; i < 14; i++) { 
        // Horný bajt
        uint8_t upper_byte = shadow.word[i] >> 8;
        result = twi_write(upper_byte); 
        if (result != TWI_ACK) {
            twi_stop();
            return 2; // Chyba: NACK po hornom bajte
        }
        
        // Dolný bajt
        uint8_t lower_byte = shadow.word[i] & 0x00FF;
        result = twi_write(lower_byte); 
        if (result != TWI_ACK) {
            twi_stop();
            return 3; // Chyba: NACK po dolnom bajte
        }
    }
    
    // 4. Ukončenie komunikácie (STOP)
    twi_stop();

    // Všetko prebehlo úspešne
    return 0; 
}

//-----------------------------------------------------------------------------------------------------------------------------------
// 3-vodičové rozhranie (SCLK, SEN, SDIO) – zatiaľ neimplementované
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Inicializácia 3-wire rozhrania (SCLK, SEN, SDIO).
 *
 * Funkcia je zatiaľ neimplementovaná (TODO). Projekt používa 2-wire (I2C) režim.
 */
void Si4703::bus3Wire(void)
{
  // TODO: Implementácia 3-wire rozhrania podľa dokumentácie Si4703
}			

//-----------------------------------------------------------------------------------------------------------------------------------
// 2-vodičové rozhranie (I2C: SCLCK, SDIO) – príprava pinov a prepnutie čipu do 2-wire módu
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Inicializuje 2-wire (I2C) rozhranie pre Si4703 a prepne čip do I2C módu.
 *
 * Kroky:
 *  - nastaví smer pinov RST a SDIO,
 *  - privedie čip do resetu so SDIO = LOW (signál pre 2-wire mód),
 *  - pustí reset a inicializuje TWI (I2C) jednotku.
 */
void Si4703::bus2Wire(void)		
{
  // Nastavenie smeru pinov
  gpio_mode_output(&DDRD, _rstPin);    // Reset pin
  gpio_mode_output(&DDRC, _sdioPin);   // I2C dátová linka
  
  // Nastavenie komunikačného režimu na 2-wire
  gpio_write_low(&PORTD, _rstPin);   // Si4703 do resetu
  gpio_write_low(&PORTC, _sdioPin);  // SDIO = LOW -> 2-wire rozhranie
  _delay_ms(1);                      // Krátke čakanie na ustálenie pinov
  gpio_write_high(&PORTD, _rstPin);  // Uvoľnenie resetu so SDIO = LOW
  _delay_ms(1);                      // Čas na naštartovanie čipu

  // Inicializácia TWI (I2C) po prechode do 2-wire módu
  twi_init();
}	

//-----------------------------------------------------------------------------------------------------------------------------------
// Power-up sekvencia – zapnutie oscilátora a povolenie čipu
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zapne čip Si4703 – najprv oscilátor, potom samotné rádio.
 *
 * Kroky:
 *  - načíta registre do shadow,
 *  - povolí kryštálový oscilátor (XOSCEN),
 *  - po čakaní povolí napájanie rádia (ENABLE) a zruší MUTE (DMUTE).
 */
void Si4703::powerUp()
{
  // Povolenie oscilátora
  getShadow();                            // Načítanie registrov
  shadow.reg.TEST1.bits.XOSCEN = 1;       // Povoliť oscilátor
  putShadow();                            // Zápis do registrov
  _delay_ms(500);                         // Čas na ustálenie oscilátora

  // Povolenie zariadenia
  getShadow();                            // Načítanie registrov
  shadow.reg.POWERCFG.bits.ENABLE   = 1;  // Powerup enable
  shadow.reg.POWERCFG.bits.DISABLE  = 0;  // Powerdown disable
  shadow.reg.POWERCFG.bits.DMUTE    = 1;  // Zrušiť mute (audio zapnuté)
  putShadow();                            // Zápis do registrov
  _delay_ms(110);                         // Max. čas power-up podľa datasheetu
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Power-down sekvencia – vypnutie audio výstupu a rádia
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vypne čip Si4703 a nastaví audio výstupy do vysokej impedancie.
 *
 * Kroky:
 *  - nastaví výstupy do Hi-Z,
 *  - GPIO piny do Hi-Z,
 *  - nastaví príznaky pre power-down a zapíše registre.
 */
void Si4703::powerDown()
{
  getShadow();                                // Načítanie registrov
  shadow.reg.TEST1.bits.AHIZEN      = 1;      // Audio výstupy do vysokej impedancie

  shadow.reg.SYSCONFIG1.bits.GPIO1  = GPIO_Z; // GPIO1 = Hi-Z
  shadow.reg.SYSCONFIG1.bits.GPIO2  = GPIO_Z; // GPIO2 = Hi-Z
  shadow.reg.SYSCONFIG1.bits.GPIO3  = GPIO_Z; // GPIO3 = Hi-Z

  shadow.reg.POWERCFG.bits.DMUTE    = 0;      // Mute (audio vypnuté)
  shadow.reg.POWERCFG.bits.ENABLE   = 1;      // Power-up bit ponechaný
  shadow.reg.POWERCFG.bits.DISABLE  = 1;      // Power-down aktivovaný
  
  putShadow();                                // Zápis do registrov
  _delay_ms(2);                               // Krátky čas na vypnutie
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Štart rádia – prepnutie do 2-wire módu, power-up a základná konfigurácia
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Spustí rádio: nastaví I2C režim, zapne čip a vykoná základnú konfiguráciu.
 *
 * Robí:
 *  - inicializáciu 2-wire rozhrania,
 *  - power-up sekvenciu,
 *  - nastavenie pásma, rozstupu, de-emfázy,
 *  - konfiguráciu seeku, RDS, audio parametrov a GPIO,
 *  - uloženie konfigurácie zápisom shadow registrov.
 */
void Si4703::start() 
{
  bus2Wire();   // Inicializácia 2-wire rozhrania (I2C)
  powerUp();    // Power-up čipu

  // Predvolená začiatočná konfigurácia
  getShadow();                            // Načítanie registrov

  // Výber pásma a regiónu
  setRegion(_band,_space,_de);                      // Nastavenie hraníc pásma
  shadow.reg.SYSCONFIG2.bits.SPACE  = _space;       // Rozstup kanálov
  shadow.reg.SYSCONFIG2.bits.BAND   = _band;        // Pásmo
  shadow.reg.SYSCONFIG1.bits.DE     = _de;          // De-emfáza

  // Nastavenie tuningu
  shadow.reg.SYSCONFIG1.bits.STCIEN = 0;            // Interrupt STC vypnutý

  // Nastavenie seek režimu
  shadow.reg.POWERCFG.bits.SEEK     = 0;            // Seek vypnutý
  shadow.reg.POWERCFG.bits.SEEKUP   = 1;            // Predvolený smer seeku = hore
  shadow.reg.POWERCFG.bits.SKMODE   = _skmode;      // Režim seeku (wrap/stop)
  shadow.reg.SYSCONFIG2.bits.SEEKTH = _seekth;      // RSSI prah pre seek
  shadow.reg.SYSCONFIG3.bits.SKCNT  = _skcnt;       // Prah impulzov
  shadow.reg.SYSCONFIG3.bits.SKSNR  = _sksnr;       // SNR prah
  shadow.reg.SYSCONFIG1.bits.AGCD   = _agcd;        // AGC disable

  // Nastavenie RDS
  shadow.reg.SYSCONFIG1.bits.RDSIEN = 0;            // RDS interrupt vypnutý
  shadow.reg.POWERCFG.bits.RDSM     = 0;            // RDS štandardný režim
  shadow.reg.SYSCONFIG1.bits.RDS    = 1;            // RDS povolené

  // Nastavenie audia
  shadow.reg.TEST1.bits.AHIZEN      = 0;            // Audio výstupy povolené
  shadow.reg.POWERCFG.bits.MONO     = 0;            // Stereo režim
  shadow.reg.SYSCONFIG1.bits.BLNDADJ= BLA_31_49;    // Blend úroveň 31–49 dBμV
  shadow.reg.SYSCONFIG2.bits.VOLUME = 0;            // Začiatok s hlasitosťou 0
  shadow.reg.SYSCONFIG3.bits.VOLEXT = 0;            // Rozšírený rozsah hlasitosti vypnutý
  
  // Nastavenie softmute
  shadow.reg.POWERCFG.bits.DSMUTE   = 1;            // Softmute vypnutý
  shadow.reg.SYSCONFIG3.bits.SMUTEA = SMA_16dB;     // Softmute útlm 16 dB
  shadow.reg.SYSCONFIG3.bits.SMUTER = SMRR_Fastest; // Najrýchlejšia reakcia softmute

  // Nastavenie GPIO pinov
  shadow.reg.SYSCONFIG1.bits.GPIO1  = GPIO_Z;       // GPIO1 = Hi-Z
  shadow.reg.SYSCONFIG1.bits.GPIO2  = GPIO_Z;       // GPIO2 = Hi-Z
  shadow.reg.SYSCONFIG1.bits.GPIO3  = GPIO_Z;       // GPIO3 = Hi-Z

  putShadow();                                      // Zápis konfigurácie do čipu
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Nastavenie hraníc pásma a rozstupu kanálov
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Nastaví hranice pásma a rozstup kanálov podľa zvoleného regiónu.
 *
 * @param band  Kód pásma (BAND_US_EU, BAND_JPW, BAND_JP).
 * @param space Kód rozstupu (SPACE_50KHz/100KHz/200KHz).
 * @param de    De-emfáza (aktuálne len uložené do člena, vlastné spracovanie je inde).
 */
void Si4703::setRegion(int band,   // Band Range
                       int space,  // Band Spacing
                       int de)     // De-Emphasis
{
  (void)de; // de sa reálne spracúva inde, tu sa riešia hlavne frekvencie

  switch (band)
  {
    case BAND_US_EU:      // 87.5–108 MHz (US / Európa)
      _bandStart = 8750;  // Spodná hranica pásma (kHz)
      _bandEnd   = 10800; // Horná hranica pásma (kHz)
      break;
    
    case BAND_JPW:        // 76–108 MHz (Japonsko – široké pásmo)
      _bandStart = 7600;  // Spodná hranica pásma (kHz)
      _bandEnd   = 10800; // Horná hranica pásma (kHz)
      break;
    
    case BAND_JP:         // 76–90 MHz (Japonsko – úzke pásmo)
      _bandStart = 7600;  // Spodná hranica pásma (kHz)
      _bandEnd   = 9000;  // Horná hranica pásma (kHz)
      break;

    default:
      break;
  }

  switch (space)
  {
    case SPACE_100KHz:
      _bandSpacing = 10;  // 100 kHz krok (v kHz)
      break;

    case SPACE_200KHz:
      _bandSpacing = 20;  // 200 kHz krok
      break;

    case SPACE_50KHz:
      _bandSpacing = 5;   // 50 kHz krok
      break;
    
    default:
      break;
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Nastavenie mono režimu
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Prepne rádio do mono režimu alebo späť do stereo.
 *
 * @param en true = mono, false = stereo.
 */
void Si4703::setMono(bool en)
{
  getShadow();                            // Načítanie registrov
  shadow.reg.POWERCFG.bits.MONO = en;     // Nastavenie mono bitu
  putShadow();                            // Zápis registrov
}	

//-----------------------------------------------------------------------------------------------------------------------------------
// Zistenie mono režimu
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zistí, či je zapnutý mono režim.
 *
 * @return true ak je rádio v mono režime, inak false.
 */
bool Si4703::getMono(void)
{
  getShadow();                              // Načítanie registrov
  return (shadow.reg.POWERCFG.bits.MONO);   // Stav mono bitu
}	

//-----------------------------------------------------------------------------------------------------------------------------------
// Nastavenie mute (ticho)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Nastaví alebo zruší mute (vypnutie zvuku).
 *
 * @param en true = audio zapnuté (DMUTE=1), false = audio vypnuté.
 */
void Si4703::setMute(bool en)
{
  getShadow();                              // Načítanie registrov
  shadow.reg.POWERCFG.bits.DMUTE = en;      // Nastavenie DMUTE
  putShadow();                              // Zápis registrov
}	

//-----------------------------------------------------------------------------------------------------------------------------------
// Zistenie mute stavu
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zistí aktuálny mute stav.
 *
 * @return true ak je DMUTE=1 (audio povolené), false ak je DMUTE=0.
 */
bool Si4703::getMute(void)
{
  getShadow();                              // Načítanie registrov
  return (shadow.reg.POWERCFG.bits.DMUTE);  // Stav DMUTE
}	

//-----------------------------------------------------------------------------------------------------------------------------------
// Nastavenie rozšíreného rozsahu hlasitosti
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Povolenie alebo zakázanie rozšíreného rozsahu hlasitosti (VOLEXT).
 *
 * @param en true = rozšírený rozsah, false = normálny rozsah.
 */
void Si4703::setVolExt(bool en)
{
  getShadow();                              // Načítanie registrov
  shadow.reg.SYSCONFIG3.bits.VOLEXT = en;   // Nastavenie VOLEXT
  putShadow();                              // Zápis registrov
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zistenie rozšíreného rozsahu hlasitosti
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zistí, či je zapnutý rozšírený rozsah hlasitosti.
 *
 * @return true ak je VOLEXT=1, inak false.
 */
bool Si4703::getVolExt(void)
{
  getShadow();                               // Načítanie registrov
  return (shadow.reg.SYSCONFIG3.bits.VOLEXT);// Stav VOLEXT
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Aktuálna hlasitosť
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti aktuálnu nastavenú hlasitosť.
 *
 * @return Hodnota v rozsahu 0–15.
 */
int Si4703::getVolume(void)
{
  getShadow();                                // Načítanie registrov
  return (shadow.reg.SYSCONFIG2.bits.VOLUME);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Nastavenie hlasitosti
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Nastaví hlasitosť na zadanú hodnotu v rozsahu 0–15.
 *
 * Hodnota sa orezáva do povoleného rozsahu. Po nastavení vráti
 * aktuálnu hlasitosť načítanú z registra.
 *
 * @param volume Hodnota hlasitosti 0–15.
 * @return Skutočne nastavená hlasitosť.
 */
int Si4703::setVolume(int volume)
{
  getShadow();                                // Načítanie registrov
  if (volume < 0 ) volume = 0;                // Orezanie spodnej hranice
  if (volume > 15) volume = 15;               // Orezanie hornej hranice
  shadow.reg.SYSCONFIG2.bits.VOLUME = volume; // Nastavenie hlasitosti
  putShadow();                                // Zápis registrov
  return (getVolume());
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zvýšenie hlasitosti o 1 krok
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zvýši hlasitosť o jeden krok.
 *
 * @return Nová hodnota hlasitosti.
 */
int Si4703::incVolume(void)
{
  return (setVolume(getVolume() + 1));
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zníženie hlasitosti o 1 krok
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zníži hlasitosť o jeden krok.
 *
 * @return Nová hodnota hlasitosti.
 */
int Si4703::decVolume(void)
{
  return (setVolume(getVolume() - 1));
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie aktuálne naladenej frekvencie z READCHAN
// Vracia napr. 9740 pre 97,40 MHz (v kHz)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Získa aktuálne naladenú frekvenciu na základe registra READCHAN.
 *
 * Výpočet:
 *  - Freq = _bandSpacing * READCHAN + _bandStart (v kHz).
 *
 * @return Frekvencia v kHz.
 */
int Si4703::getChannel()
{
  getShadow();                                // Načítanie registrov
  
  // Freq = Spacing * Channel + Bottom of Band
  return (_bandSpacing * shadow.reg.READCHAN.bits.READCHAN + _bandStart);  
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Nastavenie kanála (frekvencie) v kHz
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Nastaví naladenú frekvenciu (v kHz) v rámci aktuálneho pásma.
 *
 * Hodnota je orezaná na interval <_bandStart, _bandEnd>. Interný kanál sa vypočíta
 * z rozdielu a rozstupu. Funkcia počká na dokončenie tuningu pomocou STC.
 *
 * @param freq Frekvencia v kHz.
 * @return Skutočne naladená frekvencia v kHz.
 */
int Si4703::setChannel(int freq)
{
  if (freq > _bandEnd)    freq = _bandEnd;    // Horná hranica
  if (freq < _bandStart)  freq = _bandStart;  // Spodná hranica

  // Freq     = Spacing * Channel + bandStart.
  // Channel  = (Freq - bandStart) / Spacing
  getShadow();                              // Načítanie registrov
  shadow.reg.CHANNEL.bits.CHAN  = (freq - _bandStart) / _bandSpacing;
  shadow.reg.CHANNEL.bits.TUNE  = 1;        // Spustenie tuningu
  putShadow();                              // Zápis registrov

  if (shadow.reg.SYSCONFIG1.bits.STCIEN == 0) // STC polling mód
    while (!getSTC());                       // Čakanie na nastavenie STC
  else
    {
      // Čakanie na HW interrupt STC (ak je použitý)
      // TODO: implementovať obsluhu interruptu podľa potreby
    }

  getShadow();                              // Obnovenie registrov
  shadow.reg.CHANNEL.bits.TUNE  = 0;        // Zrušenie TUNE bitu
  putShadow();                              // Zápis registrov
  while (getSTC());                         // Čakanie, kým čip vynuluje STC

  return getChannel();
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zvýšenie frekvencie o jeden krok v rámci pásma (s obehnutím)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zvýši frekvenciu o jeden krok (_bandSpacing) s obehnutím na spodnú hranicu.
 *
 * @return Nová naladená frekvencia v kHz.
 */
int Si4703::incChannel(void)
{
  int freq = getChannel();          // Aktuálna frekvencia
  freq += _bandSpacing;             // Posun o jeden krok hore

  if (freq > _bandEnd) {            // Ak preskočíme hornú hranicu
    freq = _bandStart;              // Skok na spodnú hranicu
  }

  return setChannel(freq);
}

/**
 * @brief Zníži frekvenciu o jeden krok (_bandSpacing) s obehnutím na hornú hranicu.
 *
 * @return Nová naladená frekvencia v kHz.
 */
int Si4703::decChannel(void)
{
  int freq = getChannel();          // Aktuálna frekvencia
  freq -= _bandSpacing;             // Posun o jeden krok dole

  if (freq < _bandStart) {          // Ak prejdeme pod spodnú hranicu
    freq = _bandEnd;                // Skok na hornú hranicu
  }

  return setChannel(freq);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zistenie STC (Seek/Tune Complete) stavu
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zistí stav príznaku STC (seek/tune complete).
 *
 * @return true ak je STC=1, inak false.
 */
bool Si4703::getSTC(void)
{
  getShadow();                                  // Načítanie registrov
  return (shadow.reg.STATUSRSSI.bits.STC);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Seek na ďalšiu dostupnú stanicu
// Vracia frekvenciu pri úspechu, 0 pri zlyhaní
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vykoná seek v zadanom smere a vráti nájdenú frekvenciu.
 *
 * @param seekDirection SEEK_UP alebo SEEK_DOWN.
 * @return Naladená frekvencia v kHz pri úspechu, 0 pri zlyhaní (band limit / nič nenašiel).
 */
int Si4703::seek(byte seekDirection)
{
  getShadow();                                      // Načítanie registrov
  shadow.reg.POWERCFG.bits.SEEKUP = seekDirection;  // Smer seeku
  shadow.reg.POWERCFG.bits.SEEK   = 1;              // Spustenie seeku
  putShadow();                                      // Zápis registrov

  if (shadow.reg.SYSCONFIG1.bits.STCIEN == 0)       // Polling STC
    {
      while (!getSTC())                             // Čakanie na nastavenie STC
      {
        _delay_ms(40);
        // Sem je možné doplniť zobrazenie progresu podľa READCHAN
        // TODO: podľa potreby
      }
    }
  else
    {
      // Čakanie na interrupt indikujúci STC (Seek/Tune Complete)
      // TODO: ak sa použije interruptová obsluha
    }
  
  getShadow();                                      // Načítanie registrov
  bool sfbl = shadow.reg.STATUSRSSI.bits.SFBL;      // Stav SFBL (band limit / fail)
  shadow.reg.POWERCFG.bits.SEEK   = 0;              // Ukončenie seeku
  putShadow();                                      // Zápis registrov
  while (getSTC());                                 // Čakanie na vynulovanie STC

  if (sfbl)  return 0;   // Neúspech: band limit alebo stanica nenájdená
  return getChannel();   // Úspech: vráti novú frekvenciu
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Seek nahor s obehnutím pásma pri zlyhaní
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Seek smerom nahor s pokusom obehnúť pásmo pri neúspechu.
 *
 * @return Naladená frekvencia v kHz alebo 0, ak sa nenašla žiadna stanica.
 */
int Si4703::seekUp()
{
    int result = seek(SEEK_UP);
    if (result == 0) {
        // Pravdepodobne horná hranica alebo bez stanice,
        // skočíme na spodnú hranicu a skúšame ešte raz
        setChannel(_bandStart);
        result = seek(SEEK_UP);
    }
    return result;
}

/**
 * @brief Seek smerom nadol s pokusom obehnúť pásmo pri neúspechu.
 *
 * @return Naladená frekvencia v kHz alebo 0 pri zlyhaní.
 */
int Si4703::seekDown()
{
    int result = seek(SEEK_DOWN);
    if (result == 0) {
        // Pravdepodobne spodná hranica alebo bez stanice,
        // skočíme na hornú hranicu a skúšame ešte raz
        setChannel(_bandEnd);
        result = seek(SEEK_DOWN);
    }
    return result;
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zistenie stereo/mono stavu (ST bit)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Zistí, či je aktuálny príjem stereo (ST=1) alebo mono.
 *
 * @return true ak je stereo, inak false.
 */
bool Si4703::getST(void)
{
  getShadow();                              // Načítanie registrov
  return (shadow.reg.STATUSRSSI.bits.ST);   // ST bit
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Čítanie RDS – zatiaľ neimplementované
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Prečíta RDS dáta z registračných blokov.
 *
 * Funkcia je pripravená ako vstupný bod pre spracovanie RDS (PS, RT a pod.),
 * aktuálne obsahuje len TODO.
 */
void Si4703::readRDS(void)
{ 
  // TODO: Implementovať čítanie a dekódovanie RDS blokov RDSA–RDSD
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Zápis na GPIO1–GPIO3
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Nastaví režim zvoleného GPIO pinu Si4703.
 *
 * @param GPIO Identifikátor pinu (GPIO1, GPIO2, GPIO3).
 * @param val  Režim pinu (GPIO_Z, GPIO_I, GPIO_Low, GPIO_High).
 */
void Si4703::writeGPIO(int GPIO, int val)
{
  getShadow();    // Načítanie registrov

  switch (GPIO)
  {
    case GPIO1:
      shadow.reg.SYSCONFIG1.bits.GPIO1 = val;
      break;

    case GPIO2:
      shadow.reg.SYSCONFIG1.bits.GPIO2 = val;
      break;

    case GPIO3:
      shadow.reg.SYSCONFIG1.bits.GPIO3 = val;
      break;

    default:
      break;
  }
  
  putShadow();  // Zápis registrov
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie PN (Part Number) z DEVICEID
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti Part Number (PN) z registra DEVICEID.
 *
 * @return Hodnota PN.
 */
int Si4703::getPN()
{
  getShadow();    // Načítanie registrov
  return (shadow.reg.DEVICEID.bits.PN);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie Manufacturer ID z DEVICEID
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti Manufacturer ID (MFGID) z registra DEVICEID.
 *
 * @return Hodnota MFGID.
 */
int Si4703::getMFGID()
{
  getShadow();    // Načítanie registrov
  return (shadow.reg.DEVICEID.bits.MFGID);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie REV (verzie čipu) z CHIPID
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti revíziu čipu (REV) z registra CHIPID.
 *
 * @return Hodnota REV.
 */
int Si4703::getREV()
{
  getShadow();    // Načítanie registrov
  return (shadow.reg.CHIPID.bits.REV);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie DEV (typ zariadenia) z CHIPID
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti typ zariadenia (DEV) z registra CHIPID.
 *
 * @return Hodnota DEV.
 */
int Si4703::getDEV()
{
  getShadow();    // Načítanie registrov
  return (shadow.reg.CHIPID.bits.DEV);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie verzie firmvéru z CHIPID
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti verziu firmvéru z registra CHIPID.
 *
 * @return Hodnota FIRMWARE.
 */
int Si4703::getFIRMWARE()
{
  getShadow();    // Načítanie registrov
  return (shadow.reg.CHIPID.bits.FIRMWARE);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Začiatok pásma (spodná hranica)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti spodnú hranicu aktuálne nastaveného pásma (v kHz).
 *
 * @return Spodná hranica pásma v kHz.
 */
int Si4703::getBandStart()
{
  return (_bandStart);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Koniec pásma (horná hranica)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti hornú hranicu aktuálne nastaveného pásma (v kHz).
 *
 * @return Horná hranica pásma v kHz.
 */
int Si4703::getBandEnd()
{
  return (_bandEnd);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Rozstup kanálov (krok v kHz)
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti rozstup kanálov (krok) v kHz.
 *
 * @return Hodnota _bandSpacing v kHz.
 */
int Si4703::getBandSpace()
{
  return (_bandSpacing);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Získanie aktuálnej hodnoty RSSI
//-----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Vráti aktuálnu hodnotu RSSI (intenzita prijímaného signálu).
 *
 * @return RSSI v jednotkách definovaných čipom (dBμV podľa dokumentácie).
 */
int Si4703::getRSSI(void)
{
  getShadow();                              // Načítanie registrov
  return (shadow.reg.STATUSRSSI.bits.RSSI); // RSSI hodnota
}
