#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class WifiState : uint8_t
{
    Disconnected,
    Connecting,
    Connected,
    Failed
};

class WifiManager
{
public:
    void begin();
    void connect(const char* ssid, const char* password);
    void disconnect();
    void reconnectSavedNetwork();
    void forgetNetwork();
    bool startSetupAccessPoint();
    void stopSetupAccessPoint();
    void update();

    WifiState getState() const;
    bool isConnected() const;
    bool hasSavedNetwork() const;
    bool isSetupAccessPointActive() const;
    const char* getNetworkName() const;
    const char* getSetupNetworkName() const;
    const char* getSetupNetworkPassword() const;
    String getSetupAddress() const;

private:
    static constexpr unsigned long CONNECTION_TIMEOUT_MS = 15000;
    static constexpr unsigned long RETRY_DELAY_MS = 10000;
    static constexpr unsigned long CONNECTION_RESET_MS = 500;
    WifiState state = WifiState::Disconnected;
    Preferences preferences;
    String networkName;
    String networkPassword;
    unsigned long connectionStartedAt = 0;
    unsigned long retryAt = 0;
    bool shouldReconnect = false;
    bool connectionResetPending = false;
    bool preferencesReady = false;
    bool savedNetworkAvailable = false;
    bool saveAfterConnection = false;
    bool setupAccessPointActive = false;

    void startConnection();
    void saveNetwork();
    void setState(WifiState nextState);
};
