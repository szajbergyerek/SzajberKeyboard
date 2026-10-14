#include <Arduino.h>

#include "AutoPowerOff.h"
#include "BleKeyboard.h"
#include "HttpApi.h"
#include "KeyRemapper.h"
#include "OtaUpdater.h"
#include "StartupWatchdog.h"
#include "UsbKeyboardHost.h"
#include "VibrationMotor.h"
#include "WebhookSender.h"
#include "config.h"
#include "secrets.h"

static AutoPowerOff autoPowerOff(POWER_BUTTON_PIN, (uint32_t)IDLE_TIMEOUT_MIN * 60000UL, POWER_PULSE_HIGH_MS,
                                 POWER_PULSE_GAP_MS, POWER_PULSE_COUNT, POWER_OFF_VERIFY_MS,
                                 POWER_WARNING_LEAD_MS);
static OtaUpdater otaUpdater(WIFI_SSID, WIFI_PASSWORD, DEVICE_HOSTNAME, OTA_PASSWORD,
                             (uint32_t)OTA_WINDOW_MIN * 60000UL, KEEP_WIFI_ON);
static BleKeyboard bleKeyboard(BLE_DEVICE_NAME, BLE_MANUFACTURER);
static UsbKeyboardHost usbKeyboard(USB_NO_DEVICE_WARN_MS);
static StartupWatchdog startupWatchdog(STARTUP_ENUM_TIMEOUT_MS, STARTUP_MAX_RESTARTS);
static VibrationMotor vibrationMotor(VIBRATION_PIN, VIBRATION_MAX_PULSES, VIBRATION_MAX_ON_MS);
static KeyRemapper keyRemapper;
static WebhookSender webhookSender;
static HttpApi httpApi(HTTP_API_PORT, vibrationMotor, keyRemapper);

// Previous BLE state, so the connect vibration is triggered from loop() and the
// NimBLE callback never touches the motor state machine from another task
static bool bleWasConnected = false;

/**
 * Bring up the power button pin first, wait for the supply to settle, then start BLE,
 * the USB host, its startup watchdog, the vibration motor, the webhook sender,
 * the key remapper and the OTA window.
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
    autoPowerOff.setWarningHandler(
        []() { vibrationMotor.play(VIBRATE_WARNING_PULSES, VIBRATE_WARNING_ON_MS, VIBRATE_WARNING_OFF_MS); });
    vibrationMotor.begin();
    webhookSender.begin();
    keyRemapper.begin();
    keyRemapper.setActionHandler(
        [](const KeyRule& rule) { webhookSender.send(rule.method, rule.url, rule.header, rule.body); });
    bleKeyboard.begin();

    usbKeyboard.setKeyboardHandler([](const uint8_t report[8]) {
        autoPowerOff.notifyActivity();
        uint8_t remapped[8];
        keyRemapper.apply(report, remapped);
        bleKeyboard.sendKeyboardReport(remapped);
    });
    usbKeyboard.setConsumerHandler([](uint16_t usage) {
        autoPowerOff.notifyActivity();
        bleKeyboard.sendConsumerUsage(usage);
    });
    bleKeyboard.setLedHandler([](uint8_t leds) { usbKeyboard.setLeds(leds); });
    usbKeyboard.begin();
    startupWatchdog.begin();

    httpApi.setActivityHandler([]() { autoPowerOff.notifyActivity(); });
    httpApi.begin();
    otaUpdater.begin();
}

/**
 * Serve OTA and the HTTP API, report missing USB devices, watch the keyboard
 * enumeration after boot, advance the vibration pattern, vibrate once when a BLE
 * host connects and run the idle power off.
 *
 * @return void
 */
void loop() {
    otaUpdater.handle();
    httpApi.handle();
    vibrationMotor.update();
    keyRemapper.tick();

    bool bleConnected = bleKeyboard.isConnected();
    if (bleConnected && !bleWasConnected) {
        vibrationMotor.play(1, VIBRATE_CONNECT_MS, 0);
    }
    bleWasConnected = bleConnected;

    usbKeyboard.poll();
    startupWatchdog.update(usbKeyboard.hasKeyboard(), otaUpdater.isUpdating());
    autoPowerOff.update(otaUpdater.isUpdating());
    delay(10);
}
