#include <Arduino.h>

#include "AutoPowerOff.h"
#include "BleKeyboard.h"
#include "OtaUpdater.h"
#include "StartupWatchdog.h"
#include "UsbKeyboardHost.h"
#include "config.h"
#include "secrets.h"

static AutoPowerOff autoPowerOff(POWER_BUTTON_PIN, (uint32_t)IDLE_TIMEOUT_MIN * 60000UL, POWER_PULSE_HIGH_MS,
                                 POWER_PULSE_GAP_MS, POWER_PULSE_COUNT, POWER_OFF_VERIFY_MS);
static OtaUpdater otaUpdater(WIFI_SSID, WIFI_PASSWORD, DEVICE_HOSTNAME, OTA_PASSWORD,
                             (uint32_t)OTA_WINDOW_MIN * 60000UL);
static BleKeyboard bleKeyboard(BLE_DEVICE_NAME, BLE_MANUFACTURER);
static UsbKeyboardHost usbKeyboard(USB_NO_DEVICE_WARN_MS);
static StartupWatchdog startupWatchdog(STARTUP_ENUM_TIMEOUT_MS, STARTUP_MAX_RESTARTS);

/**
 * Bring up the power button pin first, wait for the supply to settle, then start BLE,
 * the USB host, its startup watchdog and the OTA window.
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

    // The power module output ramps from about 2.3 V to 5 V, so give the rail and the
    // keyboard time to come up before the USB host looks for a device
    Serial.printf("[MAIN] Waiting %u ms for the supply and the keyboard\n", (unsigned int)USB_HOST_START_DELAY_MS);
    delay(USB_HOST_START_DELAY_MS);

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
    startupWatchdog.begin();

    otaUpdater.begin();
}

/**
 * Serve OTA during its window, report missing USB devices, watch the keyboard
 * enumeration after boot and run the idle power off.
 *
 * @return void
 */
void loop() {
    otaUpdater.handle();
    usbKeyboard.poll();
    startupWatchdog.update(usbKeyboard.hasKeyboard(), otaUpdater.isUpdating());
    autoPowerOff.update(otaUpdater.isUpdating());
    delay(10);
}
