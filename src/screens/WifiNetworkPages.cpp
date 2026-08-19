#include "screens/WifiNetworkPages.h"

#include "wifi/WifiProvisioningService.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr const char* NETWORK_ACTIONS[] = {
        "Connect", "Forget Network", "Back"
    };
    constexpr uint8_t NETWORK_ACTION_COUNT = 3;
    constexpr const char* FORGET_OPTIONS[] = { "Cancel", "Forget" };
    constexpr uint8_t FORGET_OPTION_COUNT = 2;
    constexpr const char* BACK_OPTION[] = { "Back" };

}

WifiNetworkActionsPage::WifiNetworkActionsPage(
    SelectedWifiNetwork& nextSelectedNetwork
) : selectedNetwork(nextSelectedNetwork)
{
}

void WifiNetworkActionsPage::draw(uint8_t batteryPercent)
{
    optionsPage.draw(
        "NETWORK ACTIONS", selectedNetwork.getSsid(),
        nullptr, NETWORK_ACTIONS, NETWORK_ACTION_COUNT, batteryPercent
    );
}

bool WifiNetworkActionsPage::handleInput(const InputState& input)
{
    return optionsPage.handleInput(
        input,
        selectedNetwork.getSsid(), nullptr,
        NETWORK_ACTIONS, NETWORK_ACTION_COUNT
    );
}

NavigationRequest WifiNetworkActionsPage::select()
{
    if (optionsPage.selectedIndex() == 0)
    {
        getWifiManager().connectSavedNetwork(selectedNetwork.getSsid());
        return { NavigationMode::PopTo, PageId::WiFiSettings };
    }
    if (optionsPage.selectedIndex() == 1)
    {
        return { NavigationMode::Push, PageId::WifiForgetNetwork };
    }
    return { NavigationMode::Pop, PageId::WiFiSettings };
}

WifiForgetNetworkPage::WifiForgetNetworkPage(
    SelectedWifiNetwork& nextSelectedNetwork
) : selectedNetwork(nextSelectedNetwork)
{
}

void WifiForgetNetworkPage::draw(uint8_t batteryPercent)
{
    String message = "Forget ";
    message += selectedNetwork.getSsid();
    message += '?';
    optionsPage.draw(
        "FORGET NETWORK", message.c_str(),
        nullptr, FORGET_OPTIONS, FORGET_OPTION_COUNT, batteryPercent
    );
}

bool WifiForgetNetworkPage::handleInput(const InputState& input)
{
    String message = "Forget ";
    message += selectedNetwork.getSsid();
    message += '?';
    return optionsPage.handleInput(
        input,
        message.c_str(), nullptr,
        FORGET_OPTIONS, FORGET_OPTION_COUNT
    );
}

NavigationRequest WifiForgetNetworkPage::select()
{
    if (optionsPage.selectedIndex() == 0)
    {
        return { NavigationMode::Pop, PageId::WifiNetworkActions };
    }

    getWifiManager().forgetSavedNetwork(selectedNetwork.getSsid());
    selectedNetwork.select("");
    return { NavigationMode::PopTo, PageId::WiFiSettings };
}

void WifiSetupPage::onEnter()
{
    getWifiProvisioningPortal().start();
}

void WifiSetupPage::onExit()
{
    getWifiProvisioningPortal().stop();
}

void WifiSetupPage::draw(uint8_t batteryPercent)
{
    WifiManager& wifi = getWifiManager();
    String instructions = "Connect to: ";
    instructions += wifi.getSetupNetworkName();
    instructions += "\nPassword: ";
    instructions += wifi.getSetupNetworkPassword();
    instructions += "\nOpen: ";
    instructions += wifi.getSetupAddress();
    optionsPage.draw(
        "ADD NETWORK", instructions.c_str(),
        nullptr, BACK_OPTION, 1, batteryPercent
    );
}

bool WifiSetupPage::handleInput(const InputState& input)
{
    return input.upPressed || input.downPressed;
}

NavigationRequest WifiSetupPage::select()
{
    return { NavigationMode::Pop, PageId::WiFiSettings };
}
