#include "Input.h"

#include "BoardConfig.h"

namespace
{
    constexpr unsigned long DEBOUNCE_MS = 30;

    struct Button
    {
        uint8_t pin;
        bool stableState;
        bool lastReading;
        unsigned long lastChangeTime;
    };

    Button previousButton = {
        BoardConfig::PREVIOUS_BUTTON_PIN,
        HIGH,
        HIGH,
        0
    };

    Button nextButton = {
        BoardConfig::NEXT_BUTTON_PIN,
        HIGH,
        HIGH,
        0
    };

    Button selectButton = {
        BoardConfig::SELECT_BUTTON_PIN,
        HIGH,
        HIGH,
        0
    };

    bool wasPressed(Button& button)
    {
        const bool reading = digitalRead(button.pin);
        const unsigned long now = millis();

        if (reading != button.lastReading)
        {
            button.lastReading = reading;
            button.lastChangeTime = now;
        }

        if (
            now - button.lastChangeTime >= DEBOUNCE_MS &&
            reading != button.stableState
        ) {
            button.stableState = reading;
            return button.stableState == LOW;
        }

        return false;
    }
}

void initInput()
{
    pinMode(BoardConfig::PREVIOUS_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BoardConfig::NEXT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BoardConfig::SELECT_BUTTON_PIN, INPUT_PULLUP);
}

InputState readInput()
{
    return {
        wasPressed(previousButton),
        wasPressed(nextButton),
        wasPressed(selectButton)
    };
}
