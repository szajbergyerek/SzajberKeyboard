#pragma once

// Copy this file to secrets.h and fill in your own values. secrets.h is git ignored.

// WiFi credentials (the ESP32 only supports 2.4 GHz networks)
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Device hostname, also used as the mDNS name
#define DEVICE_HOSTNAME "bt-keyboard"

// Password required by ArduinoOTA uploads (empty string disables authentication)
#define OTA_PASSWORD ""
