#pragma once

#include "navigation/Navigation.h"

class Page
{
public:
    virtual ~Page() = default;

    virtual void draw(uint8_t batteryPercent) = 0;
    virtual bool handleInput(const InputState& input) = 0;

    virtual NavigationRequest select()
    {
        return noNavigation();
    }

    virtual void onStartup()
    {
    }

    virtual void onEnter()
    {
    }

    virtual void onExit()
    {
    }

    virtual bool handleConnectivityStateChange(
        uint8_t batteryPercent,
        bool wifiStateChanged,
        bool portalStateChanged
    )
    {
        (void) batteryPercent;
        (void) wifiStateChanged;
        (void) portalStateChanged;
        return false;
    }
};
