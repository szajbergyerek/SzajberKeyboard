#include "HidReportParser.h"

static constexpr uint16_t USAGE_PAGE_KEYBOARD = 0x07;
static constexpr uint16_t USAGE_PAGE_CONSUMER = 0x0C;

bool HidReportParser::parse(const uint8_t* desc, size_t len) {
    _fieldCount = 0;
    _usesReportIds = false;

    uint16_t usagePage = 0;
    int32_t logicalMin = 0;
    uint8_t reportSize = 0;
    uint8_t reportCount = 0;
    uint8_t reportId = 0;
    uint16_t bitOffsets[256] = {0};

    uint16_t localUsages[16];
    uint8_t localUsageCount = 0;
    uint16_t localUsagePage = 0;
    uint16_t usageMin = 0;

    size_t i = 0;
    while (i < len) {
        uint8_t prefix = desc[i];
        if (prefix == 0xFE) {
            // Long item: skip it entirely
            if (i + 1 >= len) {
                break;
            }
            i += 3 + desc[i + 1];
            continue;
        }
        uint8_t size = prefix & 0x03;
        if (size == 3) {
            size = 4;
        }
        uint8_t type = (prefix >> 2) & 0x03;
        uint8_t tag = prefix >> 4;
        if (i + 1 + size > len) {
            break;
        }

        uint32_t value = 0;
        for (uint8_t b = 0; b < size; b++) {
            value |= (uint32_t)desc[i + 1 + b] << (8 * b);
        }
        int32_t signedValue = (int32_t)value;
        if (size == 1) {
            signedValue = (int8_t)value;
        } else if (size == 2) {
            signedValue = (int16_t)value;
        }

        if (type == 0) {
            // Main item
            if (tag == 0x8) {
                bool isConstant = value & 0x01;
                bool isArray = !(value & 0x02);
                uint16_t fieldPage = localUsagePage ? localUsagePage : usagePage;
                bool relevant = fieldPage == USAGE_PAGE_KEYBOARD || fieldPage == USAGE_PAGE_CONSUMER;
                if (!isConstant && relevant && _fieldCount < MAX_FIELDS && reportSize > 0 && reportSize <= 16) {
                    Field& field = _fields[_fieldCount++];
                    field.reportId = reportId;
                    field.bitOffset = bitOffsets[reportId];
                    field.bitSize = reportSize;
                    field.count = reportCount;
                    field.isArray = isArray;
                    field.usagePage = fieldPage;
                    field.usageMin = localUsageCount ? 0 : usageMin;
                    field.logicalMin = logicalMin;
                    field.usageCount = localUsageCount;
                    memcpy(field.usages, localUsages, sizeof(uint16_t) * localUsageCount);
                }
                bitOffsets[reportId] += (uint16_t)reportSize * reportCount;
            }
            localUsageCount = 0;
            localUsagePage = 0;
            usageMin = 0;
        } else if (type == 1) {
            // Global item
            switch (tag) {
                case 0x0: usagePage = value; break;
                case 0x1: logicalMin = signedValue; break;
                case 0x7: reportSize = value; break;
                case 0x8:
                    reportId = value;
                    _usesReportIds = true;
                    break;
                case 0x9: reportCount = value; break;
                default: break;
            }
        } else if (type == 2) {
            // Local item
            switch (tag) {
                case 0x0:
                    if (size == 4) {
                        localUsagePage = value >> 16;
                    }
                    if (localUsageCount < 16) {
                        localUsages[localUsageCount++] = value & 0xFFFF;
                    }
                    break;
                case 0x1:
                    if (size == 4) {
                        localUsagePage = value >> 16;
                    }
                    usageMin = value & 0xFFFF;
                    break;
                default: break;
            }
        }
        i += 1 + size;
    }
    return _fieldCount > 0;
}

bool HidReportParser::hasKeyboardFields() const {
    for (size_t i = 0; i < _fieldCount; i++) {
        if (_fields[i].usagePage == USAGE_PAGE_KEYBOARD) {
            return true;
        }
    }
    return false;
}

bool HidReportParser::hasConsumerFields() const {
    for (size_t i = 0; i < _fieldCount; i++) {
        if (_fields[i].usagePage == USAGE_PAGE_CONSUMER) {
            return true;
        }
    }
    return false;
}

bool HidReportParser::extractKeyboard(const uint8_t* report, size_t len, uint8_t out[8]) const {
    uint8_t reportId;
    const uint8_t* payload;
    size_t payloadLen;
    splitReport(report, len, reportId, payload, payloadLen);

    bool found = false;
    uint8_t modifiers = 0;
    uint8_t keys[6] = {0};
    uint8_t keyCount = 0;

    auto addUsage = [&](uint16_t usage) {
        if (usage >= 0xE0 && usage <= 0xE7) {
            modifiers |= 1 << (usage - 0xE0);
        } else if (usage >= 0x04 && keyCount < 6) {
            keys[keyCount++] = (uint8_t)usage;
        }
    };

    for (size_t f = 0; f < _fieldCount; f++) {
        const Field& field = _fields[f];
        if (field.usagePage != USAGE_PAGE_KEYBOARD || field.reportId != reportId) {
            continue;
        }
        found = true;
        for (uint8_t e = 0; e < field.count; e++) {
            uint32_t value = readBits(payload, payloadLen, field.bitOffset + (uint32_t)e * field.bitSize, field.bitSize);
            if (field.isArray) {
                if (value != 0) {
                    addUsage(usageAt(field, (int32_t)value - field.logicalMin));
                }
            } else if (value != 0) {
                addUsage(usageAt(field, e));
            }
        }
    }

    if (found) {
        out[0] = modifiers;
        out[1] = 0;
        memcpy(out + 2, keys, 6);
    }
    return found;
}

bool HidReportParser::extractConsumer(const uint8_t* report, size_t len, uint16_t& usage) const {
    uint8_t reportId;
    const uint8_t* payload;
    size_t payloadLen;
    splitReport(report, len, reportId, payload, payloadLen);

    bool found = false;
    usage = 0;
    for (size_t f = 0; f < _fieldCount && usage == 0; f++) {
        const Field& field = _fields[f];
        if (field.usagePage != USAGE_PAGE_CONSUMER || field.reportId != reportId) {
            continue;
        }
        found = true;
        for (uint8_t e = 0; e < field.count; e++) {
            uint32_t value = readBits(payload, payloadLen, field.bitOffset + (uint32_t)e * field.bitSize, field.bitSize);
            if (value == 0) {
                continue;
            }
            usage = field.isArray ? usageAt(field, (int32_t)value - field.logicalMin) : usageAt(field, e);
            if (usage != 0) {
                break;
            }
        }
    }
    return found;
}

uint16_t HidReportParser::usageAt(const Field& field, int32_t index) {
    if (index < 0) {
        return 0;
    }
    if (field.usageCount > 0) {
        if (index >= field.usageCount) {
            index = field.usageCount - 1;
        }
        return field.usages[index];
    }
    return field.usageMin + index;
}

uint32_t HidReportParser::readBits(const uint8_t* data, size_t len, uint32_t bitOffset, uint8_t bitSize) {
    uint32_t value = 0;
    for (uint8_t b = 0; b < bitSize; b++) {
        uint32_t bit = bitOffset + b;
        if (bit / 8 >= len) {
            return 0;
        }
        if (data[bit / 8] & (1 << (bit % 8))) {
            value |= 1UL << b;
        }
    }
    return value;
}

void HidReportParser::splitReport(const uint8_t* report, size_t len, uint8_t& reportId, const uint8_t*& payload,
                                  size_t& payloadLen) const {
    if (_usesReportIds && len > 0) {
        reportId = report[0];
        payload = report + 1;
        payloadLen = len - 1;
    } else {
        reportId = 0;
        payload = report;
        payloadLen = len;
    }
}
