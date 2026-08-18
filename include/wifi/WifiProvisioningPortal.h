#pragma once

#include <Arduino.h>
#include <WebServer.h>

class WifiProvisioningPortal
{
public:
    WifiProvisioningPortal();

    bool start();
    void stop();
    void update();
    bool isActive() const;

private:
    static constexpr unsigned long SUCCESS_PAGE_DELAY_MS = 5000;

    WebServer server;
    bool active = false;
    bool connectionSubmitted = false;
    unsigned long connectedAt = 0;

    void handleHome();
    void handleConnect();
    void handleStatus();
    void handleNotFound();
};

