#include "DeviceSettings.h"

#include <LittleFS.h>

static const char* CONFIG_PATH = "/config.json";

DeviceSettings::DeviceSettings(uint16_t defaultMinutes, uint16_t minMinutes, uint16_t maxMinutes)
    : _idleTimeoutMin(defaultMinutes), _minMinutes(minMinutes), _maxMinutes(maxMinutes) {}

void DeviceSettings::begin() {
    _fsReady = LittleFS.begin(true);
    if (!_fsReady) {
        Serial.println("[CONFIG] LittleFS mount failed, using the built in idle timeout");
        return;
    }
    load();
    Serial.printf("[CONFIG] Idle timeout: %u min\n", (unsigned int)_idleTimeoutMin);
}

uint32_t DeviceSettings::idleTimeoutMs() const {
    return (uint32_t)_idleTimeoutMin * 60000UL;
}

void DeviceSettings::setChangeHandler(ChangeHandler handler) {
    _changeHandler = handler;
}

bool DeviceSettings::setIdleTimeoutMin(uint16_t minutes, String& error) {
    if (minutes < _minMinutes || minutes > _maxMinutes) {
        error = "value out of range";
        return false;
    }
    _idleTimeoutMin = minutes;
    if (!save()) {
        error = "could not write the file";
        return false;
    }
    Serial.printf("[CONFIG] Idle timeout set to %u min\n", (unsigned int)_idleTimeoutMin);
    if (_changeHandler) {
        _changeHandler(idleTimeoutMs());
    }
    return true;
}

String DeviceSettings::toJson() const {
    char body[48];
    snprintf(body, sizeof(body), R"({"idleTimeoutMin":%u})", (unsigned int)_idleTimeoutMin);
    return String(body);
}

bool DeviceSettings::save() const {
    if (!_fsReady) {
        return false;
    }
    File file = LittleFS.open(CONFIG_PATH, "w");
    if (!file) {
        return false;
    }
    String json = toJson();
    size_t written = file.print(json);
    file.close();
    return written == json.length();
}

void DeviceSettings::load() {
    if (!_fsReady || !LittleFS.exists(CONFIG_PATH)) {
        return;
    }
    File file = LittleFS.open(CONFIG_PATH, "r");
    if (!file) {
        return;
    }
    String json = file.readString();
    file.close();

    int key = json.indexOf("idleTimeoutMin");
    if (key < 0) {
        return;
    }
    int colon = json.indexOf(':', key);
    if (colon < 0) {
        return;
    }
    long value = json.substring(colon + 1).toInt();
    if (value < _minMinutes || value > _maxMinutes) {
        Serial.println("[CONFIG] Stored idle timeout out of range, ignored");
        return;
    }
    _idleTimeoutMin = (uint16_t)value;
}
