# FM Rádio Prijímač s Digitálnym Ladením

Tento projekt je digitálny FM prijímač postavený na báze mikrokontroléra (MCU) a špecializovaného tuner modulu Si4703. Umožňuje presné digitálne ladenie, hľadanie staníc a zobrazenie informácií o frekvencii a stanici (RDS) na displeji.



## 1. Problém a Návrh Riešenia (Problem Statement & Solution)

### Popis Problému
Tradičné analógové FM rádiá sa často spoliehajú na manuálne ladenie pomocou otočného kondenzátora, čo je nepresné a ťažkopádne. Užívateľ nevidí presnú frekvenciu, na ktorej sa nachádza, a chýbajú moderné funkcie ako automatické hľadanie staníc alebo zobrazenie názvu stanice (RDS).

### Navrhované Riešenie (s MCU)
Naše riešenie využíva mikrokontrolér (napr. Arduino Nano) ako riadiacu jednotku, ktorá digitálne komunikuje s FM tuner modulom (Si4703) prostredníctvom I2C zbernice.
1.  **MCU (Mozog):** Prijíma vstupy z tlačidiel (Ladiť Hore, Ladiť Dole, Hľadať).
2.  **Ovládanie Tunera:** MCU posiela príkazy priamo modulu Si4703 (napr. "nalaď frekvenciu 94.8 MHz" alebo "nájdi ďalšiu silnú stanicu").
3.  **Zobrazenie Dát:** MCU číta aktuálny stav z tunera (frekvenciu, silu signálu) a prípadné RDS dáta (názov stanice). Tieto informácie potom zobrazuje v reálnom čase na OLED displeji.

Výsledkom je kompaktný, presný a užívateľsky prívetivý FM prijímač.

---

## 2. Hardvérové Komponenty (Hardware Components)

Zoznam hlavných komponentov potrebných na stavbu.

| Komponent | Počet | Dôvod použitia (Justification) |
| :--- | :---: | :--- |
| **MCU: Arduino Uno** | 1 ks | Srdce projektu. Spracováva vstupy z tlačidiel, riadi tuner a displej cez I2C zbernicu. Je malé a má dostatok pinov. |
| **FM Tuner: Si4703 Breakout Board**| 1 ks | Kľúčový komponent. Je to kompletný FM prijímač na čipe. Ovláda sa digitálne (I2C), čo umožňuje presné ladenie, a hlavne **podporuje RDS**. |
| **Displej: OLED 0.96" (SSD1306)** | 1 ks | Zobrazuje frekvenciu, názov stanice (RDS) a silu signálu. Zvolený kvôli vysokému kontrastu, nízkej spotrebe a I2C rozhraniu (šetrí piny MCU). |
| **Taktilné tlačidlá** | 3 ks | Jednoduchý vstup pre užívateľa. Slúžia na: Ladiť Hore, Ladiť Dole a Automatické Hľadanie (Seek). |
| **3.5mm Audio Jack (slúchadlový)** | 1 ks | Poskytuje štandardný výstup pre slúchadlá alebo externé reproduktory. Audio signál ide priamo z modulu Si4703. |
| **Kontaktné pole a káble** | 1 sada| Na prepojenie prototypu. |

---

## 3. Návrh Softvéru (Software Design)

Táto sekcia popisuje logiku a tok programu.

### Blokový Diagram Systému
Diagram znázorňuje tok signálov a dát medzi komponentmi.
