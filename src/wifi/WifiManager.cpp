#include "wifi/WifiManager.h"

#include <WiFi.h>

namespace
{
    constexpr char SETUP_NETWORK_NAME[] = "PocketReader-Setup";
    constexpr char SETUP_NETWORK_PASSWORD[] = "pocketreader";

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

    preferencesReady = preferences.begin("pocket-wifi", false);

    if (!preferencesReady)
    {
        Serial.println(F("Could not open Wi-Fi preferences"));
        return;
    }

    networkName = preferences.getString("ssid", "");
    networkPassword = preferences.getString("password", "");
    savedNetworkAvailable = networkName.length() > 0;

    if (savedNetworkAvailable)
    {
        Serial.print(F("Loaded saved Wi-Fi network: "));
        Serial.println(networkName);
        shouldReconnect = true;
        saveAfterConnection = false;
        startConnection();
    }
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
    saveAfterConnection = true;
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    setState(WifiState::Disconnected);
}

void WifiManager::disconnect()
{
    shouldReconnect = false;
    connectionResetPending = false;
    retryAt = 0;
    WiFi.disconnect(false, false);
    setState(WifiState::Disconnected);
}

void WifiManager::reconnectSavedNetwork()
{
    if (!preferencesReady)
    {
        Serial.println(F("Wi-Fi preferences are unavailable"));
        return;
    }

    const String savedName = preferences.getString("ssid", "");

    if (savedName.length() == 0)
    {
        Serial.println(F("No saved Wi-Fi network"));
        return;
    }

    networkName = savedName;
    networkPassword = preferences.getString("password", "");
    savedNetworkAvailable = true;
    shouldReconnect = true;
    saveAfterConnection = false;
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    setState(WifiState::Disconnected);
}

void WifiManager::forgetNetwork()
{
    disconnect();

    if (preferencesReady)
    {
        preferences.remove("ssid");
        preferences.remove("password");
    }

    networkName = "";
    networkPassword = "";
    savedNetworkAvailable = false;
    saveAfterConnection = false;
    Serial.println(F("Forgot saved Wi-Fi network"));
}

bool WifiManager::startSetupAccessPoint()
{
    if (setupAccessPointActive)
    {
        return true;
    }

    WiFi.mode(isConnected() ? WIFI_AP_STA : WIFI_AP);

    if (!WiFi.softAP(SETUP_NETWORK_NAME, SETUP_NETWORK_PASSWORD))
    {
        Serial.println(F("Could not start Wi-Fi setup network"));
        return false;
    }

    setupAccessPointActive = true;
    Serial.print(F("Wi-Fi setup network: "));
    Serial.println(SETUP_NETWORK_NAME);
    Serial.print(F("Wi-Fi setup address: "));
    Serial.println(WiFi.softAPIP());
    return true;
}

void WifiManager::stopSetupAccessPoint()
{
    if (!setupAccessPointActive)
    {
        return;
    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    setupAccessPointActive = false;
    Serial.println(F("Wi-Fi setup network stopped"));
}

void WifiManager::update()
{
    const unsigned long now = millis();

    if (connectionResetPending)
    {
        if (static_cast<long>(now - retryAt) >= 0)
        {
            connectionResetPending = false;
            startConnection();
        }

        return;
    }

    const wl_status_t wifiStatus = WiFi.status();

    if (wifiStatus == WL_CONNECTED)
    {
        if (saveAfterConnection)
        {
            saveNetwork();
        }

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

bool WifiManager::hasSavedNetwork() const
{
    return savedNetworkAvailable;
}

bool WifiManager::isSetupAccessPointActive() const
{
    return setupAccessPointActive;
}

const char* WifiManager::getNetworkName() const
{
    return networkName.c_str();
}

const char* WifiManager::getSetupNetworkName() const
{
    return SETUP_NETWORK_NAME;
}

const char* WifiManager::getSetupNetworkPassword() const
{
    return SETUP_NETWORK_PASSWORD;
}

String WifiManager::getSetupAddress() const
{
    return WiFi.softAPIP().toString();
}

void WifiManager::startConnection()
{
    WiFi.mode(setupAccessPointActive ? WIFI_AP_STA : WIFI_STA);
    WiFi.begin(networkName.c_str(), networkPassword.c_str());
    connectionStartedAt = millis();
    retryAt = 0;
    setState(WifiState::Connecting);
}

void WifiManager::saveNetwork()
{
    saveAfterConnection = false;

    if (!preferencesReady)
    {
        Serial.println(F("Wi-Fi preferences are unavailable"));
        return;
    }

    const size_t ssidBytes = preferences.putString("ssid", networkName);
    preferences.putString(
        "password",
        networkPassword
    );

    if (ssidBytes == 0)
    {
        Serial.println(F("Could not save Wi-Fi credentials"));
        return;
    }

    savedNetworkAvailable = true;
    Serial.print(F("Saved Wi-Fi network: "));
    Serial.println(networkName);
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
