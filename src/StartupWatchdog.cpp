#include "StartupWatchdog.h"

#include <esp_system.h>

// RTC memory is not reloaded from flash on a software reset, so these keep their value
// across ESP.restart() and are only treated as fresh after a power on reset.
static RTC_NOINIT_ATTR uint32_t rtcMagic;
static RTC_NOINIT_ATTR uint32_t rtcRestartCount;

static constexpr uint32_t RTC_MAGIC = 0x4B424457;  // "KBDW"

StartupWatchdog::StartupWatchdog(uint32_t timeoutMs, uint8_t maxRestarts)
    : _timeoutMs(timeoutMs), _maxRestarts(maxRestarts) {}

void StartupWatchdog::begin() {
    esp_reset_reason_t reason = esp_reset_reason();
    if (rtcMagic != RTC_MAGIC || reason == ESP_RST_POWERON) {
        rtcMagic = RTC_MAGIC;
        rtcRestartCount = 0;
    }

    _startMs = millis();
    _armed = rtcRestartCount < _maxRestarts;
    if (_armed) {
        Serial.printf("[WATCHDOG] Waiting up to %lu ms for the keyboard, %u of %u restarts used\n",
                      (unsigned long)_timeoutMs, (unsigned int)rtcRestartCount, (unsigned int)_maxRestarts);
    } else {
        Serial.printf("[WATCHDOG] No keyboard after %u restarts, running without one\n",
                      (unsigned int)rtcRestartCount);
    }
}

void StartupWatchdog::update(bool keyboardReady, bool inhibit) {
    if (keyboardReady) {
        if (rtcRestartCount != 0) {
            rtcRestartCount = 0;
            Serial.println("[WATCHDOG] Keyboard enumerated, restart counter cleared");
        }
        _armed = false;
        return;
    }
    if (!_armed) {
        return;
    }
    if (inhibit) {
        _startMs = millis();
        return;
    }
    if (millis() - _startMs < _timeoutMs) {
        return;
    }

    rtcRestartCount++;
    Serial.printf("[WATCHDOG] No keyboard %lu ms after the USB host start, restarting (%u of %u)\n",
                  (unsigned long)_timeoutMs, (unsigned int)rtcRestartCount, (unsigned int)_maxRestarts);
    Serial.flush();
    delay(50);
    ESP.restart();
}

uint8_t StartupWatchdog::restartCount() const {
    return (uint8_t)rtcRestartCount;
}
