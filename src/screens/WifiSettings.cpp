#include "screens/WifiSettings.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/Selection.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr char ADD_NETWORK_LABEL[] = "Add New Network";
    constexpr char BACK_LABEL[] = "Back";
}

WifiSettingsPage::WifiSettingsPage(SelectedWifiNetwork& nextSelectedNetwork)
    : selectedNetwork(nextSelectedNetwork)
{
}

void WifiSettingsPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;
    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    if (selectedIndex >= itemCount) selectedIndex = itemCount - 1;
    const String connectionStatus = getConnectionStatus();

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("WI-FI SETTINGS", batteryPercent);
        drawMessage("Known Networks", nullptr, items, itemCount, selectedIndex);
        drawFooter(connectionStatus.c_str());
    }
    while (display.nextPage());
}

bool WifiSettingsPage::handleInput(const InputState& input)
{
    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    const uint8_t previousIndex = selectedIndex;

    if (!moveSelection(input, selectedIndex, itemCount))
    {
        return false;
    }

    if (selectedIndex != previousIndex) redrawSelection(previousIndex);
    return true;
}

NavigationRequest WifiSettingsPage::select()
{
    WifiManager& wifi = getWifiManager();
    const uint8_t networkCount = wifi.getSavedNetworkCount();
    if (selectedIndex < networkCount)
    {
        selectedNetwork.select(wifi.getSavedNetworkName(selectedIndex));
        return { NavigationMode::Push, PageId::WifiNetworkActions };
    }
    if (selectedIndex == networkCount)
    {
        return { NavigationMode::Push, PageId::WifiSetup };
    }
    return { NavigationMode::Pop, PageId::MainMenu };
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

    if (wifi.isConnected())
    {
        status = "Connected: ";
    }
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

    if (status.length() > 0)
    {
        status += wifi.getNetworkName();
    }
    return status;
}

void WifiSettingsPage::redrawSelection(uint8_t previousIndex)
{
    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    redrawMessageSelection(
        "Known Networks", nullptr, items, itemCount,
        previousIndex, selectedIndex
    );
}
