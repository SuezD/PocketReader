#include "wifi/WifiService.h"

WifiManager& getWifiManager()
{
    static WifiManager wifiManager;
    return wifiManager;
}

