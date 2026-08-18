#pragma once

#include "navigation/Page.h"

class WifiSetupPage : public Page
{
public:
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    void onEnter() override;
    void onExit() override;

private:
    bool stopPortalOnExit = false;
};
