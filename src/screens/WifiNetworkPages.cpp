#include "screens/WifiNetworkPages.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
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

    bool moveSelection(
        const InputState& input,
        uint8_t& selectedIndex,
        uint8_t optionCount
    ) {
        const uint8_t previousIndex = selectedIndex;
        if (input.upPressed && !input.downPressed && selectedIndex > 0)
        {
            selectedIndex--;
        }
        else if (
            input.downPressed && !input.upPressed &&
            selectedIndex + 1 < optionCount
        ) {
            selectedIndex++;
        }
        else if (!input.upPressed && !input.downPressed)
        {
            return false;
        }
        return selectedIndex != previousIndex;
    }

    void drawOptionPage(
        const char* header,
        const char* message,
        const char* const options[],
        uint8_t optionCount,
        uint8_t selectedIndex,
        uint8_t batteryPercent
    ) {
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(Theme::BACKGROUND_COLOR);
            drawHeader(header, batteryPercent);
            drawMessage(message, nullptr, options, optionCount, selectedIndex);
            drawFooter();
        }
        while (display.nextPage());
    }
}

WifiNetworkActionsPage::WifiNetworkActionsPage(
    SelectedWifiNetwork& nextSelectedNetwork
) : selectedNetwork(nextSelectedNetwork)
{
}

void WifiNetworkActionsPage::draw(uint8_t batteryPercent)
{
    selectedIndex = 0;
    drawOptionPage(
        "NETWORK ACTIONS", selectedNetwork.getSsid(),
        NETWORK_ACTIONS, NETWORK_ACTION_COUNT,
        selectedIndex, batteryPercent
    );
}

bool WifiNetworkActionsPage::handleInput(const InputState& input)
{
    const uint8_t previousIndex = selectedIndex;
    if (!moveSelection(input, selectedIndex, NETWORK_ACTION_COUNT))
    {
        return input.upPressed || input.downPressed;
    }
    redrawMessageSelection(
        selectedNetwork.getSsid(), nullptr,
        NETWORK_ACTIONS, NETWORK_ACTION_COUNT,
        previousIndex, selectedIndex
    );
    return true;
}

NavigationRequest WifiNetworkActionsPage::select()
{
    if (selectedIndex == 0)
    {
        getWifiManager().connectSavedNetwork(selectedNetwork.getSsid());
        return { NavigationMode::PopTo, PageId::WiFiSettings };
    }
    if (selectedIndex == 1)
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
    selectedIndex = 0;
    String message = "Forget ";
    message += selectedNetwork.getSsid();
    message += '?';
    drawOptionPage(
        "FORGET NETWORK", message.c_str(),
        FORGET_OPTIONS, FORGET_OPTION_COUNT,
        selectedIndex, batteryPercent
    );
}

bool WifiForgetNetworkPage::handleInput(const InputState& input)
{
    const uint8_t previousIndex = selectedIndex;
    if (!moveSelection(input, selectedIndex, FORGET_OPTION_COUNT))
    {
        return input.upPressed || input.downPressed;
    }
    String message = "Forget ";
    message += selectedNetwork.getSsid();
    message += '?';
    redrawMessageSelection(
        message.c_str(), nullptr,
        FORGET_OPTIONS, FORGET_OPTION_COUNT,
        previousIndex, selectedIndex
    );
    return true;
}

NavigationRequest WifiForgetNetworkPage::select()
{
    if (selectedIndex == 0)
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
    drawOptionPage(
        "ADD NETWORK", instructions.c_str(),
        BACK_OPTION, 1, 0, batteryPercent
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
