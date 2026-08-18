#include "wifi/WifiProvisioningPortal.h"

#include <WiFi.h>

#include "generated/WifiWebAssets.h"
#include "wifi/WifiService.h"

namespace
{
    const char* getStatusName(WifiState state)
    {
        switch (state)
        {
            case WifiState::Disconnected: return "disconnected";
            case WifiState::Connecting: return "connecting";
            case WifiState::Connected: return "connected";
            case WifiState::Failed: return "failed";
        }

        return "unknown";
    }

    void appendJsonString(String& output, const String& value)
    {
        output += '"';

        for (size_t index = 0; index < value.length(); index++)
        {
            const char character = value[index];

            if (character == '"' || character == '\\')
            {
                output += '\\';
                output += character;
            }
            else if (static_cast<uint8_t>(character) >= 0x20)
            {
                output += character;
            }
        }

        output += '"';
    }
}

WifiProvisioningPortal::WifiProvisioningPortal()
    : server(80)
{
}

bool WifiProvisioningPortal::start()
{
    if (active)
    {
        return true;
    }

    if (!getWifiManager().startSetupAccessPoint())
    {
        return false;
    }

    server.on("/", HTTP_GET, [this]() { handleHome(); });
    server.on("/setup.css", HTTP_GET, [this]() {
        server.send_P(200, "text/css", WifiWebAssets::SETUP_CSS);
    });
    server.on("/setup.js", HTTP_GET, [this]() {
        server.send_P(200, "application/javascript", WifiWebAssets::SETUP_JS);
    });
    server.on("/connecting.js", HTTP_GET, [this]() {
        server.send_P(
            200,
            "application/javascript",
            WifiWebAssets::CONNECTING_JS
        );
    });
    server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
    server.on("/networks", HTTP_GET, [this]() { handleNetworks(); });
    server.on("/rescan", HTTP_POST, [this]() { handleRescan(); });
    server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server.onNotFound([this]() { handleNotFound(); });
    server.begin();
    active = true;
    connectionSubmitted = false;
    scanStarted = false;
    connectedAt = 0;
    startScan();
    Serial.println(F("Wi-Fi setup page started"));
    return true;
}

void WifiProvisioningPortal::stop()
{
    if (!active)
    {
        return;
    }

    server.stop();
    getWifiManager().stopSetupAccessPoint();
    active = false;
    connectionSubmitted = false;
    scanStarted = false;
    connectedAt = 0;
    Serial.println(F("Wi-Fi setup page stopped"));
}

void WifiProvisioningPortal::update()
{
    if (!active)
    {
        return;
    }

    server.handleClient();

    if (!connectionSubmitted || !getWifiManager().isConnected())
    {
        return;
    }

    if (connectedAt == 0)
    {
        connectedAt = millis();
    }
    else if (millis() - connectedAt >= SUCCESS_PAGE_DELAY_MS)
    {
        stop();
    }
}

bool WifiProvisioningPortal::isActive() const
{
    return active;
}

void WifiProvisioningPortal::handleHome()
{
    server.send_P(200, "text/html", WifiWebAssets::SETUP_HTML);
}

void WifiProvisioningPortal::handleConnect()
{
    if (!server.hasArg("ssid"))
    {
        server.send(400, "text/plain", "Network name is required");
        return;
    }

    const String ssid = server.arg("ssid");
    const String password = server.arg("password");

    if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 63)
    {
        server.send(400, "text/plain", "Invalid network details");
        return;
    }

    connectionSubmitted = true;
    connectedAt = 0;
    getWifiManager().connect(ssid.c_str(), password.c_str());
    server.send_P(202, "text/html", WifiWebAssets::CONNECTING_HTML);
}

void WifiProvisioningPortal::handleNetworks()
{
    const int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING)
    {
        server.send(200, "application/json", "{\"scanning\":true,\"networks\":[]}");
        return;
    }

    if (result == WIFI_SCAN_FAILED)
    {
        scanStarted = false;
        startScan();
        server.send(200, "application/json", "{\"scanning\":true,\"networks\":[]}");
        return;
    }

    String response = "{\"scanning\":false,\"networks\":[";

    for (int index = 0; index < result; index++)
    {
        if (index > 0)
        {
            response += ',';
        }

        response += "{\"ssid\":";
        appendJsonString(response, WiFi.SSID(index));
        response += ",\"rssi\":";
        response += WiFi.RSSI(index);
        response += ",\"secure\":";
        response += WiFi.encryptionType(index) == WIFI_AUTH_OPEN
            ? "false"
            : "true";
        response += '}';
    }

    response += "]}";
    server.send(200, "application/json", response);
}

void WifiProvisioningPortal::handleRescan()
{
    WiFi.scanDelete();
    scanStarted = false;
    startScan();
    server.send(202, "application/json", "{\"scanning\":true}");
}

void WifiProvisioningPortal::handleStatus()
{
    const WifiState state = getWifiManager().getState();
    String response = "{\"state\":\"";
    response += getStatusName(state);
    response += "\",\"message\":\"";

    if (state == WifiState::Connected)
    {
        response += "Connected. You can return to the reader.";
    }
    else if (state == WifiState::Failed)
    {
        response += "Connection failed. Check the network name and password.";
    }
    else
    {
        response += "Testing the network details...";
    }

    response += "\"}";
    server.send(200, "application/json", response);
}

void WifiProvisioningPortal::handleNotFound()
{
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

void WifiProvisioningPortal::startScan()
{
    if (scanStarted)
    {
        return;
    }

    const int result = WiFi.scanNetworks(true, false);
    scanStarted = result == WIFI_SCAN_RUNNING || result >= 0;

    if (!scanStarted)
    {
        Serial.println(F("Could not start Wi-Fi scan"));
    }
}
