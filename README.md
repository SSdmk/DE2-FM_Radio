# FM Radio Receiver with Digital Tuning

This project is a digital FM receiver built around a microcontroller (MCU) and a specialized Si4703 tuner module. It allows for precise digital tuning, station seeking, and displays frequency and station information (RDS) on a display.

## Our Team
* Lukáš Vizina
* Rastislav Štefan Sokol
* Martin Šveda
* Samuel Sedmák

## 1. Problem Statement & Solution

### Problem Statement
Traditional analog FM radios often rely on manual tuning using a variable capacitor, which is imprecise and cumbersome. The user cannot see the exact frequency they are tuned to, and modern features like automatic station seeking or displaying the station name (RDS) are missing.

### Proposed Solution (with MCU)
Our solution uses a microcontroller (e.g., Arduino Uno) as the control unit, which communicates digitally with the FM tuner module (Si4703) via the I2C bus.
1.  **MCU (The Brain):** Receives inputs from the buttons (Tune Up, Tune Down, Seek).
2.  **Tuner Control:** The MCU sends commands directly to the Si4703 module (e.g., "tune to 94.8 MHz" or "find the next strong station").
3.  **Data Display:** The MCU reads the current status from the tuner (frequency, signal strength) and any available RDS data (station name). It then displays this information in real-time on an OLED display.

The result is a compact, accurate, and user-friendly FM receiver.

---

## 2. Hardware Components

A list of the main components required for the build.

| Component | Quantity | Justification |
| :--- | :---: | :--- |
| **MCU: Arduino Uno** | 1 pc | The heart of the project. It processes inputs from the buttons, controls the tuner and the display via the I2C bus. It is small and has enough pins. |
| **FM Tuner: Si4703 Breakout Board**| 1 pc | A key component. It is a complete FM receiver on a chip. It is controlled digitally (I2C), which allows for precise tuning, and most importantly, it **supports RDS**. |
| **Display: OLED 0.96" (SSD1306)** | 1 pc | Displays the frequency, station name (RDS), and signal strength. Chosen for its high contrast, low power consumption, and I2C interface (saves MCU pins). |
| **Tactile Buttons** | 3 pcs | Simple input for the user. They are used for: Tune Up, Tune Down, and Automatic Seek. |
| **3.5mm Audio Jack** | 1 pc | Provides a standard output for headphones or external speakers. The audio signal comes directly from the Si4703 module. |
| **Breadboard and Jumper Wires** | 1 set | For interconnecting the prototype. |

---

## 3. Software Design

This section describes the program logic and flow.

### System Block Diagram
The diagram illustrates the flow of signals and data between components.
