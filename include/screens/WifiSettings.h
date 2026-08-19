#pragma once

#include "navigation/Page.h"

class WifiSettingsPage : public Page
{
public:
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    void onExit() override;
    bool handleConnectivityStateChange(
        uint8_t batteryPercent,
        bool wifiStateChanged,
        bool portalStateChanged
    ) override;

private:
    enum class State : uint8_t
    {
        NetworkList,
        NetworkActions,
        ForgetConfirmation,
        SetupInstructions
    };

    static constexpr uint8_t MAX_ITEMS = 7;
    String selectedNetwork;
    State state = State::NetworkList;
    uint8_t networkSelection = 0;
    uint8_t optionSelection = 0;
    uint8_t batteryPercent = 0;

    uint8_t getItems(const char** items) const;
    String getConnectionStatus() const;
    String getForgetMessage() const;
    String getSetupInstructions() const;
    void drawContent();
    void drawCurrentContent();
    void redrawCurrentSelection(uint8_t previousIndex);
    void enterState(State nextState);
};
