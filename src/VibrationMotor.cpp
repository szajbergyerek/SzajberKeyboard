#include "VibrationMotor.h"

VibrationMotor::VibrationMotor(uint8_t pin, uint16_t maxPulses, uint16_t maxOnMs)
    : _pin(pin), _maxPulses(maxPulses), _maxOnMs(maxOnMs) {}

void VibrationMotor::begin() {
    // The pin must never float or drive the motor during boot
    digitalWrite(_pin, LOW);
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    Serial.printf("[VIBRA] Vibration motor ready on GPIO%u\n", (unsigned int)_pin);
}

bool VibrationMotor::play(uint16_t pulses, uint16_t onMs, uint16_t offMs) {
    if (pulses > _maxPulses || onMs > _maxOnMs || offMs > _maxOnMs) {
        return false;
    }
    if (pulses == 0 || onMs == 0) {
        stop();
        return true;
    }

    _remaining = pulses;
    _onMs = onMs;
    _offMs = offMs;
    _motorOn = true;
    _phaseStartMs = millis();
    digitalWrite(_pin, HIGH);
    Serial.printf("[VIBRA] Pattern started: %u x %u ms, %u ms gap\n", (unsigned int)pulses, (unsigned int)onMs,
                  (unsigned int)offMs);
    return true;
}

void VibrationMotor::stop() {
    digitalWrite(_pin, LOW);
    _motorOn = false;
    _remaining = 0;
}

void VibrationMotor::update() {
    if (_remaining == 0) {
        return;
    }

    uint32_t elapsed = millis() - _phaseStartMs;
    if (_motorOn) {
        if (elapsed < _onMs) {
            return;
        }
        digitalWrite(_pin, LOW);
        _motorOn = false;
        _phaseStartMs = millis();
        _remaining--;
        if (_remaining == 0) {
            Serial.println("[VIBRA] Pattern finished");
        }
        return;
    }

    if (elapsed < _offMs) {
        return;
    }
    _motorOn = true;
    _phaseStartMs = millis();
    digitalWrite(_pin, HIGH);
}

bool VibrationMotor::isBusy() const {
    return _remaining > 0;
}
