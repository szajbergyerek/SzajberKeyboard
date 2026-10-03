#include "BleKeyboard.h"

static constexpr uint8_t KEYBOARD_REPORT_ID = 1;
static constexpr uint8_t CONSUMER_REPORT_ID = 2;

static const uint8_t REPORT_MAP[] = {
    // Keyboard
    0x05, 0x01,                // Usage Page (Generic Desktop)
    0x09, 0x06,                // Usage (Keyboard)
    0xA1, 0x01,                // Collection (Application)
    0x85, KEYBOARD_REPORT_ID,  //   Report ID
    0x05, 0x07,                //   Usage Page (Keyboard)
    0x19, 0xE0,                //   Usage Minimum (Left Control)
    0x29, 0xE7,                //   Usage Maximum (Right GUI)
    0x15, 0x00,                //   Logical Minimum (0)
    0x25, 0x01,                //   Logical Maximum (1)
    0x75, 0x01,                //   Report Size (1)
    0x95, 0x08,                //   Report Count (8)
    0x81, 0x02,                //   Input (Data, Variable, Absolute): modifiers
    0x95, 0x01,                //   Report Count (1)
    0x75, 0x08,                //   Report Size (8)
    0x81, 0x01,                //   Input (Constant): reserved byte
    0x95, 0x05,                //   Report Count (5)
    0x75, 0x01,                //   Report Size (1)
    0x05, 0x08,                //   Usage Page (LEDs)
    0x19, 0x01,                //   Usage Minimum (Num Lock)
    0x29, 0x05,                //   Usage Maximum (Kana)
    0x91, 0x02,                //   Output (Data, Variable, Absolute): LEDs
    0x95, 0x01,                //   Report Count (1)
    0x75, 0x03,                //   Report Size (3)
    0x91, 0x01,                //   Output (Constant): LED padding
    0x95, 0x06,                //   Report Count (6)
    0x75, 0x08,                //   Report Size (8)
    0x15, 0x00,                //   Logical Minimum (0)
    0x26, 0xFF, 0x00,          //   Logical Maximum (255)
    0x05, 0x07,                //   Usage Page (Keyboard)
    0x19, 0x00,                //   Usage Minimum (0)
    0x29, 0xFF,                //   Usage Maximum (255)
    0x81, 0x00,                //   Input (Data, Array, Absolute): key codes
    0xC0,                      // End Collection

    // Consumer control (multimedia keys)
    0x05, 0x0C,                // Usage Page (Consumer)
    0x09, 0x01,                // Usage (Consumer Control)
    0xA1, 0x01,                // Collection (Application)
    0x85, CONSUMER_REPORT_ID,  //   Report ID
    0x15, 0x00,                //   Logical Minimum (0)
    0x26, 0xFF, 0x03,          //   Logical Maximum (1023)
    0x19, 0x00,                //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,          //   Usage Maximum (1023)
    0x75, 0x10,                //   Report Size (16)
    0x95, 0x01,                //   Report Count (1)
    0x81, 0x00,                //   Input (Data, Array, Absolute)
    0xC0,                      // End Collection
};

BleKeyboard::BleKeyboard(const char* deviceName, const char* manufacturer)
    : _deviceName(deviceName), _manufacturer(manufacturer) {}

void BleKeyboard::begin() {
    NimBLEDevice::init(_deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    _server = NimBLEDevice::createServer();
    _server->setCallbacks(this, false);
    _server->advertiseOnDisconnect(true);

    _hid = new NimBLEHIDDevice(_server);
    _keyboardInput = _hid->inputReport(KEYBOARD_REPORT_ID);
    _keyboardOutput = _hid->outputReport(KEYBOARD_REPORT_ID);
    _consumerInput = _hid->inputReport(CONSUMER_REPORT_ID);
    _keyboardOutput->setCallbacks(this);

    _hid->manufacturer(_manufacturer);
    _hid->pnp(0x02, 0xE502, 0xA111, 0x0210);
    _hid->hidInfo(0x00, 0x01);
    _hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
    _hid->setBatteryLevel(100);
    _hid->startServices();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(_hid->hidService()->getUUID());
    advertising->setScanResponse(true);
    advertising->start();

    Serial.printf("[BLE] Advertising as \"%s\", %d bonded host(s) stored\n", _deviceName, NimBLEDevice::getNumBonds());
}

bool BleKeyboard::isConnected() const {
    return _connected;
}

void BleKeyboard::sendKeyboardReport(const uint8_t report[8]) {
    if (!_connected) {
        return;
    }
    _keyboardInput->setValue(report, 8);
    _keyboardInput->notify();
}

void BleKeyboard::sendConsumerUsage(uint16_t usage) {
    if (!_connected) {
        return;
    }
    uint8_t value[2] = {(uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
    _consumerInput->setValue(value, sizeof(value));
    _consumerInput->notify();
}

void BleKeyboard::setLedHandler(LedHandler handler) {
    _ledHandler = handler;
}

void BleKeyboard::onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) {
    _connected = true;
    // Request a short connection interval (7.5 to 15 ms) for low key latency
    server->updateConnParams(desc->conn_handle, 6, 12, 0, 200);
    Serial.printf("[BLE] Host connected: %s\n", NimBLEAddress(desc->peer_ota_addr).toString().c_str());
}

void BleKeyboard::onDisconnect(NimBLEServer* server, ble_gap_conn_desc* desc) {
    (void)desc;
    _connected = server->getConnectedCount() > 0;
    Serial.println("[BLE] Host disconnected, advertising again");
}

void BleKeyboard::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    Serial.printf("[BLE] Connection secured (encrypted: %s, bonded: %s)\n", desc->sec_state.encrypted ? "yes" : "no",
                  desc->sec_state.bonded ? "yes" : "no");
}

void BleKeyboard::onWrite(NimBLECharacteristic* characteristic) {
    NimBLEAttValue value = characteristic->getValue();
    if (value.length() >= 1 && _ledHandler) {
        _ledHandler(value.data()[0]);
    }
}
