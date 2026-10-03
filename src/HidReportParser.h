#pragma once

#include <Arduino.h>

/**
 * Minimal HID report descriptor parser. It locates keyboard (usage page 0x07) and
 * consumer control (usage page 0x0C) input fields so report protocol interfaces
 * (multimedia keys, NKRO key bitmaps) can be translated.
 */
class HidReportParser {
public:
    /**
     * Parse a HID report descriptor and remember the relevant input fields.
     *
     * @param desc Report descriptor bytes.
     * @param len Length of the descriptor.
     * @return true if at least one keyboard or consumer field was found.
     */
    bool parse(const uint8_t* desc, size_t len);

    /**
     * Check whether keyboard fields were found.
     *
     * @return true if the descriptor contains keyboard input fields.
     */
    bool hasKeyboardFields() const;

    /**
     * Check whether consumer control fields were found.
     *
     * @return true if the descriptor contains consumer control input fields.
     */
    bool hasConsumerFields() const;

    /**
     * Translate an input report into an 8 byte boot keyboard report.
     *
     * @param report Raw input report (including the report ID if the device uses IDs).
     * @param len Length of the report.
     * @param out Destination for the boot report (modifiers, reserved, 6 key codes).
     * @return true if the report contained keyboard fields.
     */
    bool extractKeyboard(const uint8_t* report, size_t len, uint8_t out[8]) const;

    /**
     * Get the first pressed consumer control usage of an input report.
     *
     * @param report Raw input report (including the report ID if the device uses IDs).
     * @param len Length of the report.
     * @param usage Receives the pressed usage, or 0 if nothing is pressed.
     * @return true if the report contained consumer control fields.
     */
    bool extractConsumer(const uint8_t* report, size_t len, uint16_t& usage) const;

private:
    struct Field {
        uint8_t reportId;
        uint16_t bitOffset;
        uint8_t bitSize;
        uint8_t count;
        bool isArray;
        uint16_t usagePage;
        uint16_t usageMin;
        int32_t logicalMin;
        uint8_t usageCount;
        uint16_t usages[16];
    };

    /**
     * Resolve the usage belonging to a field element.
     *
     * @param field The field description.
     * @param index Element index (variable fields) or logical value (array fields).
     * @return The usage ID.
     */
    static uint16_t usageAt(const Field& field, int32_t index);

    /**
     * Read an unsigned little endian bit field from a buffer.
     *
     * @param data Buffer to read from.
     * @param len Buffer length in bytes.
     * @param bitOffset Offset of the first bit.
     * @param bitSize Number of bits (1 to 32).
     * @return The value, or 0 if it lies outside the buffer.
     */
    static uint32_t readBits(const uint8_t* data, size_t len, uint32_t bitOffset, uint8_t bitSize);

    /**
     * Split a raw report into report ID and payload.
     *
     * @param report Raw input report.
     * @param len Length of the report.
     * @param reportId Receives the report ID (0 if the device does not use IDs).
     * @param payload Receives a pointer to the data after the report ID.
     * @param payloadLen Receives the payload length.
     * @return void
     */
    void splitReport(const uint8_t* report, size_t len, uint8_t& reportId, const uint8_t*& payload,
                     size_t& payloadLen) const;

    static constexpr size_t MAX_FIELDS = 16;
    Field _fields[MAX_FIELDS];
    size_t _fieldCount = 0;
    bool _usesReportIds = false;
};
