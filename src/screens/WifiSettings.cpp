#include "screens/WifiSettings.h"

#include "Display.h"
#include "DisplayRefresh.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "components/Selection.h"
#include "wifi/WifiProvisioningService.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr char ADD_NETWORK_LABEL[] = "Add New Network";
    constexpr char BACK_LABEL[] = "Back";
    constexpr const char* NETWORK_ACTIONS[] = {
        "Connect", "Forget Network", "Back"
    };
    constexpr uint8_t NETWORK_ACTION_COUNT = 3;
    constexpr const char* FORGET_OPTIONS[] = { "Cancel", "Forget" };
    constexpr uint8_t FORGET_OPTION_COUNT = 2;
    constexpr const char* SETUP_OPTIONS[] = { "Back" };
}

void WifiSettingsPage::onEnter()
{
    state = State::NetworkList;
    optionSelection = 0;
    getWifiProvisioningPortal().stop();
}

void WifiSettingsPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;

    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    if (networkSelection >= itemCount) networkSelection = itemCount - 1;

    setPageRefreshWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("WI-FI SETTINGS", batteryPercent);
        drawCurrentContent();
        const String status = getConnectionStatus();
        drawFooter(status.c_str());
    }
    while (display.nextPage());
}

bool WifiSettingsPage::handleInput(const InputState& input)
{
    uint8_t optionCount = 0;
    uint8_t* selection = &optionSelection;
    switch (state)
    {
        case State::NetworkList:
        {
            const char* items[MAX_ITEMS];
            optionCount = getItems(items);
            selection = &networkSelection;
            break;
        }
        case State::NetworkActions:
            optionCount = NETWORK_ACTION_COUNT;
            break;
        case State::ForgetConfirmation:
            optionCount = FORGET_OPTION_COUNT;
            break;
        case State::SetupInstructions:
            optionCount = 1;
            break;
    }

    const uint8_t previousIndex = *selection;
    if (!moveSelection(input, *selection, optionCount)) return false;
    if (*selection != previousIndex) redrawCurrentSelection(previousIndex);
    return true;
}

NavigationRequest WifiSettingsPage::select()
{
    WifiManager& wifi = getWifiManager();
    switch (state)
    {
        case State::NetworkList:
        {
            const uint8_t networkCount = wifi.getSavedNetworkCount();
            if (networkSelection < networkCount)
            {
                selectedNetwork = wifi.getSavedNetworkName(networkSelection);
                enterState(State::NetworkActions);
                return noNavigation();
            }
            if (networkSelection == networkCount)
            {
                getWifiProvisioningPortal().start();
                enterState(State::SetupInstructions);
                return noNavigation();
            }
            return navigateBack();
        }
        case State::NetworkActions:
            if (optionSelection == 0)
            {
                wifi.connectSavedNetwork(selectedNetwork.c_str());
                enterState(State::NetworkList);
            }
            else if (optionSelection == 1)
            {
                enterState(State::ForgetConfirmation);
            }
            else
            {
                enterState(State::NetworkList);
            }
            return noNavigation();
        case State::ForgetConfirmation:
            if (optionSelection == 0)
            {
                enterState(State::NetworkActions);
            }
            else
            {
                wifi.forgetSavedNetwork(selectedNetwork.c_str());
                selectedNetwork = "";
                enterState(State::NetworkList);
            }
            return noNavigation();
        case State::SetupInstructions:
            getWifiProvisioningPortal().stop();
            enterState(State::NetworkList);
            return noNavigation();
    }
    return noNavigation();
}

void WifiSettingsPage::onExit()
{
    getWifiProvisioningPortal().stop();
}

bool WifiSettingsPage::handleConnectivityStateChange(
    uint8_t nextBatteryPercent,
    bool wifiStateChanged,
    bool portalStateChanged
)
{
    (void) portalStateChanged;
    batteryPercent = nextBatteryPercent;
    if (wifiStateChanged)
    {
        redrawHeaderStatus(batteryPercent);
        const String connectionStatus = getConnectionStatus();
        redrawFooter(connectionStatus.c_str());
    }
    return true;
}

uint8_t WifiSettingsPage::getItems(const char** items) const
{
    WifiManager& wifi = getWifiManager();
    const uint8_t networkCount = wifi.getSavedNetworkCount();
    for (uint8_t index = 0; index < networkCount; index++)
    {
        items[index] = wifi.getSavedNetworkName(index);
    }
    items[networkCount] = ADD_NETWORK_LABEL;
    items[networkCount + 1] = BACK_LABEL;
    return networkCount + 2;
}

String WifiSettingsPage::getConnectionStatus() const
{
    WifiManager& wifi = getWifiManager();
    String status;
    if (wifi.isConnected()) status = "Connected: ";
    else if (wifi.getState() == WifiState::Connecting)
    {
        status = "Connecting: ";
    }
    else if (
        wifi.getState() == WifiState::Failed &&
        wifi.getNetworkName()[0] != '\0'
    ) {
        status = "Failed: ";
    }
    if (status.length() > 0) status += wifi.getNetworkName();
    return status;
}

String WifiSettingsPage::getForgetMessage() const
{
    String message = "Forget ";
    message += selectedNetwork;
    message += '?';
    return message;
}

String WifiSettingsPage::getSetupInstructions() const
{
    WifiManager& wifi = getWifiManager();
    String instructions = "Connect to: ";
    instructions += wifi.getSetupNetworkName();
    instructions += "\nPassword: ";
    instructions += wifi.getSetupNetworkPassword();
    instructions += "\nOpen: ";
    instructions += wifi.getSetupAddress();
    return instructions;
}

void WifiSettingsPage::drawContent()
{
    setPageBodyPartialWindow();
    display.firstPage();
    do
    {
        clearPageBody();
        drawCurrentContent();
        const String status = getConnectionStatus();
        drawFooter(status.c_str());
    }
    while (display.nextPage());
}

void WifiSettingsPage::drawCurrentContent()
{
    switch (state)
    {
        case State::NetworkList:
        {
            const char* items[MAX_ITEMS];
            const uint8_t itemCount = getItems(items);
            if (networkSelection >= itemCount) networkSelection = itemCount - 1;
            drawMessage(
                "Known Networks", nullptr,
                items, itemCount, networkSelection
            );
            break;
        }
        case State::NetworkActions:
            drawMessage(
                selectedNetwork.c_str(), nullptr,
                NETWORK_ACTIONS, NETWORK_ACTION_COUNT, optionSelection
            );
            break;
        case State::ForgetConfirmation:
        {
            const String message = getForgetMessage();
            drawMessage(
                message.c_str(), nullptr,
                FORGET_OPTIONS, FORGET_OPTION_COUNT, optionSelection
            );
            break;
        }
        case State::SetupInstructions:
        {
            const String instructions = getSetupInstructions();
            drawMessage(
                instructions.c_str(), nullptr,
                SETUP_OPTIONS, 1, optionSelection
            );
            break;
        }
    }
}

void WifiSettingsPage::redrawCurrentSelection(uint8_t previousIndex)
{
    switch (state)
    {
        case State::NetworkList:
        {
            const char* items[MAX_ITEMS];
            const uint8_t itemCount = getItems(items);
            redrawMessageSelection(
                "Known Networks", nullptr,
                items, itemCount, previousIndex, networkSelection
            );
            break;
        }
        case State::NetworkActions:
            redrawMessageSelection(
                selectedNetwork.c_str(), nullptr,
                NETWORK_ACTIONS, NETWORK_ACTION_COUNT,
                previousIndex, optionSelection
            );
            break;
        case State::ForgetConfirmation:
        {
            const String message = getForgetMessage();
            redrawMessageSelection(
                message.c_str(), nullptr,
                FORGET_OPTIONS, FORGET_OPTION_COUNT,
                previousIndex, optionSelection
            );
            break;
        }
        case State::SetupInstructions:
            break;
    }
}

void WifiSettingsPage::enterState(State nextState)
{
    state = nextState;
    optionSelection = 0;
    drawContent();
}
