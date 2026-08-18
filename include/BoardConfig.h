#pragma once

#include <Arduino.h>

namespace BoardConfig
{
    // XIAO ESP32-S3: D8 (GPIO 7) and D10 (GPIO 9).
    constexpr int EPD_SCK = 7;
    constexpr int EPD_MOSI = 9;
    constexpr int EPD_CS = 3;   // D2
    constexpr int EPD_DC = 4;   // D3
    constexpr int EPD_RST = 5;  // D4
    constexpr int EPD_BUSY = 6; // D5

    constexpr uint8_t PREVIOUS_BUTTON_PIN = 1; // D0
    constexpr uint8_t NEXT_BUTTON_PIN = 2;     // D1
    constexpr uint8_t SELECT_BUTTON_PIN = 8;   // D9
}
