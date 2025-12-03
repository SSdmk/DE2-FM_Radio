/* 
 *  Muthanna Alwahash 2020/21
 */

#ifndef Si4703_h
#define Si4703_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "gpio.h"

/**
 * @file
 * @brief Rozhranie pre ovládanie FM rádia Si4703.
 *
 * Tento hlavičkový súbor obsahuje:
 *  - konštanty a symbolické názvy pre nastavenie rádia (pásmo, de-emfáza,
 *    rozstup kanálov, GPIO režimy, parametre seeku atď.),
 *  - deklaráciu triedy @ref Si4703, ktorá zapúzdruje komunikáciu s čipom,
 *  - dátové štruktúry (union + bitové polia) reprezentujúce registre Si4703
 *    vrátane „shadow“ kópie všetkých registrov.
 *
 * Trieda @ref Si4703 poskytuje metódy na:
 *  - zapnutie/vypnutie čipu,
 *  - ladenie frekvencie a seekovanie,
 *  - čítanie RSSI a stavových príznakov (stereo, RDS, STC),
 *  - nastavenie hlasitosti, mute, mono, rozšíreného rozsahu hlasitosti,
 *  - prácu s RDS a GPIO pinmi.
 */

/**
 * @typedef byte
 * @brief Kratší alias pre uint8_t.
 */
using byte = uint8_t;

//------------------------------------------------------------------------------------------------------------
// Konštanty pre nastavenie pásma (band)

/** @brief USA/EU pásmo 87.5–108 MHz (predvolené). */
static const uint8_t BAND_US_EU = 0b00;
/** @brief Japonské široké pásmo 76–108 MHz. */
static const uint8_t BAND_JPW   = 0b01;
/** @brief Japonské úzke pásmo 76–90 MHz. */
static const uint8_t BAND_JP    = 0b10;

// De-emphasis

/** @brief De-emfáza 75 μs – používaná v USA (predvolené). */
static const uint8_t DE_75us = 0b0;
/** @brief De-emfáza 50 μs – používaná v Európe, Austrálii a Japonsku. */
static const uint8_t DE_50us = 0b1;

// Channel Spacing

/** @brief Rozstup kanálov 200 kHz (USA/Austrália, predvolené). */
static const uint8_t SPACE_200KHz = 0b00;
/** @brief Rozstup kanálov 100 kHz (Európa/Japonsko). */
static const uint8_t SPACE_100KHz = 0b01;
/** @brief Rozstup kanálov 50 kHz. */
static const uint8_t SPACE_50KHz  = 0b10;

// GPIO1-3 Pins

/** @brief Identifikátor GPIO1 pinu. */
static const uint8_t GPIO1 = 1;
/** @brief Identifikátor GPIO2 pinu. */
static const uint8_t GPIO2 = 2;
/** @brief Identifikátor GPIO3 pinu. */
static const uint8_t GPIO3 = 3;

// GPIO1-3 Possible Values

/** @brief GPIO v stave vysokej impedancie (Hi-Z, predvolené). */
static const uint8_t GPIO_Z    = 0b00;
/** @brief GPIO ako vstup/špeciálna funkcia (STC/RDS int, Mono/Stereo indikácia). */
static const uint8_t GPIO_I    = 0b01;
/** @brief GPIO v stave logickej nuly (nízka úroveň, GND). */
static const uint8_t GPIO_Low  = 0b10;
/** @brief GPIO v stave logickej jednotky (VIO). */
static const uint8_t GPIO_High = 0b11;

// Seek Mode

/** @brief Seek obieha pásmo – po dosiahnutí hranice pokračuje od druhej strany. */
static const uint8_t SKMODE_WRAP = 0b0;
/** @brief Seek zastaví na hranici pásma. */
static const uint8_t SKMODE_STOP = 0b1;

// Seek SNR Threshold

/** @brief Seek SNR prah vypnutý (predvolené). Hodnoty sú 0x0 až 0xF. */
static const uint8_t SKSNR_DIS = 0x0;
/** @brief Najnižší SNR prah – najviac zastávok pri seeku. */
static const uint8_t SKSNR_MIN = 0x1;
/** @brief Najvyšší SNR prah – najmenej zastávok pri seeku. */
static const uint8_t SKSNR_MAX = 0xF;

// Seek FM Impulse Detection Threshold

/** @brief Prah detekcie impulzov vypnutý (predvolené). */
static const uint8_t SKCNT_DIS = 0x0;
/** @brief Najvyššia citlivosť na impulzy – najviac zastávok. */
static const uint8_t SKCNT_MAX = 0x1;
/** @brief Najnižšia citlivosť na impulzy – najmenej zastávok. */
static const uint8_t SKCNT_MIN = 0xF;

// Softmute Attenuation

/** @brief Softmute útlm 16 dB (predvolené). */
static const uint8_t SMA_16dB = 0b00;
/** @brief Softmute útlm 14 dB. */
static const uint8_t SMA_14dB = 0b01;
/** @brief Softmute útlm 12 dB. */
static const uint8_t SMA_12dB = 0b10;
/** @brief Softmute útlm 10 dB. */
static const uint8_t SMA_10dB = 0b11;

// Softmute Attack/Recover Rate

/** @brief Najrýchlejšia rýchlosť nábehu/útlmu softmute. */
static const uint8_t SMRR_Fastest = 0b00;
/** @brief Rýchly nábeh/útlm softmute. */
static const uint8_t SMRR_Fast    = 0b01;
/** @brief Pomalý nábeh/útlm softmute. */
static const uint8_t SMRR_Slow    = 0b10;
/** @brief Najpomalší nábeh/útlm softmute. */
static const uint8_t SMRR_Slowest = 0b11;

// Stereo/Mono Blend Level Adjustment

/** @brief Rozsah blend 31–49 dBμV (predvolené). */
static const uint8_t BLA_31_49 = 0b00;
/** @brief Rozsah blend 37–55 dBμV (+6 dB). */
static const uint8_t BLA_37_55 = 0b01;
/** @brief Rozsah blend 19–37 dBμV (–12 dB). */
static const uint8_t BLA_19_37 = 0b10;
/** @brief Rozsah blend 25–43 dBμV (–6 dB). */
static const uint8_t BLA_25_43 = 0b11;

//------------------------------------------------------------------------------------------------------------

/**
 * @class Si4703
 * @brief Trieda zapúzdrujúca ovládanie FM rádia Si4703.
 *
 * Trieda rieši:
 *  - inicializáciu a zapnutie/vypnutie čipu,
 *  - nastavenie pásma, rozstupu kanálov a de-emfázy,
 *  - ladenie konkrétnej frekvencie a seekovanie nahor/nadol,
 *  - získanie RSSI, informácií o stereo/mono režime, mute stave a pod.,
 *  - ovládanie hlasitosti a rozšíreného rozsahu hlasitosti,
 *  - čítanie RDS dát,
 *  - konfiguráciu GPIO pinov Si4703.
 *
 * Komunikácia prebieha cez I2C (2-wire režim) alebo 3-wire rozhranie
 * podľa interných funkcií triedy.
 */
class Si4703
{
//------------------------------------------------------------------------------------------------------------
  public:
    /**
     * @brief Konštruktor triedy Si4703 s možnosťou nastaviť piny MCU a základné parametre rádia.
     *
     * @param rstPin   GPIO pin MCU pripojený na reset (RST) Si4703.
     * @param sdioPin  GPIO pin MCU pre I2C SDA (SDIO).
     * @param sclkPin  GPIO pin MCU pre I2C SCL (SCLK).
     * @param intPin   GPIO pin MCU pre STC/RDS interrupt (0 = nepoužitý).
     * @param band     Pásmo rádia (napr. @ref BAND_US_EU, @ref BAND_JPW, @ref BAND_JP).
     * @param space    Rozstup kanálov (napr. @ref SPACE_100KHz).
     * @param de       De-emfáza (napr. @ref DE_75us, @ref DE_50us).
     * @param skmode   Režim seekovania (wrap/stop, @ref SKMODE_WRAP/@ref SKMODE_STOP).
     * @param seekth   RSSI prah seekovania.
     * @param skcnt    Prah impulznej detekcie pri seeku (@ref SKCNT_MIN až @ref SKCNT_MAX).
     * @param sksnr    SNR prah seekovania (@ref SKSNR_MIN až @ref SKSNR_MAX).
     * @param agcd     AGC disable (0 = povolené, 1 = zakázané).
     */
    Si4703(	                
				// MCU Pins Selection
                int rstPin    = PD4,            // Reset Pin
			    int sdioPin   = PC4,           // I2C Data IO Pin
			    int sclkPin   = PC5,           // I2C Clock Pin
			    int intPin    = 0,	          // Seek/Tune Complete and RDS interrupt Pin

                // Band Settings
				int band    = BAND_US_EU,	// Band Range
                int space   = SPACE_100KHz,	// Band Spacing
                int de      = DE_75us,		// De-Emphasis
                
                // RDS Settings

                // Tune Settings

                // Seek Settings
				int skmode  = SKMODE_STOP,	// Seek Mode
				int seekth  = 24,	        // Seek Threshold
				int skcnt 	= SKSNR_MAX,    // Seek Clicks Number Threshold
				int sksnr	= SKCNT_MIN,    // Seek Signal/Noise Ratio
                int agcd	= 0				// AGC disable
    		);
		
    /// Zapne rádio (napájanie a základná inicializácia).
    void	powerUp();				
	/// Vypne rádio kvôli úspore energie.
	void	powerDown();			
	/// Spustí rádio (po powerUp nastaví prevádzkové režimy).
	void 	start();				

	/// Získa číslo dielu (Part Number) z registra DEVICEID.
	int		getPN();				
	/// Získa Manufacturer ID z registra DEVICEID.
	int		getMFGID();				
	/// Získa verziu čipu (REV) z registra CHIPID.
	int		getREV();				
	/// Získa typ zariadenia (DEV) z registra CHIPID.
	int		getDEV();				
	/// Získa verziu firmvéru z registra CHIPID.
	int		getFIRMWARE();			

	/// Vráti počiatočnú frekvenciu pásma v MHz.
	int		getBandStart();			
	/// Vráti koncovú frekvenciu pásma v MHz.
	int		getBandEnd();			
	/// Vráti rozstup kanálov (spacing) v kHz alebo MHz podľa konfigurácie.
	int		getBandSpace();			

	/// Získa aktuálnu hodnotu RSSI (Received Signal Strength Indicator).
	int		getRSSI(void);			

	/// Vráti aktuálny kanál (interné 3-miestne číslo).
	int 	getChannel(void);		
	/// Nastaví kanál podaním frekvencie; vráti nastavený kanál.
	int		setChannel(int freq);	
	/// Zvýši frekvenciu o jeden krok pásma.
	int		incChannel(void);		
	/// Zníži frekvenciu o jeden krok pásma.
	int		decChannel(void);		
	
	/// Seek smerom nahor; vráti naladený kanál alebo 0 pri neúspechu.
	int 	seekUp(void); 			
	/// Seek smerom nadol; vráti naladený kanál alebo 0 pri neúspechu.
	int 	seekDown(void); 		

	/// Nastaví nútený mono režim (true = mono).
	void	setMono(bool en);		
	/// Zistí, či je rádio v mono režime.
	bool	getMono(void);			
	/// Zistí, či je príjem stereo (ST = 1).
	bool	getST(void);			

	/// Nastaví mute (true = zvuk vypnutý).
	void	setMute(bool en);		
	/// Zistí aktuálny mute stav.
	bool	getMute(void);			
	/// Nastaví rozšírený rozsah hlasitosti (VOLEXT).
	void	setVolExt(bool en);		
	/// Zistí, či je rozšírený rozsah hlasitosti zapnutý.
	bool	getVolExt(void);		
	/// Získa aktuálnu hlasitosť (0–15).
	int		getVolume(void);		
	/// Nastaví hlasitosť (0–15); vráti nastavenú hodnotu.
	int		setVolume(int volume);	
	/// Zvýši hlasitosť o jeden krok; vráti novú hodnotu.
	int		incVolume(void);		
	/// Zníži hlasitosť o jeden krok; vráti novú hodnotu.
	int		decVolume(void);		

	/// Prečíta RDS dáta z registrov a spracuje ich (napr. PS/RT).
	void	readRDS(void);			

	/**
	 * @brief Zapíše hodnotu na GPIO piny Si4703.
	 *
	 * @param GPIO Identifikátor pinu (@ref GPIO1, @ref GPIO2, @ref GPIO3).
	 * @param val  Režim pinu (@ref GPIO_Z, @ref GPIO_I, @ref GPIO_Low, @ref GPIO_High).
	 */
	void	writeGPIO(int GPIO,
					  int val);

//------------------------------------------------------------------------------------------------------------
  private:
    // MCU Pines Selection
	int _rstPin;				///< MCU pin pripojený na reset Si4703.
	int _sdioPin;				///< MCU pin pre I2C SDA (SDIO).
	int _sclkPin;				///< MCU pin pre I2C SCL (SCLK).
	int _intPin;				///< MCU pin pre STC/RDS interrupt.

	// Band Settings
	int _band;					///< Kód zvoleného pásma.
  	int _space;					///< Kód rozstupu kanálov.
  	int _de;					///< De-emfáza (50/75 μs).
	int	_bandStart;				///< Spodná hranica pásma (MHz).
	int	_bandEnd;				///< Horná hranica pásma (MHz).
	int	_bandSpacing;			///< Rozstup pásma (kHz/MHz).

	// RDS Settings
	// (miesto pre budúce nastavenia RDS, ak budú doplnené)

	// Tune Settings
	// (miesto pre vlastné parametre ladenia)

	// Seek Settings
	int _skmode;				///< Režim seekovania (wrap/stop).
	int _seekth;				///< RSSI prah pre seek.
	int _skcnt;					///< Prah impulznej detekcie pri seeku.
	int _sksnr;					///< SNR prah pre seek.
	int _agcd;					///< AGC disable (0/1).

	// Private Functions

	/// Načíta registre čipu do „shadow“ štruktúry.
	void	getShadow();		
	/// Zapíše obsah „shadow“ štruktúry späť do registrov čipu.
	byte 	putShadow();		
	/// Inicializuje 3-wire rozhranie (SCLK, SEN, SDIO).
	void	bus3Wire(void);		
	/// Inicializuje 2-wire (I2C) rozhranie (SCLCK, SDIO).
	void	bus2Wire(void);		
	/// Nastaví región (pásmo, rozstup a de-emfázu).
	void	setRegion(int band,	// Band Range
					  int space,// Band Spacing
					  int de);	// De-Emphasis
	/// Zistí STC stav (Seek/Tune Complete).
	bool	getSTC(void);		
	/// Interná pomocná funkcia na vykonanie seeku v smere seekDir.
	int 	seek(byte seekDir);	

	// I2C interface
	/// I2C adresa čipu Si4703 (7-bitová).
	static const int  		I2C_ADDR		= 0x10;
	/// Maximálny počet pokusov o komunikáciu pred zlyhaním.
	static const uint16_t  	I2C_FAIL_MAX 	= 10; 	

	/// Konštanta pre seek smerom nadol.
	static const uint16_t  	SEEK_DOWN 		= 0; 	
	/// Konštanta pre seek smerom nahor.
	static const uint16_t  	SEEK_UP 		= 1;

	// Registers shadow
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra DEVICEID (0x00) – identifikácia zariadenia.
	 */
	union DEVICEID_t
	{
		uint16_t 	word;   ///< Celý 16-bitový obsah registra.

		struct bits
		{
			uint16_t	MFGID	:12;	///< Manufacturer ID.
			uint8_t 	PN		:4;		///< Part Number.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra CHIPID (0x01) – verzia čipu a FW.
	 */
	union CHIPID_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	FIRMWARE:6;		///< Verzia firmvéru.
			uint16_t 	DEV		:4;		///< Typ zariadenia.
			uint16_t 	REV		:6;		///< Revizia čipu.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra POWERCFG (0x02) – napájanie a základné režimy.
	 */
	union POWERCFG_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	ENABLE	:1;		///< Povolenie power-up.
			uint16_t			:5;		///< Rezervované.
			uint16_t	DISABLE :1;		///< Power-down.
			uint16_t			:1;		///< Rezervované.
			uint16_t	SEEK	:1;		///< Spustenie seeku.
			uint16_t	SEEKUP	:1;		///< Smer seeku (0=down,1=up).
			uint16_t	SKMODE	:1;		///< Režim seeku (wrap/stop).
			uint16_t	RDSM	:1;		///< RDS režim (štandard/verbose).
			uint16_t			:1;		///< Rezervované.
			uint16_t	MONO	:1;		///< Mono select (stereo/mono).
			uint16_t	DMUTE	:1;		///< Mute disable (audio on/off).
			uint16_t	DSMUTE	:1;		///< Softmute disable.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra CHANNEL (0x03) – voľba kanálu a ladenie.
	 */
	union CHANNEL_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	CHAN	:10;	///< Číslo kanálu.
			uint16_t			:5;		///< Rezervované.
			uint16_t	TUNE	:1;		///< Spustiť ladenie (1 = tune).
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra SYSCONFIG1 (0x04) – GPIO a RDS nastavenia.
	 */
	union SYSCONFIG1_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	GPIO1	:2;		///< Režim pinu GPIO1.
			uint16_t	GPIO2	:2;		///< Režim pinu GPIO2.
			uint16_t	GPIO3	:2;		///< Režim pinu GPIO3.
			uint16_t	BLNDADJ	:2;		///< Nastavenie stereo/mono blend level.
			uint16_t			:2;		///< Rezervované.
			uint16_t	AGCD	:1;		///< AGC disable.
			uint16_t	DE		:1;		///< De-emphasis (75/50 μs).
			uint16_t	RDS		:1;		///< Zapnutie/vypnutie RDS.
			uint16_t			:1;		///< Rezervované.
			uint16_t	STCIEN 	:1;		///< Interrupt pri STC.
			uint16_t	RDSIEN	:1;		///< Interrupt pri RDS.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra SYSCONFIG2 (0x05) – hlasitosť, pásmo, seek prah.
	 */
	union SYSCONFIG2_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	VOLUME	:4;		///< Hlasitosť 0–15.
			uint16_t	SPACE	:2;		///< Rozstup kanálov.
			uint16_t	BAND	:2;		///< Pásmo (US/EU/JP).
			uint16_t	SEEKTH	:8;		///< RSSI seek prah.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra SYSCONFIG3 (0x06) – seek prahy, softmute, VOLEXT.
	 */
	union SYSCONFIG3_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	SKCNT	:4;		///< Prah impulznej detekcie pri seeku.
			uint16_t	SKSNR	:4;		///< SNR prah pre seek.
			uint16_t	VOLEXT	:1;		///< Rozšírený rozsah hlasitosti.
			uint16_t			:3;		///< Rezervované.
			uint16_t	SMUTEA	:2;		///< Softmute útlm.
			uint16_t	SMUTER	:2;		///< Softmute attack/recover rate.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra TEST1 (0x07) – testovacie a oscilátor nastavenia.
	 */
	union TEST1_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t			:14;	///< Rezervované.
			uint16_t	AHIZEN	:1;		///< Audio High-Z enable.
			uint16_t	XOSCEN	:1;		///< Crystal Oscillator enable.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra TEST2 (0x08) – rezervovaný.
	 */
	union TEST2_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t			:16;	///< Rezervované.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra BOOTCONFIG (0x09) – rezervovaný.
	 */
	union BOOTCONFIG_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t			:16;	///< Rezervované.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra STATUSRSSI (0x0A) – stav a RSSI.
	 */
	union STATUSRSSI_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RSSI 	:8;		///< RSSI (Received Signal Strength Indicator).
			uint16_t	ST		:1;		///< Stereo indikácia.
			uint16_t	BLERA 	:2;		///< RDS Block A errors.
			uint16_t	RDSS 	:1;		///< RDS synchronizácia.
			uint16_t	AFCRL	:1;		///< AFC rail indikácia.
			uint16_t	SFBL 	:1;		///< Seek fail/Band limit.
			uint16_t	STC 	:1;		///< Seek/Tune complete.
			uint16_t	RDSR 	:1;		///< RDS ready.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra READCHAN (0x0B) – naladený kanál a RDS chyby.
	 */
	union READCHAN_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	READCHAN :10;	///< Číslo naladeného kanálu.
			uint16_t	BLERD 	:2;		///< RDS Block D errors.
			uint16_t	BLERC 	:2;		///< RDS Block C errors.
			uint16_t	BLERB	:2;		///< RDS Block B errors.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra RDSA (0x0C) – RDS blok A.
	 */
	union RDSA_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSA 	:16;	///< RDS blok A.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra RDSB (0x0D) – RDS blok B.
	 */
	union RDSB_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSB 	:16;	///< RDS blok B.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra RDSC (0x0E) – RDS blok C.
	 */
	union RDSC_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSC 	:16;	///< RDS blok C.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief Reprezentácia registra RDSD (0x0F) – RDS blok D.
	 */
	union RDSD_t
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSD 	:16;	///< RDS blok D.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	/**
	 * @brief „Shadow“ štruktúra pre všetky registre Si4703.
	 *
	 * Umožňuje:
	 *  - pristupovať k registrom ako k poľu word[16],
	 *  - alebo cez pomenované union štruktúry (STATUSRSSI, SYSCONFIG atď.).
	 */
	union shadow_t
	{
		uint16_t	word[16];			///< 32 bajtov = 16×16-bit registre.

		struct reg
		{
			STATUSRSSI_t	STATUSRSSI;	// Register 0x0A - 00
			READCHAN_t		READCHAN;	// Register 0x0B - 01
			RDSA_t			RDSA;		// Register 0x0C - 02
			RDSB_t			RDSB;		// Register 0x0D - 03
			RDSC_t			RDSC;		// Register 0x0E - 04
			RDSD_t			RDSD;		// Register 0x0F - 05
			// ----------------------------------------------
			DEVICEID_t 		DEVICEID;	// Register 0x00 - 06
			CHIPID_t		CHIPID;		// Register 0x01 - 07
			POWERCFG_t 		POWERCFG;	// Register 0x02 - 08
			CHANNEL_t		CHANNEL;	// Register 0x03 - 09
			SYSCONFIG1_t	SYSCONFIG1;	// Register 0x04 - 10
			SYSCONFIG2_t	SYSCONFIG2;	// Register 0x05 - 11
			SYSCONFIG3_t	SYSCONFIG3;	// Register 0x06 - 12
			TEST1_t			TEST1;		// Register 0x07 - 13
			TEST2_t			TEST2;		// Register 0x08 - 14
			BOOTCONFIG_t	BOOTCONFIG;	// Register 0x09 - 15
		} 			reg;
	} shadow;							// Kópia všetkých 16 registrov po 16 bitoch;
};


#endif
