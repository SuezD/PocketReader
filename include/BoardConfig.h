#pragma once

#include <Arduino.h>

namespace BoardConfig
{
    constexpr int EPD_CS = 10;
    constexpr int EPD_DC = 9;
    constexpr int EPD_RST = 8;
    constexpr int EPD_BUSY = 7;

    constexpr uint8_t PREVIOUS_BUTTON_PIN = 2;
    constexpr uint8_t NEXT_BUTTON_PIN = 3;
    constexpr uint8_t SELECT_BUTTON_PIN = 4;
}
