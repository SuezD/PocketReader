#include "screens/Settings.h"

#include "Display.h"
#include "DisplayRefresh.h"
#include "Theme.h"
#include "books/BookServerSettings.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "components/Selection.h"
#include "wifi/WifiProvisioningService.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr const char* OVERVIEW_OPTIONS[] = {
        "Wi-Fi", "Book Server", "Back"
    };
    constexpr uint8_t OVERVIEW_OPTION_COUNT = 3;
    constexpr char ADD_NETWORK_LABEL[] = "Configure Wi-Fi";
    constexpr char BACK_LABEL[] = "Back";
    constexpr const char* NETWORK_ACTIONS[] = {
        "Connect", "Forget Network", "Back"
    };
    constexpr uint8_t NETWORK_ACTION_COUNT = 3;
    constexpr const char* FORGET_OPTIONS[] = { "Cancel", "Forget" };
    constexpr uint8_t FORGET_OPTION_COUNT = 2;
    constexpr const char* SETUP_OPTIONS[] = { "Back" };
}

void SettingsPage::onEnter()
{
    state = State::Overview;
    optionSelection = 0;
    getWifiProvisioningPortal().stop();
}

void SettingsPage::onExit()
{
    getWifiProvisioningPortal().stop();
}

void SettingsPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;
    setPageRefreshWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("SETTINGS", batteryPercent);
        drawCurrentContent();
        const String status = getWifiStatus();
        drawFooter(status.length() == 0 ? nullptr : status.c_str());
    }
    while (display.nextPage());
}

bool SettingsPage::handleInput(const InputState& input)
{
    uint8_t count = 0;
    uint8_t* selection = &optionSelection;
    switch (state)
    {
        case State::Overview:
            count = OVERVIEW_OPTION_COUNT;
            selection = &overviewSelection;
            break;
        case State::NetworkList:
        {
            const char* items[MAX_NETWORK_ITEMS];
            count = getNetworkItems(items);
            selection = &networkSelection;
            break;
        }
        case State::NetworkActions:
            count = NETWORK_ACTION_COUNT;
            break;
        case State::ForgetConfirmation:
            count = FORGET_OPTION_COUNT;
            break;
        case State::BookServer:
        {
            const char* items[3];
            count = getBookServerActions(items);
            break;
        }
        case State::SetupInstructions:
            count = 1;
            break;
        case State::CheckingBookServer:
            return false;
    }

    const uint8_t previousIndex = *selection;
    if (!moveSelection(input, *selection, count)) return false;
    if (*selection != previousIndex) redrawCurrentSelection(previousIndex);
    return true;
}

NavigationRequest SettingsPage::select()
{
    WifiManager& wifi = getWifiManager();
    switch (state)
    {
        case State::Overview:
            if (overviewSelection == 0) enterState(State::NetworkList);
            else if (overviewSelection == 1) enterState(State::BookServer);
            else return navigateBack();
            return noNavigation();

        case State::NetworkList:
        {
            const uint8_t networkCount = wifi.getSavedNetworkCount();
            if (networkSelection < networkCount)
            {
                selectedNetwork = wifi.getSavedNetworkName(networkSelection);
                enterState(State::NetworkActions);
            }
            else if (networkSelection == networkCount)
            {
                openSetup(State::NetworkList);
            }
            else
            {
                enterState(State::Overview);
            }
            return noNavigation();
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

        case State::BookServer:
            if (getBookServerSettings().getManifestUrl()[0] == '\0')
            {
                if (optionSelection == 0) openSetup(State::BookServer);
                else enterState(State::Overview);
            }
            else if (optionSelection == 0)
            {
                checkBookServer();
            }
            else if (optionSelection == 1)
            {
                openSetup(State::BookServer);
            }
            else
            {
                enterState(State::Overview);
            }
            return noNavigation();

        case State::SetupInstructions:
            getWifiProvisioningPortal().stop();
            enterState(setupReturnState);
            return noNavigation();

        case State::CheckingBookServer:
            return noNavigation();
    }
    return noNavigation();
}

bool SettingsPage::handleConnectivityStateChange(
    uint8_t nextBatteryPercent,
    bool wifiStateChanged,
    bool portalStateChanged
) {
    (void) portalStateChanged;
    batteryPercent = nextBatteryPercent;
    if (!wifiStateChanged) return true;

    redrawHeaderStatus(batteryPercent);
    if (state == State::Overview) drawBody();
    else
    {
        const String status = getWifiStatus();
        redrawFooter(status.length() == 0 ? nullptr : status.c_str());
    }
    return true;
}

uint8_t SettingsPage::getNetworkItems(const char** items) const
{
    WifiManager& wifi = getWifiManager();
    const uint8_t count = wifi.getSavedNetworkCount();
    for (uint8_t index = 0; index < count; index++)
    {
        items[index] = wifi.getSavedNetworkName(index);
    }
    items[count] = ADD_NETWORK_LABEL;
    items[count + 1] = BACK_LABEL;
    return count + 2;
}

uint8_t SettingsPage::getBookServerActions(const char** items) const
{
    uint8_t count = 0;
    if (getBookServerSettings().getManifestUrl()[0] != '\0')
    {
        items[count++] = "Check Connection";
    }
    items[count++] = "Configure Server";
    items[count++] = "Back";
    return count;
}

String SettingsPage::getWifiStatus() const
{
    WifiManager& wifi = getWifiManager();
    String status;
    if (wifi.isConnected()) status = "Connected: ";
    else if (wifi.getState() == WifiState::Connecting) status = "Connecting: ";
    else if (
        wifi.getState() == WifiState::Failed &&
        wifi.getNetworkName()[0] != '\0'
    ) {
        status = "Failed: ";
    }
    else return "Not connected";
    status += wifi.getNetworkName();
    return status;
}

String SettingsPage::getBookServerStatus() const
{
    BookServerSettings& settings = getBookServerSettings();
    if (settings.getManifestUrl()[0] == '\0') return "Not configured";

    BookSync& sync = getBookSync();
    if (!sync.hasManifestResultFor(settings.getRevision())) return "Configured";
    const BookSyncResult result = sync.getLastManifestResult();
    if (result == BookSyncResult::Success) return "Available";
    if (result == BookSyncResult::NotConnected) return "Not Connected to Wi-Fi";
    return "Unavailable";
}

String SettingsPage::getBookServerHost() const
{
    String url = getBookServerSettings().getManifestUrl();
    if (url.length() == 0) return "";
    const int scheme = url.indexOf("://");
    if (scheme >= 0) url.remove(0, scheme + 3);
    const int path = url.indexOf('/');
    if (path >= 0) url.remove(path);
    return url;
}

String SettingsPage::getForgetMessage() const
{
    String message = "Forget ";
    message += selectedNetwork;
    message += '?';
    return message;
}

String SettingsPage::getSetupInstructions() const
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

void SettingsPage::openSetup(State returnState)
{
    setupReturnState = returnState;
    if (!getWifiProvisioningPortal().start()) return;
    enterState(State::SetupInstructions);
}

void SettingsPage::checkBookServer()
{
    state = State::CheckingBookServer;
    drawBody();
    const BookSyncResult result = getBookSync().fetchManifest();
    Serial.print(F("[BookServer] Settings check result: "));
    Serial.println(static_cast<uint8_t>(result));
    enterState(State::BookServer);
}

void SettingsPage::drawBody()
{
    setPageBodyPartialWindow();
    display.firstPage();
    do
    {
        clearPageBody();
        drawCurrentContent();
        const String status = getWifiStatus();
        drawFooter(status.length() == 0 ? nullptr : status.c_str());
    }
    while (display.nextPage());
}

void SettingsPage::drawCurrentContent()
{
    switch (state)
    {
        case State::Overview:
        {
            const String wifi = getWifiStatus();
            const String host = getBookServerHost();
            const String server = host.length() == 0
                ? "Server: Not configured"
                : "Server: " + host;
            drawMessage(
                wifi.c_str(), server.c_str(),
                OVERVIEW_OPTIONS, OVERVIEW_OPTION_COUNT, overviewSelection
            );
            break;
        }
        case State::NetworkList:
        {
            const char* items[MAX_NETWORK_ITEMS];
            const uint8_t count = getNetworkItems(items);
            if (networkSelection >= count) networkSelection = count - 1;
            drawMessage("Known Networks", nullptr, items, count, networkSelection);
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
        case State::BookServer:
        {
            const String status = getBookServerStatus();
            const String host = getBookServerHost();
            const char* actions[3];
            const uint8_t count = getBookServerActions(actions);
            drawMessage(
                status.c_str(), host.length() == 0 ? nullptr : host.c_str(),
                actions, count, optionSelection
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
        case State::CheckingBookServer:
            drawMessage("Checking book server...");
            break;
    }
}

void SettingsPage::redrawCurrentSelection(uint8_t previousIndex)
{
    switch (state)
    {
        case State::Overview:
        {
            const String wifi = getWifiStatus();
            const String host = getBookServerHost();
            const String server = host.length() == 0
                ? "Server: Not configured"
                : "Server: " + host;
            redrawMessageSelection(
                wifi.c_str(), server.c_str(),
                OVERVIEW_OPTIONS, OVERVIEW_OPTION_COUNT,
                previousIndex, overviewSelection
            );
            break;
        }
        case State::NetworkList:
        {
            const char* items[MAX_NETWORK_ITEMS];
            const uint8_t count = getNetworkItems(items);
            redrawMessageSelection(
                "Known Networks", nullptr, items, count,
                previousIndex, networkSelection
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
        case State::BookServer:
        {
            const String status = getBookServerStatus();
            const String host = getBookServerHost();
            const char* actions[3];
            const uint8_t count = getBookServerActions(actions);
            redrawMessageSelection(
                status.c_str(), host.length() == 0 ? nullptr : host.c_str(),
                actions, count, previousIndex, optionSelection
            );
            break;
        }
        case State::SetupInstructions:
        case State::CheckingBookServer:
            break;
    }
}

void SettingsPage::enterState(State nextState)
{
    state = nextState;
    optionSelection = 0;
    drawBody();
}
