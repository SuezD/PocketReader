#include "screens/WifiSetup.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "wifi/WifiService.h"
#include "wifi/WifiProvisioningService.h"

void WifiSetupPage::draw(uint8_t batteryPercent)
{
    WifiManager& wifi = getWifiManager();
    String instructions = "Connect to ";
    instructions += wifi.getSetupNetworkName();
    instructions += "\nPassword: ";
    instructions += wifi.getSetupNetworkPassword();
    instructions += "\nThen open ";
    instructions += wifi.getSetupAddress();

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("WI-FI SETUP", batteryPercent);
        drawMessage(instructions.c_str());
        drawFooter();
    }
    while (display.nextPage());
}

bool WifiSetupPage::handleInput(const InputState& input)
{
    return input.upPressed || input.downPressed;
}

void WifiSetupPage::onEnter()
{
    WifiManager& wifi = getWifiManager();
    WifiProvisioningPortal& portal = getWifiProvisioningPortal();
    stopPortalOnExit = !portal.isActive();
    portal.start();
}

void WifiSetupPage::onExit()
{
    if (stopPortalOnExit)
    {
        getWifiProvisioningPortal().stop();
    }
}
