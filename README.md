# Szájber Keyboard

ESP32-S3 firmware that turns a wired USB keyboard into a Bluetooth (BLE) keyboard.

The keyboard's data lines are soldered directly to the ESP32-S3 native USB pins, the board
runs as a USB host, reads the HID reports and forwards them over BLE HID to the paired
computer or phone. Everything runs from a battery through an IP5310 power module, which the
firmware switches off automatically after a period of inactivity.

## Hardware

| Part | Connection |
|------|------------|
| ESP32-S3 N16R8 DevKitC-1 | 5V pin from the IP5310 battery module |
| USB keyboard (61 keys, BYK901 controller) | D- to GPIO19, D+ to GPIO20, 5V and GND from the IP5310 module |
| IP5310 power button | Transistor driven by GPIO6 (HIGH = button pressed) |
| Serial log | UART0, the board's "COM" (CH343) USB-C port, 115200 baud |

The keyboard is not powered by the ESP32, so VBUS is not switched by the firmware.

## Features

- **USB host**: native USB-OTG host on GPIO19/20. The boot keyboard interface is read in boot
  protocol; other HID interfaces are parsed from their report descriptors, so multimedia
  (consumer control) keys and NKRO key bitmaps are forwarded as well. Caps/Num/Scroll Lock LED
  state from the host is sent back to the keyboard.
- **BLE HID keyboard**: advertises as "Szájber Keyboard", forwards key and multimedia reports
  immediately, stores bonds in NVS and reconnects to the paired host after power on.
- **Auto power off**: after `IDLE_TIMEOUT_MIN` minutes without a key press, GPIO6 gives two
  ~120 ms pulses 300 ms apart, which switches the IP5310 module off. If the board is still
  running 5 seconds later (for example while charging), the idle timer is reset.
- **OTA updates**: WiFi and ArduinoOTA are enabled for `OTA_WINDOW_MIN` minutes after boot,
  then WiFi is switched off. A running upload is never interrupted.
- **Serial log**: USB keyboard connect and disconnect, BLE connect and disconnect, power off
  requests, and a hint about swapped D-/D+ wires if no USB device appears within 10 seconds.

## Project layout

```
include/config.h          tunable settings (timeouts, GPIO6 pulse timing, device name)
include/secrets.h         WiFi and OTA settings (git ignored, see secrets.example.h)
src/main.cpp              wiring of the components
src/UsbKeyboardHost.*     USB host driver for the keyboard
src/HidReportParser.*     HID report descriptor parser (keyboard and consumer fields)
src/BleKeyboard.*         NimBLE based BLE HID keyboard
src/AutoPowerOff.*        idle timer and power module button control
src/OtaUpdater.*          time limited WiFi and ArduinoOTA
```

## Configuration

Edit `include/config.h`:

| Setting | Default | Meaning |
|---------|---------|---------|
| `IDLE_TIMEOUT_MIN` | 15 | Minutes without a key press before power off |
| `OTA_WINDOW_MIN` | 3 | Minutes after boot with WiFi and OTA enabled |
| `POWER_PULSE_HIGH_MS` | 120 | Length of one simulated button press |
| `POWER_PULSE_GAP_MS` | 300 | Pause between button presses |
| `BLE_DEVICE_NAME` | "Szájber Keyboard" | Bluetooth name |

Copy `include/secrets.example.h` to `include/secrets.h` and fill in the WiFi data.

## Building and uploading

PlatformIO is required (`pip install platformio`).

First upload over the "COM" USB port (switch the battery module off first, so there are not
two 5V sources at the same time):

```
pio run -e usb -t upload
pio device monitor
```

Later uploads over WiFi, within `OTA_WINDOW_MIN` minutes after power on:

```
pio run -e ota -t upload
```

Set `upload_port` in the `ota` environment of `platformio.ini` to the board's IP address.

## Notes

- The native USB port is used in host mode, so USB CDC is disabled and `Serial` stays on UART0.
- The approach was inspired by
  [KoStard/ESP32S3-USB-Keyboard-To-BLE](https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE),
  but the USB host and BLE code are written from scratch on top of the ESP-IDF USB host API and
  NimBLE-Arduino.
