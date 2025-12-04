# FM Radio Receiver with Digital Tuning

This project is a digital FM receiver built around a microcontroller (MCU) and a specialized Si4703 tuner module. It allows for precise digital tuning, station seeking, volume control via a rotary encoder, and displays frequency and station RSSI on an OLED screen.

## Our Team
* Lukáš Vizina
* Rastislav Štefan Sokol
* Martin Šveda
* Samuel Sedmák

## 1. Problem Statement & Solution

### Problem Statement
Traditional analog FM radios often rely on manual tuning using a variable capacitor, which is imprecise and cumbersome. The user cannot see the exact frequency they are tuned to, and modern features like automatic station seeking, digital volume control.

### Proposed Solution (with MCU)
Our solution uses a microcontroller (**Arduino Uno**) as the control unit, which communicates digitally with the FM tuner module (Si4703) via the I2C bus.

1.  **User Inputs:** The system processes inputs from **4 tactile buttons** (for seeking, mute, power, and favorites) and a **Rotary Encoder** (for smooth volume control and fine-tuning).
2.  **Tuner Control:** The MCU sends commands directly to the Si4703 module (e.g., "tune to 94.8 MHz" or "find the next strong station").
3.  **Data Display:** The MCU reads the current status from the tuner (frequency, signal strength). It then displays this information in real-time on an OLED display.

The result is a compact, accurate, and user-friendly FM receiver.


## 2. Hardware Components

A list of the main components required for the build.

| Component | Quantity | Justification |
| :--- | :---: | :--- |
| **MCU: Arduino Uno** | 1 pc | The heart of the project. It processes inputs, controls the tuner, and the display via the I2C bus. |
| **FM Tuner: Si4703 Breakout Board**| 1 pc | A key component. It is a complete FM receiver on a chip. It is controlled digitally (I2C). |
| **Rotary Encoder (KY-040)** | 1 pc | Used for intuitive **volume control**. It also includes a push-button to toggle between volume and frequency tuning modes. |
| **Display: OLED 0.96" (SSD1306)** | 1 pc | Displays the frequency, station name, and signal strength. Chosen for its high contrast and I2C interface. |
| **Tactile Buttons** | 4 pcs | Used for specific actions: Seek Left/Right, Mute, Power On/Off, and saving favorite stations (Long Press). |
| **3.5mm Audio Jack** | 1 pc | Provides a standard output for headphones or external speakers. The audio signal comes directly from the Si4703 module. |
| **Breadboard and Jumper Wires** | 1 set | For interconnecting the prototype. |

### Wiring Diagram
Below is the photos of our actual hardware setup and wiring connections.

![DE2_hradware](https://github.com/user-attachments/assets/150adc52-20e1-42d6-8283-6ac59b0839c0)
<br> <img src="https://github.com/user-attachments/assets/0630c92d-c7ba-4f1d-84e8-c9cbde90afc1" width="700">


## 3. Software Logic 

The firmware operates in a continuous polling loop, handling user inputs and updating system states.

* **Initialization:** Configures I2C communication, initializes the OLED display, and sets up the Si4703 tuner.
* **Input Processing:** Detects interactions from buttons and the rotary encoder:
    * *Short vs. Long Press:* Differentiates actions (e.g., Short Up loads a favorite station, Long Up saves it).
    * *Rotary Encoder:* Adjusts Volume or Frequency based on the selected mode (toggled by the encoder button).
    * *Navigation:* Buttons manage Seek (Left/Right), Mute, and Power control.
* **System Update:**
    * Executes the requested action.
    * Reads real-time data from Si4703 (Channel, RSSI, Volume).
    * Refreshes the OLED display with current status.


## 4. User Manual / Controls

This section explains how to control the FM Radio receiver using the buttons and the rotary encoder.

| Component | Action | Function |
| :--- | :--- | :--- |
| **UP Button** | Short Press | **Recall Favorite:** Loads the saved favorite station. |
| | Long Press | **Save Favorite:** Saves the current station as the favorite. |
| **DOWN Button** | Short Press | **Mute:** Mutes or unmutes the audio. |
| | Long Press | **Power:** Turns the radio module On or Off (Standby). |
| **LEFT Button** | Short Press | **Seek Down:** Automatically searches for the nearest lower station. |
| **RIGHT Button** | Short Press | **Seek Up:** Automatically searches for the nearest higher station. |
| **Rotary Encoder** | Rotate | **Adjust Value:** <br>• In *Volume Mode*: Increases/Decreases volume.<br>• In *Freq Mode*: Fine-tunes frequency by steps (manual tuning). |
| | Click (Press) | **Toggle Mode:** Switches the encoder function between Volume and Frequency control. |

### Control Logic Flowchart
Below is the logic diagram showing how user inputs are processed.
<img width="956" height="443" alt="Výkres 3" src="https://github.com/user-attachments/assets/6d446190-c761-4fb4-85e6-d05a0ced022b" />
## 5. Conclusion & Future Improvements

### Conclusion
This project successfully demonstrates the design and implementation of a digitally controlled FM receiver. We managed to integrate the **Si4703 tuner** with an **Arduino Uno** and created a user-friendly interface using an OLED display, tactile buttons, and a rotary encoder. The final device provides audio reception, tuning, and essential features like RDS decoding and station memory, meeting all the initial requirements.

### Future Improvements
Although the project is fully functional, there is room for further enhancements:
* **3D Printed Enclosure:** Designing a custom case to house the components and make the device portable.
* **Battery Operation:** Adding a battery and charging circuit for true portability.
* **Multiple Favorites:** Expanding the software to save a list of favorite stations into the EEPROM memory, not just one.
* **Better Antenna:** Replacing the headphone wire antenna with a dedicated telescopic antenna for better signal reception.

## 6. Video Demonstration
Check out the video recordings of the device in action:

* **[▶️ Video 1:](https://youtube.com/shorts/VkSr56Elnn0)**
* **[▶️ Video 2:](https://youtube.com/shorts/RAxw9ENuTcU)**


