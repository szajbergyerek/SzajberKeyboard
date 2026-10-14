#pragma once

#include <Arduino.h>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <usb/usb_host.h>

#include "HidReportParser.h"

/**
 * USB host driver for a wired keyboard on the native USB pins (GPIO19 D-, GPIO20 D+).
 * Reads the boot keyboard interface and, when present, report protocol interfaces
 * carrying multimedia (consumer control) keys or NKRO key bitmaps.
 */
class UsbKeyboardHost {
public:
    using KeyboardHandler = std::function<void(const uint8_t report[8])>;
    using ConsumerHandler = std::function<void(uint16_t usage)>;

    /**
     * Create the host driver.
     *
     * @param noDeviceWarnMs Warn about swapped data lines if no device appears within this time.
     */
    explicit UsbKeyboardHost(uint32_t noDeviceWarnMs);

    /**
     * Install the USB host library and start the background tasks.
     *
     * @return void
     */
    void begin();

    /**
     * Set the handler receiving merged 8 byte boot keyboard reports.
     *
     * @param handler Called from the USB task on every keyboard state change.
     * @return void
     */
    void setKeyboardHandler(KeyboardHandler handler);

    /**
     * Set the handler receiving consumer control (multimedia) usages.
     *
     * @param handler Called with the pressed usage, or 0 on release.
     * @return void
     */
    void setConsumerHandler(ConsumerHandler handler);

    /**
     * Forward the keyboard LED state (Num, Caps, Scroll Lock) to the USB keyboard.
     *
     * @param leds LED bitmap as used in HID output reports.
     * @return void
     */
    void setLeds(uint8_t leds);

    /**
     * Periodic housekeeping (missing device warning). Call from loop().
     *
     * @return void
     */
    void poll();

    /**
     * Check whether an enumerated HID keyboard interface is currently active.
     *
     * @return true when the keyboard is connected and its reports are being read.
     */
    bool hasKeyboard() const;

private:
    enum class InterfaceKind : uint8_t { BootKeyboard, ReportProtocol };

    struct HidInterface {
        UsbKeyboardHost* owner;
        uint8_t number;
        uint8_t endpoint;
        uint16_t maxPacketSize;
        InterfaceKind kind;
        HidReportParser parser;
        usb_transfer_t* transfer;
        volatile bool inFlight;
    };

    enum class CommandType : uint8_t { NewDevice, DeviceGone, SetLeds };

    struct Command {
        CommandType type;
        uint8_t value;
    };

    /**
     * FreeRTOS task running the USB host library event loop.
     *
     * @param arg Pointer to the UsbKeyboardHost instance.
     * @return void
     */
    static void libraryTask(void* arg);

    /**
     * FreeRTOS task processing client events and transfer callbacks.
     *
     * @param arg Pointer to the UsbKeyboardHost instance.
     * @return void
     */
    static void clientTask(void* arg);

    /**
     * FreeRTOS task opening, configuring and closing devices (may block on control transfers).
     *
     * @param arg Pointer to the UsbKeyboardHost instance.
     * @return void
     */
    static void workerTask(void* arg);

    /**
     * USB host client event callback, forwards device events to the worker task.
     *
     * @param msg Event message from the USB host library.
     * @param arg Pointer to the UsbKeyboardHost instance.
     * @return void
     */
    static void clientEventCallback(const usb_host_client_event_msg_t* msg, void* arg);

    /**
     * Completion callback of the interrupt IN transfers.
     *
     * @param transfer The completed transfer, context points to the HidInterface.
     * @return void
     */
    static void inTransferCallback(usb_transfer_t* transfer);

    /**
     * Completion callback of control transfers, wakes up the waiting worker.
     *
     * @param transfer The completed transfer, context points to the UsbKeyboardHost.
     * @return void
     */
    static void controlTransferCallback(usb_transfer_t* transfer);

    /**
     * Open a newly enumerated device and start reading its HID interfaces.
     *
     * @param address USB address of the device.
     * @return void
     */
    void openDevice(uint8_t address);

    /**
     * Stop all transfers, release the interfaces and close the current device.
     *
     * @return void
     */
    void closeDevice();

    /**
     * Claim and start one HID interface.
     *
     * @param intf Interface descriptor.
     * @param ep Interrupt IN endpoint descriptor.
     * @param reportDescLen Length of the HID report descriptor.
     * @return void
     */
    void setupInterface(const usb_intf_desc_t* intf, const usb_ep_desc_t* ep, uint16_t reportDescLen);

    /**
     * Run a control transfer synchronously.
     *
     * @param bmRequestType Request type byte.
     * @param bRequest Request code.
     * @param wValue Request value.
     * @param wIndex Request index (usually the interface number).
     * @param data Data to send (OUT) or buffer to receive into (IN), may be nullptr.
     * @param wLength Number of data bytes.
     * @param actualLen Receives the number of data bytes transferred, may be nullptr.
     * @return true if the transfer completed successfully.
     */
    bool controlTransfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t* data,
                         uint16_t wLength, uint16_t* actualLen = nullptr);

    /**
     * Process one input report received on an interface.
     *
     * @param hid The interface the report came from.
     * @param data Report bytes.
     * @param len Report length.
     * @return void
     */
    void handleReport(HidInterface& hid, const uint8_t* data, size_t len);

    /**
     * Merge the boot and report protocol key states and pass them to the handler on change.
     *
     * @return void
     */
    void emitKeyboardState();

    /**
     * Send the stored LED state to the boot keyboard interface.
     *
     * @return void
     */
    void sendLeds();

    /**
     * Convert a USB string descriptor to a printable ASCII string.
     *
     * @param desc String descriptor, may be nullptr.
     * @return The converted string, empty if not available.
     */
    static String stringFromDescriptor(const usb_str_desc_t* desc);

    static constexpr size_t MAX_INTERFACES = 4;
    static constexpr size_t CONTROL_BUFFER_SIZE = 1024;

    uint32_t _noDeviceWarnMs;
    KeyboardHandler _keyboardHandler;
    ConsumerHandler _consumerHandler;
    usb_host_client_handle_t _client = nullptr;
    QueueHandle_t _commands = nullptr;
    SemaphoreHandle_t _controlDone = nullptr;
    usb_transfer_t* _controlTransfer = nullptr;
    usb_device_handle_t _device = nullptr;
    uint8_t _maxPacketSize0 = 8;
    HidInterface _interfaces[MAX_INTERFACES];
    size_t _interfaceCount = 0;
    volatile bool _closing = false;
    volatile bool _deviceAttached = false;
    volatile uint8_t _leds = 0;
    bool _firstReportLogged = false;
    uint8_t _bootState[8] = {0};
    uint8_t _reportState[8] = {0};
    uint8_t _lastSent[8] = {0};
    uint16_t _lastConsumer = 0;
    uint32_t _noDeviceSinceMs = 0;
    bool _noDeviceWarned = false;
};
