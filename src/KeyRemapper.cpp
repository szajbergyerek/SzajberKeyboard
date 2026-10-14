#include "KeyRemapper.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

static const char* KEYMAP_PATH = "/keymap.json";

void KeyRemapper::begin() {
    _fsReady = LittleFS.begin(true);
    if (!_fsReady) {
        Serial.println("[KEYMAP] LittleFS mount failed, rules cannot be stored");
        return;
    }
    load();
    Serial.printf("[KEYMAP] %u rule(s) loaded\n", (unsigned int)_count);
}

void KeyRemapper::apply(const uint8_t in[8], uint8_t out[8]) const {
    uint8_t source[8];
    memcpy(source, in, 8);
    memcpy(out, source, 8);
    if (_learnMode || _count == 0) {
        return;
    }

    bool modsReplaced = false;
    for (uint8_t slot = 2; slot < 8; slot++) {
        uint8_t key = source[slot];
        if (key == 0) {
            continue;
        }
        const KeyRule* rule = findRule(source[0], key);
        if (rule == nullptr) {
            continue;
        }
        out[slot] = rule->outKey;
        if (rule->mods == 0) {
            out[0] |= rule->outMods;
        } else if (!modsReplaced) {
            out[0] = rule->outMods;
            modsReplaced = true;
        }
    }
}

const KeyRule* KeyRemapper::findRule(uint8_t mods, uint8_t key) const {
    for (size_t i = 0; i < _count; i++) {
        if (_rules[i].mods != 0 && _rules[i].mods == mods && _rules[i].key == key) {
            return &_rules[i];
        }
    }
    for (size_t i = 0; i < _count; i++) {
        if (_rules[i].mods == 0 && _rules[i].key == key) {
            return &_rules[i];
        }
    }
    return nullptr;
}

void KeyRemapper::setLearnMode(bool on) {
    _learnMode = on;
    Serial.printf("[KEYMAP] Learn mode %s\n", on ? "on, rules bypassed" : "off");
}

bool KeyRemapper::isLearnMode() const {
    return _learnMode;
}

bool KeyRemapper::loadFromJson(const String& json, String& error) {
    if (!parse(json, error)) {
        return false;
    }
    if (!save()) {
        error = "could not write the file";
        return false;
    }
    Serial.printf("[KEYMAP] %u rule(s) saved\n", (unsigned int)_count);
    return true;
}

bool KeyRemapper::parse(const String& json, String& error) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        error = "invalid json";
        return false;
    }
    JsonArrayConst rules = doc["rules"].as<JsonArrayConst>();
    if (rules.isNull()) {
        error = "missing rules array";
        return false;
    }
    if (rules.size() > MAX_RULES) {
        error = "too many rules";
        return false;
    }

    size_t count = 0;
    for (JsonObjectConst rule : rules) {
        int key = rule["key"] | 0;
        int mods = rule["mods"] | 0;
        int outKey = rule["outKey"] | 0;
        int outMods = rule["outMods"] | 0;
        if (key <= 0 || key > 255 || mods < 0 || mods > 255 || outKey < 0 || outKey > 255 || outMods < 0 ||
            outMods > 255) {
            error = "value out of range";
            return false;
        }
        _rules[count].mods = (uint8_t)mods;
        _rules[count].key = (uint8_t)key;
        _rules[count].outMods = (uint8_t)outMods;
        _rules[count].outKey = (uint8_t)outKey;
        count++;
    }
    _count = count;
    return true;
}

String KeyRemapper::toJson() const {
    String out;
    out.reserve(64 + _count * 56);
    out += "{\"learn\":";
    out += _learnMode ? "true" : "false";
    out += ",\"rules\":[";
    for (size_t i = 0; i < _count; i++) {
        char item[64];
        snprintf(item, sizeof(item), "%s{\"mods\":%u,\"key\":%u,\"outMods\":%u,\"outKey\":%u}", i ? "," : "",
                 (unsigned int)_rules[i].mods, (unsigned int)_rules[i].key, (unsigned int)_rules[i].outMods,
                 (unsigned int)_rules[i].outKey);
        out += item;
    }
    out += "]}";
    return out;
}

size_t KeyRemapper::ruleCount() const {
    return _count;
}

bool KeyRemapper::save() const {
    if (!_fsReady) {
        return false;
    }
    File file = LittleFS.open(KEYMAP_PATH, "w");
    if (!file) {
        return false;
    }
    String json = toJson();
    size_t written = file.print(json);
    file.close();
    return written == json.length();
}

bool KeyRemapper::load() {
    if (!_fsReady || !LittleFS.exists(KEYMAP_PATH)) {
        return true;
    }
    File file = LittleFS.open(KEYMAP_PATH, "r");
    if (!file) {
        return false;
    }
    String json = file.readString();
    file.close();

    String error;
    if (!parse(json, error)) {
        Serial.printf("[KEYMAP] Stored rules rejected: %s\n", error.c_str());
        _count = 0;
        return false;
    }
    return true;
}
