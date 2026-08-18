#pragma once

#include <Arduino.h>

#include "components/SelectList.h"
#include "navigation/Page.h"

class MainMenuPage : public Page
{
public:
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    SelectListState listState = {};
    void redrawSelection(uint8_t previousIndex);
};

