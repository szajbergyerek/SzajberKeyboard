#pragma once

// Tunable settings. Each value can also be overridden with a -D build flag.

// Minutes without a key press before the power module is switched off
#ifndef IDLE_TIMEOUT_MIN
#define IDLE_TIMEOUT_MIN 15
#endif

// Minutes after boot during which WiFi and ArduinoOTA stay enabled
#ifndef OTA_WINDOW_MIN
#define OTA_WINDOW_MIN 3
#endif

// GPIO driving the transistor that "presses" the power module button (HIGH = pressed)
#ifndef POWER_BUTTON_PIN
#define POWER_BUTTON_PIN 6
#endif

// Length of one simulated button press in milliseconds
#ifndef POWER_PULSE_HIGH_MS
#define POWER_PULSE_HIGH_MS 120
#endif

// Pause between the simulated button presses in milliseconds
#ifndef POWER_PULSE_GAP_MS
#define POWER_PULSE_GAP_MS 300
#endif

// Number of button presses that switch the power module off
#ifndef POWER_PULSE_COUNT
#define POWER_PULSE_COUNT 2
#endif

// If the board still runs this long after the power off request, resume normal operation
#ifndef POWER_OFF_VERIFY_MS
#define POWER_OFF_VERIFY_MS 5000
#endif

// Bluetooth device name shown on the host
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "Szájber Keyboard"
#endif

// Manufacturer string reported in the BLE device information service
#ifndef BLE_MANUFACTURER
#define BLE_MANUFACTURER "DIY"
#endif

// Warn about swapped D-/D+ wires if no USB device shows up within this time
#ifndef USB_NO_DEVICE_WARN_MS
#define USB_NO_DEVICE_WARN_MS 10000
#endif

// Delay at the start of setup() so the power module output can rise to 5 V
// and the keyboard can boot before the USB host is started
#ifndef USB_HOST_START_DELAY_MS
#define USB_HOST_START_DELAY_MS 1500
#endif

// Time allowed after the USB host start for the keyboard to enumerate
#ifndef STARTUP_ENUM_TIMEOUT_MS
#define STARTUP_ENUM_TIMEOUT_MS 5000
#endif

// Maximum number of automatic restarts while waiting for the keyboard
#ifndef STARTUP_MAX_RESTARTS
#define STARTUP_MAX_RESTARTS 3
#endif
