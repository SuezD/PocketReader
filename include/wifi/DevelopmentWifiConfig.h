#pragma once

#if __has_include("wifi/DevelopmentWifi.h")
#include "wifi/DevelopmentWifi.h"
#else
namespace DevelopmentWifi
{
    constexpr char SSID[] = "";
    constexpr char PASSWORD[] = "";
}
#endif

