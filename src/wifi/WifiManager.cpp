#include "wifi/WifiManager.h"

#include <WiFi.h>

namespace
{
    const __FlashStringHelper* getStateName(WifiState state)
    {
        switch (state)
        {
            case WifiState::Disconnected:
                return F("disconnected");
            case WifiState::Connecting:
                return F("connecting");
            case WifiState::Connected:
                return F("connected");
            case WifiState::Failed:
                return F("failed");
        }

        return F("unknown");
    }
}

void WifiManager::begin()
{
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    setState(WifiState::Disconnected);
}

void WifiManager::connect(const char* ssid, const char* password)
{
    if (ssid == nullptr || ssid[0] == '\0')
    {
        Serial.println(F("Wi-Fi credentials are not configured"));
        return;
    }

    networkName = ssid;
    networkPassword = password == nullptr ? "" : password;
    shouldReconnect = true;
    startConnection();
}

void WifiManager::disconnect()
{
    shouldReconnect = false;
    retryAt = 0;
    WiFi.disconnect(false, false);
    setState(WifiState::Disconnected);
}

void WifiManager::update()
{
    const unsigned long now = millis();
    const wl_status_t wifiStatus = WiFi.status();

    if (wifiStatus == WL_CONNECTED)
    {
        setState(WifiState::Connected);
        return;
    }

    if (state == WifiState::Connected)
    {
        setState(WifiState::Disconnected);
        retryAt = now + RETRY_DELAY_MS;
    }

    if (state == WifiState::Connecting)
    {
        const bool rejected =
            wifiStatus == WL_CONNECT_FAILED ||
            wifiStatus == WL_NO_SSID_AVAIL;

        if (rejected || now - connectionStartedAt >= CONNECTION_TIMEOUT_MS)
        {
            WiFi.disconnect(false, false);
            setState(WifiState::Failed);
            retryAt = now + RETRY_DELAY_MS;
        }

        return;
    }

    if (
        shouldReconnect &&
        networkName.length() > 0 &&
        retryAt != 0 &&
        static_cast<long>(now - retryAt) >= 0
    ) {
        startConnection();
    }
}

WifiState WifiManager::getState() const
{
    return state;
}

bool WifiManager::isConnected() const
{
    return state == WifiState::Connected;
}

const char* WifiManager::getNetworkName() const
{
    return networkName.c_str();
}

void WifiManager::startConnection()
{
    WiFi.disconnect(false, false);
    WiFi.begin(networkName.c_str(), networkPassword.c_str());
    connectionStartedAt = millis();
    retryAt = 0;
    setState(WifiState::Connecting);
}

void WifiManager::setState(WifiState nextState)
{
    if (state == nextState)
    {
        return;
    }

    state = nextState;
    Serial.print(F("Wi-Fi state: "));
    Serial.println(getStateName(state));

    if (state == WifiState::Connected)
    {
        Serial.print(F("Wi-Fi network: "));
        Serial.println(networkName);
        Serial.print(F("Wi-Fi address: "));
        Serial.println(WiFi.localIP());
    }
}

