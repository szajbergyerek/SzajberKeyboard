#pragma once

#include <Arduino.h>

/**
 * Restarts the board when the USB keyboard does not enumerate shortly after boot.
 * The board can come up on a slowly rising supply (the power module ramps from about
 * 2.3 V to 5 V), where the keyboard is not ready yet when the USB host starts and no
 * device event ever arrives. A restart retries the whole enumeration.
 * The restart counter lives in RTC memory, so it survives ESP.restart() but not a real
 * power cycle: every time the power module is switched on, counting starts from zero.
 */
class StartupWatchdog {
public:
    /**
     * Create the watchdog.
     *
     * @param timeoutMs Time allowed for the keyboard to enumerate after begin().
     * @param maxRestarts Maximum number of automatic restarts per power up.
     */
    StartupWatchdog(uint32_t timeoutMs, uint8_t maxRestarts);

    /**
     * Start the timeout and arm the watchdog unless the restart budget is used up.
     * Call this right after the USB host has been started.
     *
     * @return void
     */
    void begin();

    /**
     * Clear the counter on success, or restart the board when the timeout expires.
     * Call from loop().
     *
     * @param keyboardReady True while an enumerated HID keyboard is active.
     * @param inhibit When true (for example during an OTA upload) no restart is done.
     * @return void
     */
    void update(bool keyboardReady, bool inhibit);

    /**
     * Number of automatic restarts already done since the last power up.
     *
     * @return The restart count.
     */
    uint8_t restartCount() const;

private:
    uint32_t _timeoutMs;
    uint8_t _maxRestarts;
    uint32_t _startMs = 0;
    bool _armed = false;
};
