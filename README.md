# Arduino sensor array - Hardware and Software developement

## General Description

This project is a full Hardware and Software implementation of a sensor array built for automated security purposes. The hardware (Schematic and PCB layout) was designed using **KiCad 9.0** and the code was written in **C++** and uploaded via the **platformio** extension for VSCode

With this project's prototype I managed to win the 3rd place at the **FutureShapers Hackathon 2026**, organized by **Honeywell, Politehnica Bucharest, EELISA and Groupe INSA** (Photos and Video attached). It was presented under the name "ASG: Automated Security Guardian"

---

## Architecture & Functional Blocks


1. **Central Processing Unit (Arduino UNO):**
   * Dedicated microcontroller for embedded applications.
   * Integrated high-speed communication protocols such as **I2C** for this project
   * Easily compatible with different programming devices using a USB interface

2. **Input sensors**
   * ambient temperature (DHT11)
   * ambient light sensor (Photoresistor CDS-55) working in a voltage divider configuration
   * ultrasound distance sensor (HC-SR501 Set to a 20cm threshold)
   * button simulating a fire/general emergency alarm and also a shutdown mechanism.

3. **Outputs and interface**
   * MAIN OUTPUT: 2x16 **LCD display**
   * 3 leds: Red for proximity breach and alarm, yellow and green for temperature status
   * Passive Buzzer alarm

4. **Power Management:**
   * The photoresistor acts as a light detector and is programmed to **turn off the backlight** of the LCD if low ambient light is detected

---

## PCB Layout & Design Guidelines
The printed circuit board (PCB) was routed following standard practices such as:

* **2-Layer Stackup Strategy:**
  * **Layer 1a (Top):** Primary component placement, high-speed signals and larger power traces
  * **Layer 1b (Ground Plane):** A solid, continuous ground plane (`GND`) ensuring a low-impedance return path for high-speed signals and effective electromagnetic shielding (EMI).
  * **Layer 4 (Bottom):** Secondary routing layer.

* **Component Placement:** 
  * The length of all copper paths was tuned to ensure minimum impedance (For power traces) and overall cost effectiveness.

---

## Technical Specifications

* **Power Input:** stable +5V via the Arduino's integrated voltage regulator
* **Connectivity & Interfaces:** 
  * I2C communication protocol for the LCD output display
  * Fine control of the servo motor (Which moves the ultrasound sensor to cover ~170deg FOV) via PWM
* **Status Indication:** 3 status LEDs (`LED1 - RED`, `LED2 - YELLOW`, `LED3 - GREEN`).
* **JSON compatibility** for **serial data transfer** between the device and a web server allowing **long distance monitoring** of the system
