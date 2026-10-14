#pragma once

#include <Arduino.h>
#include <functional>

/**
 * Persists user adjustable settings on LittleFS, currently just the idle timeout.
 * Survives a restart, falls back to IDLE_TIMEOUT_MIN when nothing is stored yet.
 */
class DeviceSettings {
public:
    using ChangeHandler = std::function<void(uint32_t idleTimeoutMs)>;

    /**
     * Create the settings holder.
     *
     * @param defaultMinutes Idle timeout used until something is stored, or when the
     *                       stored value is invalid.
     * @param minMinutes Smallest value accepted from the config page.
     * @param maxMinutes Largest value accepted from the config page.
     */
    DeviceSettings(uint16_t defaultMinutes, uint16_t minMinutes, uint16_t maxMinutes);

    /**
     * Mount the filesystem and load the stored settings.
     *
     * @return void
     */
    void begin();

    /**
     * Current idle timeout.
     *
     * @return The timeout in milliseconds.
     */
    uint32_t idleTimeoutMs() const;

    /**
     * Set the handler called whenever the idle timeout changes, used to apply it to
     * AutoPowerOff without that class depending on this one.
     *
     * @param handler Receives the new timeout in milliseconds.
     * @return void
     */
    void setChangeHandler(ChangeHandler handler);

    /**
     * Validate, apply and store a new idle timeout.
     *
     * @param minutes Requested timeout in minutes.
     * @param error Receives a short error message when the value is rejected.
     * @return true if the value was accepted and saved.
     */
    bool setIdleTimeoutMin(uint16_t minutes, String& error);

    /**
     * Serialize the current settings for the config page.
     *
     * @return JSON document.
     */
    String toJson() const;

private:
    /**
     * Write the settings to /config.json.
     *
     * @return true on success.
     */
    bool save() const;

    /**
     * Read the settings from /config.json, missing file is not an error.
     *
     * @return void
     */
    void load();

    uint16_t _idleTimeoutMin;
    uint16_t _minMinutes;
    uint16_t _maxMinutes;
    ChangeHandler _changeHandler;
    bool _fsReady = false;
};
