#pragma once

#include <Arduino.h>

/**
 * Switches the battery power module off after a period without key presses
 * by simulating button presses on a GPIO driven transistor.
 */
class AutoPowerOff {
public:
    /**
     * Create the power off controller.
     *
     * @param pin GPIO that presses the power module button when HIGH.
     * @param idleTimeoutMs Idle time before the power off request.
     * @param pulseHighMs Length of one simulated button press.
     * @param pulseGapMs Pause between two simulated button presses.
     * @param pulseCount Number of simulated button presses.
     * @param verifyDelayMs Time to wait before assuming the power off failed.
     */
    AutoPowerOff(uint8_t pin, uint32_t idleTimeoutMs, uint16_t pulseHighMs, uint16_t pulseGapMs,
                 uint8_t pulseCount, uint32_t verifyDelayMs);

    /**
     * Drive the button pin LOW (released) and start the idle timer.
     *
     * @return void
     */
    void begin();

    /**
     * Reset the idle timer. Safe to call from any task.
     *
     * @return void
     */
    void notifyActivity();

    /**
     * Check the idle timer and issue or verify the power off request. Call from loop().
     *
     * @param inhibit When true (for example during an OTA upload) no power off is started.
     * @return void
     */
    void update(bool inhibit);

private:
    /**
     * Generate the configured number of button press pulses (blocking, well under a second).
     *
     * @return void
     */
    void pressButton();

    uint8_t _pin;
    uint32_t _idleTimeoutMs;
    uint16_t _pulseHighMs;
    uint16_t _pulseGapMs;
    uint8_t _pulseCount;
    uint32_t _verifyDelayMs;
    volatile uint32_t _lastActivityMs = 0;
    bool _waitingForPowerDown = false;
    uint32_t _powerOffRequestMs = 0;
};
