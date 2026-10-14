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

// GPIO driving the vibration motor transistor (HIGH = motor on)
#ifndef VIBRATION_PIN
#define VIBRATION_PIN 7
#endif

// Largest pulse count accepted by the vibration API
#ifndef VIBRATION_MAX_PULSES
#define VIBRATION_MAX_PULSES 20
#endif

// Largest pulse and pause length accepted by the vibration API, in milliseconds
#ifndef VIBRATION_MAX_ON_MS
#define VIBRATION_MAX_ON_MS 5000
#endif

// TCP port of the HTTP API
#ifndef HTTP_API_PORT
#define HTTP_API_PORT 80
#endif

// Keep WiFi connected after the OTA window so the HTTP API stays reachable.
// Set to 0 to save battery, then only OTA uses WiFi during the first minutes.
#ifndef KEEP_WIFI_ON
#define KEEP_WIFI_ON 1
#endif

// Length of the single vibration played when a BLE host connects, in milliseconds
#ifndef VIBRATE_CONNECT_MS
#define VIBRATE_CONNECT_MS 150
#endif

// How long before the automatic power off the warning vibration is played
#ifndef POWER_WARNING_LEAD_MS
#define POWER_WARNING_LEAD_MS 60000
#endif

// Warning vibration pattern played before the automatic power off
#ifndef VIBRATE_WARNING_PULSES
#define VIBRATE_WARNING_PULSES 2
#endif
#ifndef VIBRATE_WARNING_ON_MS
#define VIBRATE_WARNING_ON_MS 150
#endif
#ifndef VIBRATE_WARNING_OFF_MS
#define VIBRATE_WARNING_OFF_MS 200
#endif
