#pragma once

#include <Arduino.h>

/**
 * Connects to WiFi and serves ArduinoOTA updates for a limited time window after boot.
 * Credentials are never written to the log.
 */
class OtaUpdater {
public:
    /**
     * Create the updater.
     *
     * @param ssid WiFi network name.
     * @param password WiFi password.
     * @param hostname Device hostname, also used as the mDNS name.
     * @param otaPassword Password required for OTA uploads, empty to disable authentication.
     * @param windowMs How long WiFi and OTA stay enabled after begin().
     */
    OtaUpdater(const char* ssid, const char* password, const char* hostname, const char* otaPassword,
               uint32_t windowMs);

    /**
     * Start connecting to WiFi without blocking. OTA starts as soon as WiFi is up.
     *
     * @return void
     */
    void begin();

    /**
     * Serve OTA requests, reconnect WiFi and close the window when it expires. Call from loop().
     *
     * @return void
     */
    void handle();

    /**
     * Check whether an OTA upload is currently running.
     *
     * @return true while an upload is in progress.
     */
    bool isUpdating() const;

    /**
     * Check whether the OTA window is still open.
     *
     * @return true while WiFi and OTA are enabled.
     */
    bool isActive() const;

private:
    /**
     * Configure and start ArduinoOTA (only once).
     *
     * @return void
     */
    void startOta();

    /**
     * Retry the WiFi connection if it has been lost for a while.
     *
     * @return void
     */
    void maintainWifi();

    /**
     * Stop ArduinoOTA and switch the WiFi radio off.
     *
     * @return void
     */
    void shutdown();

    const char* _ssid;
    const char* _password;
    const char* _hostname;
    const char* _otaPassword;
    uint32_t _windowMs;
    uint32_t _startMs = 0;
    bool _active = false;
    bool _otaStarted = false;
    bool _wasConnected = false;
    volatile bool _updating = false;
    uint32_t _lastReconnectAttemptMs = 0;
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 10000;
};
