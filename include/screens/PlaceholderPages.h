#pragma once

#include "navigation/Page.h"

class PlaceholderPage : public Page
{
public:
    PlaceholderPage(const char* header, const char* message);

    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;

private:
    const char* header;
    const char* message;
};

