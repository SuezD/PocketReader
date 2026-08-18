# Pocket Reader

Pocket e-reader built around a Seeed Studio XIAO ESP32-S3.

## Hardware
- Waveshare 4.2" e-paper reader display (Rev 2.2)
    - This is about as small as a post-it note and fits comfortably in your open palm! 
- Seeed Studio XIAO ESP32-S3
- Buttons (3 for next/prev and select)

## Software
- cpp with Arduino framework
- PlatformIO
- GxEPD2 library for e-paper display
- Adafruit GFX fonts and drawing utils

The interface will be split into reusable components with some helper files. Entrypoint for the project is main.cpp

## Build
I use PlatformIO as a VSCode extension but this is not a requirement.
Open the project in PlatformIO, connect the XIAO ESP32-S3, then build and upload:
```bash
pio run -e xiao_esp32s3 --target upload
pio run -e xiao_esp32s3 --target uploadfs
```
## Design Demo
<img width="298" height="398" alt="ereaderdesigndemo2-ezgif com-optimize" src="https://github.com/user-attachments/assets/ba755fc4-efe9-4ba1-9f43-8a12ffcae380" />
