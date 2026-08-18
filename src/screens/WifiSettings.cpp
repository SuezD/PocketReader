#include "screens/WifiSettings.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr char ADD_NETWORK_LABEL[] = "Add New Network";
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
    const String heading = getHeading();

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("WI-FI SETTINGS", batteryPercent);
        drawMessage(heading.c_str(), nullptr, items, itemCount, selectedIndex);
        drawFooter();
    }
    while (display.nextPage());
}

bool WifiSettingsPage::handleInput(const InputState& input)
{
    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    const uint8_t previousIndex = selectedIndex;

    if (input.upPressed && !input.downPressed && selectedIndex > 0)
    {
        selectedIndex--;
    }
    else if (
        input.downPressed && !input.upPressed &&
        selectedIndex + 1 < itemCount
    ) {
        selectedIndex++;
    }
    else if (!input.upPressed && !input.downPressed)
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
    return { NavigationMode::Push, PageId::WifiSetup };
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
        redrawContent();
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
    return networkCount + 1;
}

String WifiSettingsPage::getHeading() const
{
    WifiManager& wifi = getWifiManager();
    String heading;

    if (wifi.isConnected())
    {
        heading = "Connected\n";
        heading += wifi.getNetworkName();
    }
    else if (wifi.getState() == WifiState::Connecting)
    {
        heading = "Connecting\n";
        heading += wifi.getNetworkName();
    }
    else if (
        wifi.getState() == WifiState::Failed &&
        wifi.getNetworkName()[0] != '\0'
    ) {
        heading = "Could not connect\n";
        heading += wifi.getNetworkName();
    }

    if (heading.length() > 0)
    {
        heading += '\n';
    }
    heading += "Known Networks";
    return heading;
}

void WifiSettingsPage::redrawSelection(uint8_t previousIndex)
{
    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    const String heading = getHeading();
    redrawMessageSelection(
        heading.c_str(), nullptr, items, itemCount,
        previousIndex, selectedIndex
    );
}

void WifiSettingsPage::redrawContent()
{
    const char* items[MAX_ITEMS];
    const uint8_t itemCount = getItems(items);
    if (selectedIndex >= itemCount) selectedIndex = itemCount - 1;
    const String heading = getHeading();

    setPageContentPartialWindow();
    display.firstPage();
    do
    {
        clearPageContent();
        drawMessage(heading.c_str(), nullptr, items, itemCount, selectedIndex);
    }
    while (display.nextPage());
}
