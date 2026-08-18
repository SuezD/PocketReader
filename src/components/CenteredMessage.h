#pragma once

#include <Arduino.h>

void drawMessage(
    const char* firstLine,
    const char* secondLine = nullptr,
    const char* const options[] = nullptr,
    uint8_t optionCount = 0,
    uint8_t selectedIndex = 0
);

void redrawMessageSelection(
    const char* firstLine,
    const char* secondLine,
    const char* const options[],
    uint8_t optionCount,
    uint8_t previousIndex,
    uint8_t selectedIndex
);
