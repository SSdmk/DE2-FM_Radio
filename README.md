# FM Radio Receiver with Digital Tuning

This project is a digital FM receiver built around a microcontroller (MCU) and a specialized Si4703 tuner module. It allows for precise digital tuning, station seeking, volume control via a rotary encoder, and displays frequency and station information (RDS) on an OLED screen.

## Our Team
* Lukáš Vizina
* Rastislav Štefan Sokol
* Martin Šveda
* Samuel Sedmák

## 1. Problem Statement & Solution

### Problem Statement
Traditional analog FM radios often rely on manual tuning using a variable capacitor, which is imprecise and cumbersome. The user cannot see the exact frequency they are tuned to, and modern features like automatic station seeking, digital volume control, or displaying the station name (RDS) are missing.

### Proposed Solution (with MCU)
Our solution uses a microcontroller (**Arduino Uno**) as the control unit, which communicates digitally with the FM tuner module (Si4703) via the I2C bus.

1.  **User Inputs:** The system processes inputs from **4 tactile buttons** (for seeking, mute, power, and favorites) and a **Rotary Encoder** (for smooth volume control and fine-tuning).
2.  **Tuner Control:** The MCU sends commands directly to the Si4703 module (e.g., "tune to 94.8 MHz" or "find the next strong station").
3.  **Data Display:** The MCU reads the current status from the tuner (frequency, signal strength) and any available RDS data. It then displays this information in real-time on an OLED display.

The result is a compact, accurate, and user-friendly FM receiver.

---

## 2. Hardware Components

A list of the main components required for the build.

| Component | Quantity | Justification |
| :--- | :---: | :--- |
| **MCU: Arduino Uno** | 1 pc | The heart of the project. It processes inputs, controls the tuner, and the display via the I2C bus. |
| **FM Tuner: Si4703 Breakout Board**| 1 pc | A key component. It is a complete FM receiver on a chip. It is controlled digitally (I2C) and **supports RDS**. |
| **Rotary Encoder (KY-040)** | 1 pc | Used for intuitive **volume control**. It also includes a push-button to toggle between volume and frequency tuning modes. |
| **Display: OLED 0.96" (SSD1306)** | 1 pc | Displays the frequency, station name, and signal strength. Chosen for its high contrast and I2C interface. |
| **Tactile Buttons** | 4 pcs | Used for specific actions: Seek Left/Right, Mute, Power On/Off, and saving favorite stations (Long Press). |
| **3.5mm Audio Jack** | 1 pc | Provides a standard output for headphones or external speakers. The audio signal comes directly from the Si4703 module. |
| **Breadboard and Jumper Wires** | 1 set | For interconnecting the prototype. |

---
### Wiring Diagram
![DE2_hradware](https://github.com/user-attachments/assets/150adc52-20e1-42d6-8283-6ac59b0839c0)


## 3. Software Design

This section describes the high-level logic and flow of the program.

### System Block Diagram
The diagram illustrates the flow of signals and data between components.
