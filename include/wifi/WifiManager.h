#pragma once

#include <Arduino.h>

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
    void update();

    WifiState getState() const;
    bool isConnected() const;
    const char* getNetworkName() const;

private:
    static constexpr unsigned long CONNECTION_TIMEOUT_MS = 15000;
    static constexpr unsigned long RETRY_DELAY_MS = 10000;

    WifiState state = WifiState::Disconnected;
    String networkName;
    String networkPassword;
    unsigned long connectionStartedAt = 0;
    unsigned long retryAt = 0;
    bool shouldReconnect = false;

    void startConnection();
    void setState(WifiState nextState);
};

