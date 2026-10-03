#include "OtaUpdater.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

OtaUpdater::OtaUpdater(const char* ssid, const char* password, const char* hostname, const char* otaPassword,
                       uint32_t windowMs)
    : _ssid(ssid), _password(password), _hostname(hostname), _otaPassword(otaPassword), _windowMs(windowMs) {}

void OtaUpdater::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(_hostname);
    WiFi.setAutoReconnect(true);
    Serial.printf("[OTA] WiFi enabled for %lu min, connecting...\n", (unsigned long)(_windowMs / 60000UL));
    WiFi.begin(_ssid, _password);
    _startMs = millis();
    _lastReconnectAttemptMs = _startMs;
    _active = true;
}

void OtaUpdater::handle() {
    if (!_active) {
        return;
    }
    maintainWifi();
    if (_otaStarted) {
        ArduinoOTA.handle();
    }
    if (!_updating && millis() - _startMs >= _windowMs) {
        shutdown();
    }
}

bool OtaUpdater::isUpdating() const {
    return _updating;
}

bool OtaUpdater::isActive() const {
    return _active;
}

void OtaUpdater::startOta() {
    if (_otaStarted) {
        return;
    }
    ArduinoOTA.setHostname(_hostname);
    if (_otaPassword != nullptr && _otaPassword[0] != '\0') {
        ArduinoOTA.setPassword(_otaPassword);
    }

    ArduinoOTA.onStart([this]() {
        _updating = true;
        Serial.println("[OTA] Update started");
    });
    ArduinoOTA.onEnd([this]() {
        _updating = false;
        Serial.println("[OTA] Update finished, rebooting");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int lastPercent = 101;
        unsigned int percent = total ? progress * 100 / total : 0;
        if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("[OTA] Progress: %u%%\n", percent);
            lastPercent = percent;
        }
    });
    ArduinoOTA.onError([this](ota_error_t error) {
        _updating = false;
        Serial.printf("[OTA] Error %u\n", (unsigned int)error);
    });

    ArduinoOTA.begin();
    _otaStarted = true;
    Serial.printf("[OTA] OTA ready, IP address: %s\n", WiFi.localIP().toString().c_str());
}

void OtaUpdater::maintainWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!_wasConnected) {
            Serial.printf("[OTA] WiFi connected, IP address: %s\n", WiFi.localIP().toString().c_str());
            _wasConnected = true;
        }
        startOta();
        return;
    }

    if (_wasConnected) {
        Serial.println("[OTA] WiFi connection lost");
        _wasConnected = false;
    }
    if (millis() - _lastReconnectAttemptMs >= RECONNECT_INTERVAL_MS) {
        _lastReconnectAttemptMs = millis();
        WiFi.disconnect();
        WiFi.begin(_ssid, _password);
    }
}

void OtaUpdater::shutdown() {
    if (_otaStarted) {
        ArduinoOTA.end();
        _otaStarted = false;
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    _active = false;
    Serial.println("[OTA] OTA window closed, WiFi turned off");
}
