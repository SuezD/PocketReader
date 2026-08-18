#pragma once

#include <Arduino.h>

class SelectedWifiNetwork
{
public:
    void select(const char* ssid)
    {
        selectedSsid = ssid == nullptr ? "" : ssid;
    }

    const char* getSsid() const
    {
        return selectedSsid.c_str();
    }

private:
    String selectedSsid;
};
