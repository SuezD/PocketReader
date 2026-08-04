#pragma once

#include <Arduino.h>

void drawMessage(
    const char* firstLine,
    const char* secondLine = nullptr,
    const char* const options[] = nullptr,
    uint8_t optionCount = 0,
    uint8_t selectedIndex = 0
);
