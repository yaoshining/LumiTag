# LumiTag

> A low-power NB-IoT location tag with an E-Ink display, designed for child safety and everyday peace of mind.

[中文说明](./README.zh-CN.md)

## Overview

**LumiTag** is an open-source portable location tag built with **ESP32**, **NB-IoT**, **GNSS**, and a **1.54-inch E-Ink display**.

The project started as a small personal hardware experiment: building a simple, reliable, and low-power location device that can be carried by a child, placed in a school bag, or attached to everyday belongings.

Instead of building a phone-like smart device, LumiTag focuses on one thing:

> Showing simple safety status and periodically reporting location with minimal power consumption.

## Why LumiTag?

Most GPS trackers are either too bulky, too power-hungry, too closed, or too dependent on mobile apps.

LumiTag explores a different direction:

* Wi-Fi-first prototyping, with NB-IoT reserved for later low-power wide-area connectivity
* Low-power NB-IoT communication
* Simple GNSS location reporting
* Always-readable E-Ink status display
* Minimal UI instead of complex interaction
* Open hardware and open firmware direction
* Designed as a tag, badge, or small pendant-like device

The goal is not to build a smartwatch.
The goal is to build a quiet, reliable safety tag.

## Core Features

* ESP32-based firmware
* Wi-Fi connectivity for prototyping
* NB-IoT cellular connectivity
* GNSS positioning
* 1.54-inch SPI E-Ink display
* Deep sleep power-saving mode
* Periodic location upload
* Minimal status UI
* Prototype-friendly hardware structure
* Designed for future custom PCB and enclosure

## Prototype Hardware

Current prototype hardware:

| Part        | Description                                    |
| ----------- | ---------------------------------------------- |
| Main board  | TinkerNode NB-IoT development board            |
| MCU         | ESP32                                          |
| Cellular    | NB-IoT                                         |
| Positioning | GNSS / GPS / Beidou                            |
| Display     | 1.54-inch SPI E-Ink display                    |
| Battery     | Li-ion / LiPo battery, 500–1000mAh recommended |
| Power       | USB / battery powered                          |
| Enclosure   | 3D-printed or custom shell, planned            |

## Display

The E-Ink display is used for simple status feedback.

Example UI:

```text
LumiTag

SAFE

School

Battery 82%

Updated 09:42
```

The display is not intended for maps or complex interaction.
It should show only essential information:

* Safety status
* Current area
* Battery level
* Signal status
* Last update time

## Firmware Direction

The first version is planned to use:

* PlatformIO
* Arduino framework for ESP32
* Modular C++ structure

Suggested firmware structure:

```text
firmware/
├── platformio.ini
├── src/
│   ├── main.cpp
│   ├── Display.cpp
│   ├── Display.h
│   ├── Location.cpp
│   ├── Location.h
│   ├── Cellular.cpp
│   ├── Cellular.h
│   ├── Power.cpp
│   └── Power.h
└── include/
```

Arduino is used in the early prototype stage because it provides mature libraries for ESP32, E-Ink displays, GNSS parsing, and basic networking.

A future version may migrate selected parts to ESP-IDF for better power management and production-level control.

## Hardware Connection

The E-Ink display is connected through SPI.

Typical pins:

| E-Ink Pin | ESP32 Function |
| --------- | -------------- |
| VCC       | 3.3V           |
| GND       | GND            |
| DIN       | MOSI           |
| CLK       | SCK            |
| CS        | GPIO           |
| DC        | GPIO           |
| RST       | GPIO           |
| BUSY      | GPIO           |

Actual GPIO mapping may vary depending on the board and occupied pins.

## Power Strategy

LumiTag should avoid running all modules continuously.

A typical cycle:

```text
Wake up
→ Get location
→ Upload location via NB-IoT
→ Refresh E-Ink display if needed
→ Enter deep sleep
```

The device should prioritize:

* Low wake frequency
* Short GNSS active time
* Batched or periodic NB-IoT upload
* Minimal display refresh
* Aggressive deep sleep

## Product Form

LumiTag is designed as a small tag-like device rather than a smartwatch.

Possible forms:

* School bag tag
* Safety badge
* Small locator pendant
* Asset tracker
* Elderly safety tag
* Pet or luggage tracker

The first prototype is expected to be larger than the final product.
A custom PCB and enclosure will be needed for a polished product form.

## Roadmap

### v0.1 Prototype

* [ ] Confirm hardware wiring
* [ ] Light up E-Ink display
* [ ] Show static UI
* [ ] Read GNSS location
* [ ] Send data through NB-IoT
* [ ] Enter deep sleep

### v0.2 Functional Device

* [ ] Battery level monitoring
* [ ] Periodic location upload
* [ ] Simple cloud endpoint
* [ ] Basic geofence status
* [ ] Improved E-Ink UI

### v0.3 Product Prototype

* [ ] 3D-printed enclosure
* [ ] Optimized wiring
* [ ] Real battery test
* [ ] Outdoor field test
* [ ] Child-friendly form factor

### v1.0

* [ ] Custom PCB
* [ ] Compact enclosure
* [ ] Better antenna placement
* [ ] Waterproof design
* [ ] Open-source hardware files

## Project Status

LumiTag is currently in the early prototype stage.

The current focus is:

1. Hardware wiring
2. Wi-Fi connectivity and cloud API development
3. E-Ink display bring-up
4. GNSS location acquisition
5. NB-IoT communication after the SIM is restored
6. Power consumption testing

## Current Network Strategy

Until the NB-IoT SIM is restored, the prototype uses ESP32 2.4 GHz Wi-Fi to develop local connectivity and cloud flows. Alibaba Cloud IoT Platform now contains the `LumiTag` product and `LumiTag_ESP32_01` prototype device. `firmware/LumiTagWiFi` provides an interactive MQTT test that keeps credentials out of flash; see the [firmware notes](firmware/README.md).

The physical prototype has verified DNS resolution, TCP port `8883`, server-certificate validation, TLS MQTT connection, `user/update` telemetry publishing, and Alibaba Cloud IoT runtime-log storage in SLS. This acceptance is limited to the Wi-Fi prototype path; it does not validate NB-IoT, GNSS, E-Ink, deep sleep, or battery life.

## License

This project is planned to be released as open source.

Suggested licenses:

* Firmware: MIT License
* Hardware: CERN Open Hardware License or Creative Commons
* Documentation: CC BY 4.0
