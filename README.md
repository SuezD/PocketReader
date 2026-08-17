# Pocket Reader

Pocker e-reader being prototyped with an Arduino Uno.
Eventually this will use a smaller dedicated microcontroller (ESP32-S3).

## Hardware
- Waveshare 4.2" e-paper reader display (Rev 2.2)
    - This is about as small as a post-it note and fits comfortably in your open palm! 
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
## Design Demo
<img width="298" height="398" alt="ereaderdesigndemo2-ezgif com-optimize" src="https://github.com/user-attachments/assets/ba755fc4-efe9-4ba1-9f43-8a12ffcae380" />
