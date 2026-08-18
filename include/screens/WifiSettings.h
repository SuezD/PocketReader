#pragma once

#include "navigation/Page.h"

class WifiSettingsPage : public Page
{
public:
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    bool redrawOnWifiStateChange() const override;

private:
    enum class Action : uint8_t
    {
        StartPortal,
        StopPortal,
        Reconnect,
        ForgetNetwork
    };

    static constexpr uint8_t MAX_ACTION_COUNT = 3;

    uint8_t selectedIndex = 0;
    uint8_t batteryPercent = 0;

    uint8_t getActions(Action* actions, const char** labels) const;
    String getDetails() const;
    const char* getStatusLine() const;
    void redrawSelection(uint8_t previousIndex);
};
