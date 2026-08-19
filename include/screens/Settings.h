#pragma once

#include "books/BookSync.h"
#include "navigation/Page.h"

class SettingsPage : public Page
{
public:
    void onEnter() override;
    void onExit() override;
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    bool handleConnectivityStateChange(
        uint8_t batteryPercent,
        bool wifiStateChanged,
        bool portalStateChanged
    ) override;

private:
    enum class State : uint8_t
    {
        Overview,
        NetworkList,
        NetworkActions,
        ForgetConfirmation,
        BookServer,
        SetupInstructions,
        CheckingBookServer
    };

    static constexpr uint8_t MAX_NETWORK_ITEMS = 7;
    State state = State::Overview;
    State setupReturnState = State::Overview;
    String selectedNetwork;
    uint8_t overviewSelection = 0;
    uint8_t networkSelection = 0;
    uint8_t optionSelection = 0;
    uint8_t batteryPercent = 0;

    uint8_t getNetworkItems(const char** items) const;
    uint8_t getBookServerActions(const char** items) const;
    String getWifiStatus() const;
    String getBookServerStatus() const;
    String getBookServerHost() const;
    String getForgetMessage() const;
    String getSetupInstructions() const;
    void openSetup(State returnState);
    void checkBookServer();
    void drawBody();
    void drawCurrentContent();
    void redrawCurrentSelection(uint8_t previousIndex);
    void enterState(State nextState);
};
