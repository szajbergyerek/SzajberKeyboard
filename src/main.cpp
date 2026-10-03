#include <Arduino.h>

#include "AutoPowerOff.h"
#include "BleKeyboard.h"
#include "OtaUpdater.h"
#include "UsbKeyboardHost.h"
#include "config.h"
#include "secrets.h"

static AutoPowerOff autoPowerOff(POWER_BUTTON_PIN, (uint32_t)IDLE_TIMEOUT_MIN * 60000UL, POWER_PULSE_HIGH_MS,
                                 POWER_PULSE_GAP_MS, POWER_PULSE_COUNT, POWER_OFF_VERIFY_MS);
static OtaUpdater otaUpdater(WIFI_SSID, WIFI_PASSWORD, DEVICE_HOSTNAME, OTA_PASSWORD,
                             (uint32_t)OTA_WINDOW_MIN * 60000UL);
static BleKeyboard bleKeyboard(BLE_DEVICE_NAME, BLE_MANUFACTURER);
static UsbKeyboardHost usbKeyboard(USB_NO_DEVICE_WARN_MS);

/**
 * Bring up the power button pin first, then BLE, the USB host and the OTA window.
 *
 * @return void
 */
void setup() {
    // The power button must never float or be pressed during boot
    digitalWrite(POWER_BUTTON_PIN, LOW);
    pinMode(POWER_BUTTON_PIN, OUTPUT);
    digitalWrite(POWER_BUTTON_PIN, LOW);

    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("[MAIN] USB to BLE keyboard bridge starting");

    autoPowerOff.begin();
    bleKeyboard.begin();

    usbKeyboard.setKeyboardHandler([](const uint8_t report[8]) {
        autoPowerOff.notifyActivity();
        bleKeyboard.sendKeyboardReport(report);
    });
    usbKeyboard.setConsumerHandler([](uint16_t usage) {
        autoPowerOff.notifyActivity();
        bleKeyboard.sendConsumerUsage(usage);
    });
    bleKeyboard.setLedHandler([](uint8_t leds) { usbKeyboard.setLeds(leds); });
    usbKeyboard.begin();

    otaUpdater.begin();
}

/**
 * Serve OTA during its window, report missing USB devices and run the idle power off.
 *
 * @return void
 */
void loop() {
    otaUpdater.handle();
    usbKeyboard.poll();
    autoPowerOff.update(otaUpdater.isUpdating());
    delay(10);
}
