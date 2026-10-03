#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <functional>

/**
 * BLE HID keyboard with a keyboard report and a consumer control (multimedia) report.
 * Bonds are stored in NVS by NimBLE, so paired hosts reconnect automatically after power on.
 */
class BleKeyboard : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
    using LedHandler = std::function<void(uint8_t leds)>;

    /**
     * Create the BLE keyboard.
     *
     * @param deviceName Name advertised to hosts.
     * @param manufacturer Manufacturer string of the device information service.
     */
    BleKeyboard(const char* deviceName, const char* manufacturer);

    /**
     * Initialize the BLE stack, the HID services and start advertising.
     *
     * @return void
     */
    void begin();

    /**
     * Check whether a host is connected.
     *
     * @return true if at least one host is connected.
     */
    bool isConnected() const;

    /**
     * Send an 8 byte boot format keyboard report (modifiers, reserved, 6 key codes).
     *
     * @param report The report to send.
     * @return void
     */
    void sendKeyboardReport(const uint8_t report[8]);

    /**
     * Send a consumer control usage (0 releases the key).
     *
     * @param usage Consumer page usage ID.
     * @return void
     */
    void sendConsumerUsage(uint16_t usage);

    /**
     * Set the handler called when the host changes the keyboard LEDs.
     *
     * @param handler Receives the LED bitmap.
     * @return void
     */
    void setLedHandler(LedHandler handler);

    /**
     * Called by NimBLE when a host connects.
     *
     * @param server The BLE server.
     * @param desc Connection descriptor.
     * @return void
     */
    void onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) override;

    /**
     * Called by NimBLE when a host disconnects.
     *
     * @param server The BLE server.
     * @param desc Connection descriptor.
     * @return void
     */
    void onDisconnect(NimBLEServer* server, ble_gap_conn_desc* desc) override;

    /**
     * Called by NimBLE when pairing or encryption finished.
     *
     * @param desc Connection descriptor with the security state.
     * @return void
     */
    void onAuthenticationComplete(ble_gap_conn_desc* desc) override;

    /**
     * Called by NimBLE when the host writes the LED output report.
     *
     * @param characteristic The written characteristic.
     * @return void
     */
    void onWrite(NimBLECharacteristic* characteristic) override;

private:
    const char* _deviceName;
    const char* _manufacturer;
    NimBLEServer* _server = nullptr;
    NimBLEHIDDevice* _hid = nullptr;
    NimBLECharacteristic* _keyboardInput = nullptr;
    NimBLECharacteristic* _keyboardOutput = nullptr;
    NimBLECharacteristic* _consumerInput = nullptr;
    LedHandler _ledHandler;
    volatile bool _connected = false;
};
