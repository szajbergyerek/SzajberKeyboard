#include "AutoPowerOff.h"

AutoPowerOff::AutoPowerOff(uint8_t pin, uint32_t idleTimeoutMs, uint16_t pulseHighMs, uint16_t pulseGapMs,
                           uint8_t pulseCount, uint32_t verifyDelayMs, uint32_t warningLeadMs)
    : _pin(pin),
      _idleTimeoutMs(idleTimeoutMs),
      _pulseHighMs(pulseHighMs),
      _pulseGapMs(pulseGapMs),
      _pulseCount(pulseCount),
      _verifyDelayMs(verifyDelayMs),
      _warningLeadMs(warningLeadMs) {}

void AutoPowerOff::begin() {
    digitalWrite(_pin, LOW);
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _lastActivityMs = millis();
    Serial.printf("[POWER] Auto power off after %lu min without key presses\n",
                  (unsigned long)(_idleTimeoutMs / 60000UL));
}

void AutoPowerOff::notifyActivity() {
    _lastActivityMs = millis();
    _warned = false;
}

void AutoPowerOff::setWarningHandler(WarningHandler handler) {
    _warningHandler = handler;
}

void AutoPowerOff::update(bool inhibit) {
    uint32_t now = millis();

    if (_waitingForPowerDown) {
        if (now - _powerOffRequestMs >= _verifyDelayMs) {
            Serial.printf("[POWER] Still running %lu ms after the power off request (charging?), resetting idle timer\n",
                          (unsigned long)_verifyDelayMs);
            _waitingForPowerDown = false;
            _lastActivityMs = now;
            _warned = false;
        }
        return;
    }

    if (inhibit) {
        _lastActivityMs = now;
        _warned = false;
        return;
    }

    uint32_t idleMs = now - _lastActivityMs;
    if (!_warned && _warningLeadMs > 0 && _warningLeadMs < _idleTimeoutMs &&
        idleMs >= _idleTimeoutMs - _warningLeadMs) {
        _warned = true;
        Serial.printf("[POWER] Power off in %lu s unless a key is pressed\n",
                      (unsigned long)(_warningLeadMs / 1000UL));
        if (_warningHandler) {
            _warningHandler();
        }
    }

    if (idleMs >= _idleTimeoutMs) {
        Serial.printf("[POWER] No key press for %lu min, requesting power off\n",
                      (unsigned long)(_idleTimeoutMs / 60000UL));
        Serial.flush();
        pressButton();
        _waitingForPowerDown = true;
        _powerOffRequestMs = millis();
    }
}

void AutoPowerOff::pressButton() {
    for (uint8_t i = 0; i < _pulseCount; i++) {
        if (i > 0) {
            delay(_pulseGapMs);
        }
        digitalWrite(_pin, HIGH);
        delay(_pulseHighMs);
        digitalWrite(_pin, LOW);
    }
}
