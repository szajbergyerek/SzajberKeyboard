#pragma once

#include <Arduino.h>

/**
 * Drives the vibration motor through an NPN transistor on a single GPIO.
 * Patterns are played without blocking: play() only stores the request and
 * update() switches the motor on and off as the pattern progresses.
 */
class VibrationMotor {
public:
    /**
     * Create the motor driver.
     *
     * @param pin GPIO that switches the motor on when HIGH.
     * @param maxPulses Largest accepted pulse count, requests above it are rejected.
     * @param maxOnMs Largest accepted pulse length in milliseconds.
     */
    VibrationMotor(uint8_t pin, uint16_t maxPulses, uint16_t maxOnMs);

    /**
     * Drive the pin LOW (motor off) and configure it as an output.
     *
     * @return void
     */
    void begin();

    /**
     * Start a vibration pattern, replacing any pattern still running.
     *
     * @param pulses Number of vibrations, 0 stops the motor.
     * @param onMs Length of one vibration in milliseconds.
     * @param offMs Pause between two vibrations in milliseconds.
     * @return true if the parameters were accepted.
     */
    bool play(uint16_t pulses, uint16_t onMs, uint16_t offMs);

    /**
     * Stop immediately and drop the rest of the pattern.
     *
     * @return void
     */
    void stop();

    /**
     * Advance the running pattern. Call from loop().
     *
     * @return void
     */
    void update();

    /**
     * Check whether a pattern is still running.
     *
     * @return true while the motor has work left to do.
     */
    bool isBusy() const;

private:
    uint8_t _pin;
    uint16_t _maxPulses;
    uint16_t _maxOnMs;
    uint16_t _remaining = 0;
    uint16_t _onMs = 0;
    uint16_t _offMs = 0;
    bool _motorOn = false;
    uint32_t _phaseStartMs = 0;
};
