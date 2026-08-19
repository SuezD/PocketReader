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

    const __FlashStringHelper* getWifiStatusName(wl_status_t status)
    {
        switch (status)
        {
            case WL_IDLE_STATUS: return F("idle");
            case WL_NO_SSID_AVAIL: return F("no SSID available");
            case WL_SCAN_COMPLETED: return F("scan completed");
            case WL_CONNECTED: return F("connected");
            case WL_CONNECT_FAILED: return F("connect failed");
            case WL_CONNECTION_LOST: return F("connection lost");
            case WL_DISCONNECTED: return F("disconnected");
            default: return F("unknown");
        }
    }

    const __FlashStringHelper* getDisconnectReasonName(uint8_t reason)
    {
        switch (reason)
        {
            case WIFI_REASON_AUTH_EXPIRE: return F("authentication expired");
            case WIFI_REASON_AUTH_LEAVE: return F("authentication left");
            case WIFI_REASON_ASSOC_EXPIRE: return F("association expired");
            case WIFI_REASON_ASSOC_LEAVE: return F("association left");
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                return F("4-way handshake timeout");
            case WIFI_REASON_802_1X_AUTH_FAILED:
                return F("802.1X authentication failed");
            case WIFI_REASON_STA_LEAVING: return F("station leaving");
            case WIFI_REASON_BEACON_TIMEOUT: return F("beacon timeout");
            case WIFI_REASON_NO_AP_FOUND: return F("access point not found");
            case WIFI_REASON_AUTH_FAIL: return F("authentication failed");
            case WIFI_REASON_ASSOC_FAIL: return F("association failed");
            case WIFI_REASON_HANDSHAKE_TIMEOUT: return F("handshake timeout");
            case WIFI_REASON_CONNECTION_FAIL: return F("connection failed");
            default: return F("other");
        }
    }

    void logWifiStatus(const __FlashStringHelper* prefix, wl_status_t status)
    {
        Serial.print(prefix);
        Serial.print(getWifiStatusName(status));
        Serial.print(F(" ("));
        Serial.print(static_cast<int>(status));
        Serial.println(')');
    }
}

void WifiManager::begin()
{
    Serial.println(F("[WiFi] Initialising station"));
    WiFi.onEvent(
        [this](arduino_event_id_t, arduino_event_info_t info)
        {
            const uint8_t reason = info.wifi_sta_disconnected.reason;
            Serial.print(F("[WiFi] STA disconnected from '"));
            Serial.print(networkName);
            Serial.print(F("': "));
            Serial.print(getDisconnectReasonName(reason));
            Serial.print(F(" (reason "));
            Serial.print(reason);
            Serial.print(F(", app state "));
            Serial.print(getStateName(state));
            Serial.println(')');
        },
        ARDUINO_EVENT_WIFI_STA_DISCONNECTED
    );
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    Serial.print(F("[WiFi] Device MAC: "));
    Serial.println(WiFi.macAddress());
    setState(WifiState::Disconnected);

    preferencesReady = preferences.begin("pocket-wifi", false);

    if (!preferencesReady)
    {
        Serial.println(F("Could not open Wi-Fi preferences"));
        return;
    }

    loadSavedNetworks();
    Serial.print(F("Saved Wi-Fi networks: "));
    Serial.println(savedNetworkCount);
    for (uint8_t index = 0; index < savedNetworkCount; index++)
    {
        Serial.print(F("[WiFi] Saved priority "));
        Serial.print(index + 1);
        Serial.print(F(": '"));
        Serial.print(savedNetworkNames[index]);
        Serial.println('\'');
    }

    if (savedNetworkCount > 0)
    {
        networkName = savedNetworkNames[0];
        networkPassword = savedNetworkPasswords[0];
        Serial.print(F("Loaded saved Wi-Fi network: "));
        Serial.println(networkName);
        startupSelectionActive = true;
        saveAfterConnection = false;
        startSavedNetworkScan();
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
    Serial.print(F("[WiFi] Provisioning requested connection to '"));
    Serial.print(networkName);
    Serial.print(F("' (password characters: "));
    Serial.print(networkPassword.length());
    Serial.println(')');
    if (savedNetworkScanPending)
    {
        savedNetworkScanPending = false;
        retryAt = 0;
        setState(WifiState::Disconnected);
    }
    visibleSavedNetworkCount = 0;
    nextVisibleSavedNetwork = 0;
    startupSelectionActive = false;
    saveAfterConnection = true;
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    setState(WifiState::Disconnected);
}

void WifiManager::disconnect()
{
    startupSelectionActive = false;
    if (savedNetworkScanActive)
    {
        WiFi.scanDelete();
        savedNetworkScanActive = false;
    }
    savedNetworkScanPending = false;
    connectionResetPending = false;
    retryAt = 0;
    WiFi.disconnect(false, false);
    setState(WifiState::Disconnected);
}

bool WifiManager::connectSavedNetwork(const char* ssid)
{
    if (ssid == nullptr)
    {
        return false;
    }

    const int index = findSavedNetwork(String(ssid));
    if (index < 0)
    {
        Serial.print(F("[WiFi] Requested saved SSID was not found: '"));
        Serial.print(ssid);
        Serial.println('\'');
        return false;
    }

    networkName = savedNetworkNames[index];
    networkPassword = savedNetworkPasswords[index];
    Serial.print(F("[WiFi] User requested saved network '"));
    Serial.print(networkName);
    Serial.println('\'');
    startupSelectionActive = false;
    saveAfterConnection = false;
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    setState(WifiState::Disconnected);
    return true;
}

bool WifiManager::forgetSavedNetwork(const char* ssid)
{
    if (ssid == nullptr)
    {
        return false;
    }

    const int removedIndex = findSavedNetwork(String(ssid));
    if (removedIndex < 0)
    {
        return false;
    }

    const bool forgetsCurrentNetwork = networkName == ssid;

    for (
        uint8_t index = removedIndex;
        index + 1 < savedNetworkCount;
        index++
    ) {
        savedNetworkNames[index] = savedNetworkNames[index + 1];
        savedNetworkPasswords[index] = savedNetworkPasswords[index + 1];
    }

    savedNetworkCount--;
    savedNetworkNames[savedNetworkCount] = "";
    savedNetworkPasswords[savedNetworkCount] = "";
    persistSavedNetworks();

    Serial.print(F("Forgot Wi-Fi network: "));
    Serial.println(ssid);

    if (forgetsCurrentNetwork)
    {
        disconnect();
        networkName = "";
        networkPassword = "";
    }

    return true;
}

bool WifiManager::startSetupAccessPoint()
{
    if (setupAccessPointActive)
    {
        return true;
    }

    if (savedNetworkScanActive)
    {
        WiFi.scanDelete();
        savedNetworkScanActive = false;
        retryAt = 0;
        setState(WifiState::Disconnected);
    }
    if (savedNetworkScanPending)
    {
        savedNetworkScanPending = false;
        retryAt = 0;
        setState(WifiState::Disconnected);
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

    if (savedNetworkScanActive)
    {
        updateSavedNetworkScan();
        return;
    }

    if (savedNetworkScanPending)
    {
        if (static_cast<long>(now - retryAt) >= 0)
        {
            savedNetworkScanPending = false;
            beginSavedNetworkScan();
        }
        return;
    }

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
        startupSelectionActive = false;
        visibleSavedNetworkCount = 0;
        nextVisibleSavedNetwork = 0;
        return;
    }

    if (state == WifiState::Connected)
    {
        visibleSavedNetworkCount = 0;
        nextVisibleSavedNetwork = 0;
        setState(WifiState::Disconnected);
        retryAt = 0;
        startupSelectionActive = false;
    }

    if (state == WifiState::Connecting)
    {
        const unsigned long elapsed = now - connectionStartedAt;
        const bool rejected =
            elapsed >= CONNECTION_STATUS_GRACE_MS &&
            (
                wifiStatus == WL_CONNECT_FAILED ||
                wifiStatus == WL_NO_SSID_AVAIL
            );

        if (rejected || elapsed >= CONNECTION_TIMEOUT_MS)
        {
            Serial.print(F("[WiFi] Connection attempt to '"));
            Serial.print(networkName);
            Serial.print(F("' failed after "));
            Serial.print(elapsed);
            Serial.println(F(" ms"));
            logWifiStatus(F("[WiFi] Final Arduino status: "), wifiStatus);
            if (!rejected)
            {
                Serial.println(F("[WiFi] Failure cause: application timeout"));
            }
            WiFi.disconnect(false, false);

            if (startupSelectionActive && startNextVisibleSavedNetwork())
            {
                return;
            }

            setState(WifiState::Failed);
            retryAt = 0;
            startupSelectionActive = false;
        }

        return;
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

uint8_t WifiManager::getSavedNetworkCount() const
{
    return savedNetworkCount;
}

const char* WifiManager::getSavedNetworkName(uint8_t index) const
{
    return index < savedNetworkCount
        ? savedNetworkNames[index].c_str()
        : "";
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
    const wifi_mode_t mode =
        setupAccessPointActive ? WIFI_AP_STA : WIFI_STA;
    WiFi.mode(mode);
    Serial.print(F("[WiFi] Starting connection to '"));
    Serial.print(networkName);
    Serial.print(F("' using mode "));
    Serial.print(mode == WIFI_AP_STA ? F("AP+STA") : F("STA"));
    Serial.print(F("; password characters: "));
    Serial.println(networkPassword.length());
    const wl_status_t beginStatus =
        WiFi.begin(networkName.c_str(), networkPassword.c_str());
    logWifiStatus(F("[WiFi] WiFi.begin status: "), beginStatus);
    connectionStartedAt = millis();
    retryAt = 0;
    setState(WifiState::Connecting);
}

void WifiManager::startSavedNetworkScan()
{
    if (
        savedNetworkCount == 0 ||
        setupAccessPointActive ||
        savedNetworkScanActive ||
        savedNetworkScanPending
    ) {
        return;
    }

    WiFi.disconnect(false, false);
    Serial.println(F("[WiFi] Preparing one startup saved-network scan"));
    savedNetworkScanPending = true;
    retryAt = millis() + CONNECTION_RESET_MS;
    setState(WifiState::Connecting);
}

void WifiManager::beginSavedNetworkScan()
{
    WiFi.mode(WIFI_STA);
    visibleSavedNetworkCount = 0;
    nextVisibleSavedNetwork = 0;
    WiFi.scanDelete();
    const int result = WiFi.scanNetworks(
        true,
        false,
        false,
        SCAN_MAX_MS_PER_CHANNEL
    );
    Serial.print(F("[WiFi] scanNetworks returned "));
    Serial.println(result);

    if (result == WIFI_SCAN_RUNNING || result >= 0)
    {
        savedNetworkScanActive = true;
        retryAt = 0;
        setState(WifiState::Connecting);
        Serial.println(F("Scanning for saved Wi-Fi networks"));
    }
    else
    {
        Serial.println(F("[WiFi] Saved-network scan could not start"));
        if (!startSavedNetworkFallback())
        {
            retryAt = 0;
            startupSelectionActive = false;
            setState(WifiState::Failed);
        }
    }
}

bool WifiManager::updateSavedNetworkScan()
{
    const int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING)
    {
        return false;
    }

    savedNetworkScanActive = false;
    Serial.print(F("[WiFi] Saved-network scan completed with result "));
    Serial.println(result);
    if (result == WIFI_SCAN_FAILED)
    {
        WiFi.scanDelete();
        Serial.println(F("[WiFi] Saved-network scan failed or timed out"));
        if (!startSavedNetworkFallback())
        {
            retryAt = 0;
            startupSelectionActive = false;
            setState(WifiState::Failed);
        }
        return true;
    }

    if (result >= 0)
    {
        collectVisibleSavedNetworks(result);
    }
    WiFi.scanDelete();

    if (!startNextVisibleSavedNetwork())
    {
        retryAt = 0;
        startupSelectionActive = false;
        setState(WifiState::Failed);
        Serial.println(F("[WiFi] Scan succeeded, but no saved networks were visible"));
        return true;
    }

    return true;
}

void WifiManager::collectAllSavedNetworks()
{
    visibleSavedNetworkCount = savedNetworkCount;
    nextVisibleSavedNetwork = 0;
    for (uint8_t index = 0; index < savedNetworkCount; index++)
    {
        visibleSavedNetworkIndices[index] = index;
    }
}

bool WifiManager::startSavedNetworkFallback()
{
    Serial.println(F("[WiFi] Falling back to direct saved-network attempts"));
    collectAllSavedNetworks();
    if (startNextVisibleSavedNetwork())
    {
        return true;
    }

    Serial.println(F("[WiFi] No saved networks are available for fallback"));
    return false;
}

void WifiManager::collectVisibleSavedNetworks(int scanResultCount)
{
    visibleSavedNetworkCount = 0;
    nextVisibleSavedNetwork = 0;

    for (uint8_t savedIndex = 0; savedIndex < savedNetworkCount; savedIndex++)
    {
        for (int scanIndex = 0; scanIndex < scanResultCount; scanIndex++)
        {
            if (savedNetworkNames[savedIndex] == WiFi.SSID(scanIndex))
            {
                Serial.print(F("[WiFi] Matched saved priority "));
                Serial.print(savedIndex + 1);
                Serial.print(F(": '"));
                Serial.print(savedNetworkNames[savedIndex]);
                Serial.print(F("', RSSI "));
                Serial.print(WiFi.RSSI(scanIndex));
                Serial.print(F(" dBm, channel "));
                Serial.println(WiFi.channel(scanIndex));
                visibleSavedNetworkIndices[visibleSavedNetworkCount++] =
                    savedIndex;
                break;
            }
        }
    }

    Serial.print(F("[WiFi] Visible saved candidates: "));
    Serial.println(visibleSavedNetworkCount);
}

bool WifiManager::startNextVisibleSavedNetwork()
{
    if (nextVisibleSavedNetwork >= visibleSavedNetworkCount)
    {
        return false;
    }

    const uint8_t savedIndex =
        visibleSavedNetworkIndices[nextVisibleSavedNetwork++];
    networkName = savedNetworkNames[savedIndex];
    networkPassword = savedNetworkPasswords[savedIndex];
    saveAfterConnection = false;
    Serial.print(F("Trying saved Wi-Fi network: "));
    Serial.println(networkName);
    Serial.print(F("[WiFi] Candidate "));
    Serial.print(nextVisibleSavedNetwork);
    Serial.print(F(" of "));
    Serial.println(visibleSavedNetworkCount);
    Serial.println(F("[WiFi] Resetting station before candidate attempt"));
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    return true;
}

void WifiManager::saveNetwork()
{
    saveAfterConnection = false;

    if (!preferencesReady)
    {
        Serial.println(F("Wi-Fi preferences are unavailable"));
        return;
    }

    makeSavedNetworkPreferred(networkName, networkPassword);
    persistSavedNetworks();
    Serial.print(F("Saved Wi-Fi network: "));
    Serial.println(networkName);
}

void WifiManager::loadSavedNetworks()
{
    savedNetworkCount = min(
        preferences.getUChar("netCount", 0),
        MAX_SAVED_NETWORKS
    );

    for (uint8_t index = 0; index < savedNetworkCount; index++)
    {
        char ssidKey[8];
        char passwordKey[8];
        snprintf(ssidKey, sizeof(ssidKey), "ssid%u", index);
        snprintf(passwordKey, sizeof(passwordKey), "pass%u", index);
        savedNetworkNames[index] = preferences.getString(ssidKey, "");
        savedNetworkPasswords[index] =
            preferences.getString(passwordKey, "");

        if (savedNetworkNames[index].length() == 0)
        {
            savedNetworkCount = index;
            break;
        }
    }

    if (savedNetworkCount > 0)
    {
        return;
    }

    const String legacyName = preferences.getString("ssid", "");
    if (legacyName.length() == 0)
    {
        return;
    }

    savedNetworkNames[0] = legacyName;
    savedNetworkPasswords[0] = preferences.getString("password", "");
    savedNetworkCount = 1;
    persistSavedNetworks();
    if (preferences.isKey("ssid")) preferences.remove("ssid");
    if (preferences.isKey("password")) preferences.remove("password");
    Serial.println(F("Migrated saved Wi-Fi network storage"));
}

void WifiManager::persistSavedNetworks()
{
    if (!preferencesReady)
    {
        return;
    }

    preferences.putUChar("netCount", savedNetworkCount);
    for (uint8_t index = 0; index < MAX_SAVED_NETWORKS; index++)
    {
        char ssidKey[8];
        char passwordKey[8];
        snprintf(ssidKey, sizeof(ssidKey), "ssid%u", index);
        snprintf(passwordKey, sizeof(passwordKey), "pass%u", index);

        if (index < savedNetworkCount)
        {
            preferences.putString(ssidKey, savedNetworkNames[index]);
            preferences.putString(
                passwordKey,
                savedNetworkPasswords[index]
            );
        }
        else
        {
            if (preferences.isKey(ssidKey)) preferences.remove(ssidKey);
            if (preferences.isKey(passwordKey)) preferences.remove(passwordKey);
        }
    }
}

int WifiManager::findSavedNetwork(const String& ssid) const
{
    for (uint8_t index = 0; index < savedNetworkCount; index++)
    {
        if (savedNetworkNames[index] == ssid)
        {
            return index;
        }
    }
    return -1;
}

void WifiManager::makeSavedNetworkPreferred(
    const String& ssid,
    const String& password
)
{
    const int existingIndex = findSavedNetwork(ssid);
    uint8_t insertionCount = savedNetworkCount;

    if (existingIndex < 0 && insertionCount < MAX_SAVED_NETWORKS)
    {
        insertionCount++;
    }

    const int shiftStart = existingIndex >= 0
        ? existingIndex
        : insertionCount - 1;
    for (int index = shiftStart; index > 0; index--)
    {
        savedNetworkNames[index] = savedNetworkNames[index - 1];
        savedNetworkPasswords[index] = savedNetworkPasswords[index - 1];
    }

    savedNetworkNames[0] = ssid;
    savedNetworkPasswords[0] = password;
    savedNetworkCount = insertionCount;
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
        Serial.print(F("[WiFi] Signal: "));
        Serial.print(WiFi.RSSI());
        Serial.print(F(" dBm, channel "));
        Serial.println(WiFi.channel());
        Serial.print(F("[WiFi] Gateway: "));
        Serial.println(WiFi.gatewayIP());
        Serial.print(F("[WiFi] DNS: "));
        Serial.println(WiFi.dnsIP());
    }
}
