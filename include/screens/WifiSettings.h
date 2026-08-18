#pragma once

#include "navigation/Page.h"
#include "wifi/SelectedWifiNetwork.h"

class WifiSettingsPage : public Page
{
public:
    explicit WifiSettingsPage(SelectedWifiNetwork& selectedNetwork);
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    bool handleConnectivityStateChange(
        uint8_t batteryPercent,
        bool wifiStateChanged,
        bool portalStateChanged
    ) override;

private:
    static constexpr uint8_t MAX_ITEMS = 6;
    SelectedWifiNetwork& selectedNetwork;
    uint8_t selectedIndex = 0;
    uint8_t batteryPercent = 0;

    uint8_t getItems(const char** items) const;
    String getHeading() const;
    void redrawSelection(uint8_t previousIndex);
    void redrawContent();
};
