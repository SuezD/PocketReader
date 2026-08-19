#pragma once

#include <Arduino.h>
#include <DNSServer.h>
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
    static constexpr uint16_t DNS_PORT = 53;

    DNSServer dnsServer;
    WebServer server;
    bool active = false;
    bool connectionSubmitted = false;
    bool scanStarted = false;
    unsigned long connectedAt = 0;

    void handleHome();
    void handleConnect();
    void handleNetworks();
    void handleRescan();
    void handleStatus();
    void handleBookServer();
    void handleSaveBookServer();
    void handleTestBookServer();
    void handleNotFound();
    void startScan();
};
