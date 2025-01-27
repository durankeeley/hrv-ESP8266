# Wi-Fi Enabled Heat Recovery Ventilator (HRV) Controller

This project replaces the stock control panel of a Heat Recovery Ventilator (HRV) system with a Wi-Fi–enabled ESP8266 module. It continuously exchanges stale indoor air with fresh outdoor air, providing a healthier, cleaner, and more comfortable home environment. By integrating MQTT for monitoring and controlling fan speed, you can remotely observe roof temperature, set target fan speeds, and more.

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Hardware Setup](#hardware-setup)
4. [Software Requirements](#software-requirements)
5. [Project Structure](#project-structure)
6. [Configuration](#configuration)
7. [Usage](#usage)
8. [Credits](#credits)
9. [License](#license)

---

## Overview

A Heat Recovery Ventilator (HRV) helps maintain fresh indoor air. This project demonstrates how to replace the HRV's existing control panel with an ESP8266-based controller. The controller communicates with the HRV via a single data line (half-duplex), reads the temperature from the HRV sensors, and publishes the data to an MQTT broker. You can also send commands through MQTT to change the fan speed.

---

## Features

- **ESP8266 Wi-Fi connectivity** for easy remote access and integration with home automation systems.  
- **MQTT integration** to publish real-time data and receive commands (fan speed).  
- **Monitor roof temperature** and control the HRV fan speed automatically or via MQTT.  
- **Automatic device reset** if abnormal temperature readings are detected.  
- **Retains last known fan speed** in case of power loss or device reboot.  

---

## Hardware Setup

### HRV Wiring

The HRV unit in the roof exposes six pins, of which the following are used:

| Pin | Function        |
|----:|-----------------|
|  1  | 5V VCC          |
|  2  | GND             |
|  3  | Data (Rx/Tx)    |
|  4  | Data (Unused)   |
|  5  | GND (Unused)    |
|  6  | 5V VCC (Unused) |

For our purposes, we only need three connections:

1. **Pin 1 → 5V** on the ESP8266  
2. **Pin 2 → GND** on the ESP8266  
3. **Pin 3 → GPIO D6** on the ESP8266 (data line)

> **Note**: We use a single wire for both TX and RX (half-duplex) on `GPIO D6` of the ESP8266.

### ESP8266

This code is designed for a typical **ESP8266 NodeMCU** or **Wemos D1 mini** (or similarly compatible boards). Make sure you:

- Power the ESP8266 with **5V** on `Vin` (the onboard regulator will step down to 3.3V).
- Connect GND from HRV to GND on the ESP8266.
- Connect the HRV data line to the **D6** pin.

---

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/) (VSCode)  
- ESP8266 board support in Arduino IDE or PlatformIO  
- Required libraries:
  - [PubSubClient](https://github.com/knolleary/pubsubclient) (MQTT)
  - [SoftwareSerial](https://www.arduino.cc/en/Reference/softwareSerial)  
  - (Included in Arduino's core or install separately if needed)

---

## Project Structure

```
.
├── constants.h
├── hrv.ino             
├── mock_data.cpp       
├── mock_data.h
├── mqtt_manager.cpp    
├── mqtt_manager.h
├── serial_manager.cpp  
├── serial_manager.h
├── secrets.cpp         
├── secrets.h           
├── utils.cpp           
├── utils.h
├── wifi_manager.cpp    
├── wifi_manager.h
└── README.md
```

### Notable Files

- **`hrv.ino`**: Contains the setup and main loop for the ESP8266. Handles reading data, sending MQTT messages, reconnecting to Wi-Fi, etc.  
- **`mqtt_manager.h`** & **`mqtt_manager.cpp`**: Manages MQTT connection, subscriptions, and message publishing.  
- **`secrets.h` & `secrets.cpp`**: Store your Wi-Fi and MQTT credentials (excluded from version control for security).  
- **`serial_manager.h`** & **`serial_manager.cpp`**: Contains functions for reading and writing data over the SoftwareSerial interface.  
- **`utils.h` & `utils.cpp`**: Utility functions for data conversion and other helpers.
- **`secrets.h` & `secrets.cpp`**: Store your Wi-Fi and MQTT credentials (excluded from version control for security).  

---

## Configuration

1. **Install Libraries**: Make sure [PubSubClient](https://github.com/knolleary/pubsubclient) and [SoftwareSerial](https://www.arduino.cc/en/Reference/softwareSerial) are installed.  
2. **Update `secrets.h`/`secrets.cpp`**:
   ```cpp
   // secrets.h
   extern const char* ssid;
   extern const char* password;
   extern const char* mqtt_broker;
   extern const char* mqtt_username;
   extern const char* mqtt_password;

   // secrets.cpp
   #include "secrets.h"

   const char* ssid         = "YOUR_WIFI_SSID";
   const char* password     = "YOUR_WIFI_PASSWORD";
   const char* mqtt_broker  = "YOUR_MQTT_BROKER_IP";
   const char* mqtt_username= "YOUR_MQTT_USER";
   const char* mqtt_password= "YOUR_MQTT_PASS";
   ```
3. **Pin Configuration**: By default, the code uses `D6` for the HRV data line. If you want to change this pin, update the definition in the code:
   ```cpp
   #define D6 (12) // ESP8266 GPIO pin number for data line
   ```

---

## Usage

1. **Connect Hardware**: Wire the HRV to the ESP8266 as described above.  
2. **Compile and Upload**: Use the Arduino IDE or PlatformIO to compile and upload the code to your ESP8266.  
3. **Check Serial Monitor** (optional): If debug flags are set to `true`, you will see debug messages in the Serial Monitor at **115200 baud**.  
4. **MQTT Topics**:
   - **Publish** fan speed to `hrv/targetfanspeed` (e.g. send `'2'` or `'3'`).  
   - **Subscribe** to `hrv/rooftemp` for continuous updates of the roof temperature.  
   - **Subscribe** to `hrv/currentfanspeed` to see the current fan speed as an integer.  
   - **Subscribe** to `hrv/fanspeedbyte` to see the fan speed reported in hex, mainly for debugging.  

   You can use any MQTT client (like [MQTT Explorer](http://mqtt-explorer.com/) or Node-RED) to monitor and control these topics.

---

## Credits

- **Spencer** for the [data structure reference](http://www.hexperiments.com/?page_id=47)  
- **chimera** for the [original logic](https://www.geekzone.co.nz/forums.asp?forumid=141&topicid=195424)  
- **millst** for the [TX/RX single pin (half-duplex) technique](https://www.geekzone.co.nz/forums.asp?forumid=141&topicid=195424&page_no=2#2982537)  

---

## License

This project is provided “as is” without any guarantees. You may use it or modify it for personal or commercial use.

---

**Enjoy your remotely monitored and controlled Heat Recovery Ventilator!** If you encounter any issues, feel free to open an [issue](../../issues) on this repository.  
