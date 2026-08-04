# Pocket Reader

Pocker e-reader being prototyped with an Arduino Uno.
Eventually this will use a smaller dedicated microcontroller (ESP32-S3).

## Hardware
- Waveshare 4.2" e-paper reader display (Rev 2.2)
- Arduino Uno for prototyping
- Buttons (3 for next/prev and select)

## Software
- cpp with Arduino framework
- PlatformIO
- GxEPD2 library for e-paper display
- Adafruit GFX fonts and drawing utils

The interface will be split into reusable components with some helper files. Entrypoint for the project is main.cpp

## Build
I use PlatformIO as a VSCode extension but this is not a requirement.
Open the project in PlatformIO, connect the Arduino, then build and upload the configured environment
```bash
pio run --target upload
```
