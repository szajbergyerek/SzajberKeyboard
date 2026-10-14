#include "UsbKeyboardHost.h"

#include <usb/usb_helpers.h>

static constexpr uint8_t HID_SUBCLASS_BOOT = 0x01;
static constexpr uint8_t HID_PROTOCOL_KEYBOARD = 0x01;
static constexpr uint8_t HID_PROTOCOL_MOUSE = 0x02;
static constexpr uint8_t HID_DESCRIPTOR_TYPE = 0x21;
static constexpr uint8_t HID_REPORT_DESCRIPTOR_TYPE = 0x22;
static constexpr uint8_t HID_REQUEST_SET_REPORT = 0x09;
static constexpr uint8_t HID_REQUEST_SET_IDLE = 0x0A;
static constexpr uint8_t HID_REQUEST_SET_PROTOCOL = 0x0B;
static constexpr uint8_t REQUEST_TYPE_CLASS_INTERFACE_OUT = 0x21;
static constexpr uint8_t REQUEST_TYPE_STANDARD_INTERFACE_IN = 0x81;
static constexpr uint8_t REQUEST_GET_DESCRIPTOR = 0x06;
static constexpr uint32_t CONTROL_TIMEOUT_MS = 1000;

UsbKeyboardHost::UsbKeyboardHost(uint32_t noDeviceWarnMs) : _noDeviceWarnMs(noDeviceWarnMs) {}

void UsbKeyboardHost::begin() {
    _commands = xQueueCreate(8, sizeof(Command));
    _controlDone = xSemaphoreCreateBinary();
    _noDeviceSinceMs = millis();

    const usb_host_config_t hostConfig = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t err = usb_host_install(&hostConfig);
    if (err != ESP_OK) {
        Serial.printf("[USB] USB host install failed: %s\n", esp_err_to_name(err));
        return;
    }

    usb_host_client_config_t clientConfig = {};
    clientConfig.is_synchronous = false;
    clientConfig.max_num_event_msg = 5;
    clientConfig.async.client_event_callback = clientEventCallback;
    clientConfig.async.callback_arg = this;
    err = usb_host_client_register(&clientConfig, &_client);
    if (err != ESP_OK) {
        Serial.printf("[USB] USB host client register failed: %s\n", esp_err_to_name(err));
        return;
    }
    usb_host_transfer_alloc(CONTROL_BUFFER_SIZE, 0, &_controlTransfer);

    xTaskCreatePinnedToCore(libraryTask, "usb_lib", 4096, this, 8, nullptr, 1);
    xTaskCreatePinnedToCore(clientTask, "usb_client", 6144, this, 7, nullptr, 1);
    xTaskCreatePinnedToCore(workerTask, "usb_worker", 8192, this, 5, nullptr, 1);
    Serial.println("[USB] USB host started, waiting for keyboard");
}

void UsbKeyboardHost::setKeyboardHandler(KeyboardHandler handler) {
    _keyboardHandler = handler;
}

void UsbKeyboardHost::setConsumerHandler(ConsumerHandler handler) {
    _consumerHandler = handler;
}

void UsbKeyboardHost::setLeds(uint8_t leds) {
    _leds = leds;
    if (_commands != nullptr) {
        Command command = {CommandType::SetLeds, leds};
        xQueueSend(_commands, &command, 0);
    }
}

void UsbKeyboardHost::poll() {
    if (!_deviceAttached && !_noDeviceWarned && millis() - _noDeviceSinceMs >= _noDeviceWarnMs) {
        _noDeviceWarned = true;
        Serial.printf("[USB] No USB device detected for %lu s. Check the keyboard power and whether the "
                      "D- (GPIO19) and D+ (GPIO20) wires are swapped.\n",
                      (unsigned long)(_noDeviceWarnMs / 1000));
    }
}

bool UsbKeyboardHost::hasKeyboard() const {
    return _device != nullptr && _interfaceCount > 0;
}

void UsbKeyboardHost::libraryTask(void* arg) {
    (void)arg;
    while (true) {
        uint32_t flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

void UsbKeyboardHost::clientTask(void* arg) {
    UsbKeyboardHost* self = static_cast<UsbKeyboardHost*>(arg);
    while (true) {
        usb_host_client_handle_events(self->_client, portMAX_DELAY);
    }
}

void UsbKeyboardHost::workerTask(void* arg) {
    UsbKeyboardHost* self = static_cast<UsbKeyboardHost*>(arg);
    Command command;
    while (true) {
        if (xQueueReceive(self->_commands, &command, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        switch (command.type) {
            case CommandType::NewDevice: self->openDevice(command.value); break;
            case CommandType::DeviceGone: self->closeDevice(); break;
            case CommandType::SetLeds: self->sendLeds(); break;
        }
    }
}

void UsbKeyboardHost::clientEventCallback(const usb_host_client_event_msg_t* msg, void* arg) {
    UsbKeyboardHost* self = static_cast<UsbKeyboardHost*>(arg);
    Command command = {CommandType::NewDevice, 0};
    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        command.value = msg->new_dev.address;
    } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        command.type = CommandType::DeviceGone;
    } else {
        return;
    }
    xQueueSend(self->_commands, &command, 0);
}

void UsbKeyboardHost::inTransferCallback(usb_transfer_t* transfer) {
    HidInterface* hid = static_cast<HidInterface*>(transfer->context);
    UsbKeyboardHost* self = hid->owner;

    if (transfer->status == USB_TRANSFER_STATUS_COMPLETED) {
        self->handleReport(*hid, transfer->data_buffer, transfer->actual_num_bytes);
    }
    if (self->_closing || transfer->status == USB_TRANSFER_STATUS_NO_DEVICE ||
        transfer->status == USB_TRANSFER_STATUS_CANCELED) {
        hid->inFlight = false;
        return;
    }
    if (usb_host_transfer_submit(transfer) != ESP_OK) {
        hid->inFlight = false;
    }
}

void UsbKeyboardHost::controlTransferCallback(usb_transfer_t* transfer) {
    UsbKeyboardHost* self = static_cast<UsbKeyboardHost*>(transfer->context);
    xSemaphoreGive(self->_controlDone);
}

void UsbKeyboardHost::openDevice(uint8_t address) {
    _deviceAttached = true;
    _noDeviceWarned = false;
    if (_device != nullptr) {
        Serial.println("[USB] Additional USB device ignored, only one keyboard is supported");
        return;
    }

    esp_err_t err = usb_host_device_open(_client, address, &_device);
    if (err != ESP_OK) {
        Serial.printf("[USB] Failed to open USB device: %s\n", esp_err_to_name(err));
        _device = nullptr;
        _deviceAttached = false;
        _noDeviceSinceMs = millis();
        return;
    }

    usb_device_info_t info;
    const usb_device_desc_t* deviceDesc = nullptr;
    const usb_config_desc_t* configDesc = nullptr;
    if (usb_host_device_info(_device, &info) != ESP_OK || usb_host_get_device_descriptor(_device, &deviceDesc) != ESP_OK ||
        usb_host_get_active_config_descriptor(_device, &configDesc) != ESP_OK) {
        Serial.println("[USB] Failed to read USB device descriptors");
        usb_host_device_close(_client, _device);
        _device = nullptr;
        _deviceAttached = false;
        _noDeviceSinceMs = millis();
        return;
    }
    _maxPacketSize0 = deviceDesc->bMaxPacketSize0;
    _interfaceCount = 0;
    _firstReportLogged = false;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(configDesc);
    uint16_t total = configDesc->wTotalLength;
    const usb_intf_desc_t* currentIntf = nullptr;
    const usb_ep_desc_t* currentEp = nullptr;
    uint16_t reportDescLen = 0;

    auto finishInterface = [&]() {
        if (currentIntf != nullptr && currentEp != nullptr && currentIntf->bInterfaceClass == USB_CLASS_HID &&
            currentIntf->bAlternateSetting == 0) {
            setupInterface(currentIntf, currentEp, reportDescLen);
        }
    };

    uint16_t pos = 0;
    while (pos + 2 <= total) {
        uint8_t length = bytes[pos];
        uint8_t type = bytes[pos + 1];
        if (length == 0 || pos + length > total) {
            break;
        }
        if (type == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
            finishInterface();
            currentIntf = reinterpret_cast<const usb_intf_desc_t*>(bytes + pos);
            currentEp = nullptr;
            reportDescLen = 0;
        } else if (type == HID_DESCRIPTOR_TYPE && currentIntf != nullptr && length >= 9) {
            reportDescLen = bytes[pos + 7] | (bytes[pos + 8] << 8);
        } else if (type == USB_B_DESCRIPTOR_TYPE_ENDPOINT && currentIntf != nullptr && currentEp == nullptr) {
            const usb_ep_desc_t* ep = reinterpret_cast<const usb_ep_desc_t*>(bytes + pos);
            if ((ep->bmAttributes & 0x03) == USB_TRANSFER_TYPE_INTR && (ep->bEndpointAddress & 0x80)) {
                currentEp = ep;
            }
        }
        pos += length;
    }
    finishInterface();

    String product = stringFromDescriptor(info.str_desc_product);
    Serial.printf("[USB] Keyboard connected: VID 0x%04X PID 0x%04X, %s speed%s%s, %u HID interface(s) active\n",
                  deviceDesc->idVendor, deviceDesc->idProduct, info.speed == USB_SPEED_LOW ? "low" : "full",
                  product.length() ? ", " : "", product.c_str(), (unsigned int)_interfaceCount);
    if (_interfaceCount == 0) {
        Serial.println("[USB] No usable keyboard interface found on this device");
        _deviceAttached = false;
        _noDeviceWarned = false;
        _noDeviceSinceMs = millis();
    }
    sendLeds();
}

void UsbKeyboardHost::setupInterface(const usb_intf_desc_t* intf, const usb_ep_desc_t* ep, uint16_t reportDescLen) {
    if (_interfaceCount >= MAX_INTERFACES) {
        return;
    }
    uint8_t number = intf->bInterfaceNumber;
    HidInterface& hid = _interfaces[_interfaceCount];
    hid.owner = this;
    hid.number = number;
    hid.endpoint = ep->bEndpointAddress;
    hid.maxPacketSize = ep->wMaxPacketSize & 0x07FF;
    hid.transfer = nullptr;
    hid.inFlight = false;

    if (intf->bInterfaceSubClass == HID_SUBCLASS_BOOT && intf->bInterfaceProtocol == HID_PROTOCOL_KEYBOARD) {
        hid.kind = InterfaceKind::BootKeyboard;
    } else if (intf->bInterfaceProtocol == HID_PROTOCOL_MOUSE) {
        Serial.printf("[USB] Interface %u is a mouse, ignored\n", number);
        return;
    } else {
        hid.kind = InterfaceKind::ReportProtocol;
        uint16_t requestLen = reportDescLen ? reportDescLen : 256;
        if (requestLen > CONTROL_BUFFER_SIZE - sizeof(usb_setup_packet_t) - 64) {
            requestLen = CONTROL_BUFFER_SIZE - sizeof(usb_setup_packet_t) - 64;
        }
        uint8_t reportDesc[CONTROL_BUFFER_SIZE];
        uint16_t actual = 0;
        if (!controlTransfer(REQUEST_TYPE_STANDARD_INTERFACE_IN, REQUEST_GET_DESCRIPTOR,
                             HID_REPORT_DESCRIPTOR_TYPE << 8, number, reportDesc, requestLen, &actual)) {
            Serial.printf("[USB] Interface %u: failed to read the report descriptor, ignored\n", number);
            return;
        }
        if (!hid.parser.parse(reportDesc, actual)) {
            Serial.printf("[USB] Interface %u has no keyboard or multimedia keys, ignored\n", number);
            return;
        }
    }

    esp_err_t err = usb_host_interface_claim(_client, _device, number, 0);
    if (err != ESP_OK) {
        Serial.printf("[USB] Interface %u: claim failed: %s\n", number, esp_err_to_name(err));
        return;
    }

    if (hid.kind == InterfaceKind::BootKeyboard) {
        controlTransfer(REQUEST_TYPE_CLASS_INTERFACE_OUT, HID_REQUEST_SET_PROTOCOL, 0, number, nullptr, 0);
    }
    controlTransfer(REQUEST_TYPE_CLASS_INTERFACE_OUT, HID_REQUEST_SET_IDLE, 0, number, nullptr, 0);

    uint16_t transferSize = usb_round_up_to_mps(hid.maxPacketSize, hid.maxPacketSize);
    if (usb_host_transfer_alloc(transferSize, 0, &hid.transfer) != ESP_OK) {
        usb_host_interface_release(_client, _device, number);
        Serial.printf("[USB] Interface %u: transfer allocation failed\n", number);
        return;
    }
    hid.transfer->num_bytes = transferSize;
    hid.transfer->device_handle = _device;
    hid.transfer->bEndpointAddress = hid.endpoint;
    hid.transfer->callback = inTransferCallback;
    hid.transfer->context = &hid;
    hid.transfer->timeout_ms = 0;

    hid.inFlight = true;
    _interfaceCount++;
    err = usb_host_transfer_submit(hid.transfer);
    if (err != ESP_OK) {
        hid.inFlight = false;
        Serial.printf("[USB] Interface %u: transfer submit failed: %s\n", number, esp_err_to_name(err));
    }

    if (hid.kind == InterfaceKind::BootKeyboard) {
        Serial.printf("[USB] Interface %u: boot keyboard\n", number);
    } else {
        Serial.printf("[USB] Interface %u: report protocol (%s%s)\n", number,
                      hid.parser.hasKeyboardFields() ? "keys " : "",
                      hid.parser.hasConsumerFields() ? "multimedia" : "");
    }
}

void UsbKeyboardHost::closeDevice() {
    if (_device == nullptr) {
        _deviceAttached = false;
        return;
    }
    _closing = true;
    for (size_t i = 0; i < _interfaceCount; i++) {
        usb_host_endpoint_halt(_device, _interfaces[i].endpoint);
        usb_host_endpoint_flush(_device, _interfaces[i].endpoint);
    }

    uint32_t start = millis();
    bool busy = true;
    while (busy && millis() - start < 500) {
        busy = false;
        for (size_t i = 0; i < _interfaceCount; i++) {
            busy |= _interfaces[i].inFlight;
        }
        if (busy) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    for (size_t i = 0; i < _interfaceCount; i++) {
        HidInterface& hid = _interfaces[i];
        usb_host_interface_release(_client, _device, hid.number);
        if (hid.transfer != nullptr && !hid.inFlight) {
            usb_host_transfer_free(hid.transfer);
        }
        hid.transfer = nullptr;
    }
    usb_host_device_close(_client, _device);
    _device = nullptr;
    _interfaceCount = 0;
    _closing = false;

    // Release every key on the BLE side so nothing stays stuck
    memset(_bootState, 0, sizeof(_bootState));
    memset(_reportState, 0, sizeof(_reportState));
    emitKeyboardState();
    if (_lastConsumer != 0) {
        _lastConsumer = 0;
        if (_consumerHandler) {
            _consumerHandler(0);
        }
    }

    _deviceAttached = false;
    _noDeviceSinceMs = millis();
    Serial.println("[USB] Keyboard disconnected");
}

bool UsbKeyboardHost::controlTransfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                      uint8_t* data, uint16_t wLength, uint16_t* actualLen) {
    if (_device == nullptr || _controlTransfer == nullptr) {
        return false;
    }
    bool isIn = bmRequestType & 0x80;
    size_t dataCapacity = CONTROL_BUFFER_SIZE - sizeof(usb_setup_packet_t);
    if (wLength > dataCapacity) {
        return false;
    }

    usb_setup_packet_t* setup = reinterpret_cast<usb_setup_packet_t*>(_controlTransfer->data_buffer);
    setup->bmRequestType = bmRequestType;
    setup->bRequest = bRequest;
    setup->wValue = wValue;
    setup->wIndex = wIndex;
    setup->wLength = wLength;

    size_t dataBytes = wLength;
    if (isIn) {
        dataBytes = usb_round_up_to_mps(wLength, _maxPacketSize0);
        if (dataBytes > dataCapacity) {
            dataBytes = dataCapacity;
        }
    } else if (wLength > 0 && data != nullptr) {
        memcpy(_controlTransfer->data_buffer + sizeof(usb_setup_packet_t), data, wLength);
    }
    _controlTransfer->num_bytes = sizeof(usb_setup_packet_t) + dataBytes;
    _controlTransfer->device_handle = _device;
    _controlTransfer->bEndpointAddress = 0;
    _controlTransfer->callback = controlTransferCallback;
    _controlTransfer->context = this;
    _controlTransfer->timeout_ms = CONTROL_TIMEOUT_MS;

    xSemaphoreTake(_controlDone, 0);
    if (usb_host_transfer_submit_control(_client, _controlTransfer) != ESP_OK) {
        return false;
    }
    if (xSemaphoreTake(_controlDone, pdMS_TO_TICKS(CONTROL_TIMEOUT_MS + 500)) != pdTRUE) {
        Serial.println("[USB] Control transfer timed out");
        return false;
    }
    if (_controlTransfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        return false;
    }

    int received = _controlTransfer->actual_num_bytes - (int)sizeof(usb_setup_packet_t);
    if (received < 0) {
        received = 0;
    }
    if (isIn && data != nullptr) {
        if (received > wLength) {
            received = wLength;
        }
        memcpy(data, _controlTransfer->data_buffer + sizeof(usb_setup_packet_t), received);
    }
    if (actualLen != nullptr) {
        *actualLen = received;
    }
    return true;
}

void UsbKeyboardHost::handleReport(HidInterface& hid, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    if (!_firstReportLogged) {
        _firstReportLogged = true;
        Serial.println("[USB] First key report received from the keyboard");
    }

    if (hid.kind == InterfaceKind::BootKeyboard) {
        // ErrorRollOver: too many keys pressed, keep the previous state
        if (len >= 3 && data[2] == 0x01) {
            return;
        }
        memset(_bootState, 0, sizeof(_bootState));
        memcpy(_bootState, data, len < 8 ? len : 8);
        emitKeyboardState();
        return;
    }

    uint8_t keyboard[8];
    if (hid.parser.extractKeyboard(data, len, keyboard)) {
        memcpy(_reportState, keyboard, sizeof(_reportState));
        emitKeyboardState();
    }
    uint16_t usage = 0;
    if (hid.parser.extractConsumer(data, len, usage) && usage != _lastConsumer) {
        _lastConsumer = usage;
        if (_consumerHandler) {
            _consumerHandler(usage);
        }
    }
}

void UsbKeyboardHost::emitKeyboardState() {
    uint8_t merged[8] = {0};
    merged[0] = _bootState[0] | _reportState[0];
    uint8_t count = 0;
    const uint8_t* sources[2] = {_bootState, _reportState};
    for (const uint8_t* source : sources) {
        for (uint8_t i = 2; i < 8 && count < 6; i++) {
            uint8_t key = source[i];
            if (key == 0) {
                continue;
            }
            bool duplicate = false;
            for (uint8_t j = 0; j < count; j++) {
                duplicate |= merged[2 + j] == key;
            }
            if (!duplicate) {
                merged[2 + count++] = key;
            }
        }
    }

    if (memcmp(merged, _lastSent, sizeof(merged)) == 0) {
        return;
    }
    memcpy(_lastSent, merged, sizeof(merged));
    if (_keyboardHandler) {
        _keyboardHandler(merged);
    }
}

void UsbKeyboardHost::sendLeds() {
    if (_device == nullptr) {
        return;
    }
    for (size_t i = 0; i < _interfaceCount; i++) {
        if (_interfaces[i].kind == InterfaceKind::BootKeyboard) {
            uint8_t leds = _leds;
            controlTransfer(REQUEST_TYPE_CLASS_INTERFACE_OUT, HID_REQUEST_SET_REPORT, 0x0200,
                            _interfaces[i].number, &leds, 1);
            return;
        }
    }
}

String UsbKeyboardHost::stringFromDescriptor(const usb_str_desc_t* desc) {
    String result;
    if (desc == nullptr || desc->bLength < 2) {
        return result;
    }
    int chars = (desc->bLength - 2) / 2;
    for (int i = 0; i < chars; i++) {
        uint16_t c = desc->wData[i];
        result += (c >= 0x20 && c < 0x7F) ? (char)c : '?';
    }
    result.trim();
    return result;
}
