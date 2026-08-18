#include "wifi/WifiProvisioningService.h"

WifiProvisioningPortal& getWifiProvisioningPortal()
{
    static WifiProvisioningPortal portal;
    return portal;
}

