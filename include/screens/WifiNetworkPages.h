#pragma once

#include "navigation/Page.h"
#include "components/CenteredOptionPage.h"
#include "wifi/SelectedWifiNetwork.h"

class WifiNetworkActionsPage : public Page
{
public:
    explicit WifiNetworkActionsPage(SelectedWifiNetwork& selectedNetwork);
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    SelectedWifiNetwork& selectedNetwork;
    CenteredOptionPage optionsPage;
};

class WifiForgetNetworkPage : public Page
{
public:
    explicit WifiForgetNetworkPage(SelectedWifiNetwork& selectedNetwork);
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    SelectedWifiNetwork& selectedNetwork;
    CenteredOptionPage optionsPage;
};

class WifiSetupPage : public Page
{
public:
    void onEnter() override;
    void onExit() override;
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    CenteredOptionPage optionsPage;
};
