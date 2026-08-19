#pragma once

#include <Arduino.h>

#include "Input.h"

class CenteredOptionPage
{
public:
    void draw(
        const char* heading,
        const char* firstLine,
        const char* secondLine,
        const char* const options[],
        uint8_t optionCount,
        uint8_t batteryPercent
    );

    bool handleInput(
        const InputState& input,
        const char* firstLine,
        const char* secondLine,
        const char* const options[],
        uint8_t optionCount
    );

    uint8_t selectedIndex() const;

private:
    uint8_t selection = 0;
};
