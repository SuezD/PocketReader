#pragma once

#include <Arduino.h>

#include "Input.h"

void selectPrevious(uint8_t& selectedIndex, uint8_t optionCount);
void selectNext(uint8_t& selectedIndex, uint8_t optionCount);
bool moveSelection(
    const InputState& input,
    uint8_t& selectedIndex,
    uint8_t optionCount
);
