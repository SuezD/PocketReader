#include "screens/WifiSettings.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "wifi/WifiProvisioningService.h"
#include "wifi/WifiService.h"

namespace
{
    const char* getStateText(WifiState state)
    {
        switch (state)
        {
            case WifiState::Disconnected: return "Status: Disconnected";
            case WifiState::Connecting: return "Status: Connecting";
            case WifiState::Connected: return "Status: Connected";
            case WifiState::Failed: return "Status: Connection Failed";
        }

        return "Status: Unknown";
    }
}

void WifiSettingsPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;
    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    uint8_t actionCount = getActions(actions, labels);

    if (selectedIndex >= actionCount)
    {
        selectedIndex = actionCount == 0 ? 0 : actionCount - 1;
    }

    const String details = getDetails();

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("WI-FI SETTINGS", batteryPercent);
        drawMessage(
            getStatusLine(),
            details.c_str(),
            labels,
            actionCount,
            selectedIndex
        );
        drawFooter();
    }
    while (display.nextPage());
}

bool WifiSettingsPage::handleInput(const InputState& input)
{
    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);
    const uint8_t previousIndex = selectedIndex;

    if (input.upPressed && !input.downPressed && selectedIndex > 0)
    {
        selectedIndex--;
    }
    else if (
        input.downPressed && !input.upPressed &&
        selectedIndex + 1 < actionCount
    ) {
        selectedIndex++;
    }
    else if (!input.upPressed && !input.downPressed)
    {
        return false;
    }

    if (selectedIndex != previousIndex)
    {
        redrawSelection(previousIndex);
    }

    return true;
}

NavigationRequest WifiSettingsPage::select()
{
    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);

    if (selectedIndex >= actionCount)
    {
        return noNavigation();
    }

    WifiManager& wifi = getWifiManager();
    WifiProvisioningPortal& portal = getWifiProvisioningPortal();

    switch (actions[selectedIndex])
    {
        case Action::StartPortal:
            portal.start();
            break;
        case Action::StopPortal:
            portal.stop();
            break;
        case Action::Reconnect:
            wifi.reconnectSavedNetwork();
            break;
        case Action::ForgetNetwork:
            portal.stop();
            wifi.forgetNetwork();
            portal.start();
            break;
    }

    selectedIndex = 0;
    draw(batteryPercent);
    return noNavigation();
}

bool WifiSettingsPage::redrawOnWifiStateChange() const
{
    return true;
}

uint8_t WifiSettingsPage::getActions(
    Action* actions,
    const char** labels
) const {
    WifiManager& wifi = getWifiManager();
    uint8_t count = 0;

    if (getWifiProvisioningPortal().isActive())
    {
        actions[count] = Action::StopPortal;
        labels[count++] = "Stop Setup Portal";
    }
    else
    {
        actions[count] = Action::StartPortal;
        labels[count++] = "Start Setup Portal";
    }

    if (
        wifi.hasSavedNetwork() &&
        wifi.getState() != WifiState::Connected &&
        wifi.getState() != WifiState::Connecting
    ) {
        actions[count] = Action::Reconnect;
        labels[count++] = "Reconnect";
    }

    if (wifi.hasSavedNetwork())
    {
        actions[count] = Action::ForgetNetwork;
        labels[count++] = "Forget Network";
    }

    return count;
}

String WifiSettingsPage::getDetails() const
{
    WifiManager& wifi = getWifiManager();
    WifiProvisioningPortal& portal = getWifiProvisioningPortal();
    String details;

    if (portal.isActive())
    {
        details = "Setup: ";
        details += wifi.getSetupNetworkName();
        details += "\nPassword: ";
        details += wifi.getSetupNetworkPassword();
        details += "\nOpen: ";
        details += wifi.getSetupAddress();
    }
    else if (wifi.getNetworkName()[0] != '\0')
    {
        details = "Network: ";
        details += wifi.getNetworkName();
    }
    else
    {
        details = "Network: None";
    }

    return details;
}

const char* WifiSettingsPage::getStatusLine() const
{
    return getWifiProvisioningPortal().isActive()
        ? "Setup Portal Active"
        : getStateText(getWifiManager().getState());
}

void WifiSettingsPage::redrawSelection(uint8_t previousIndex)
{
    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);
    const String details = getDetails();

    redrawMessageSelection(
        getStatusLine(),
        details.c_str(),
        labels,
        actionCount,
        previousIndex,
        selectedIndex
    );
}
