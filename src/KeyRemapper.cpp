#include "KeyRemapper.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

static const char* KEYMAP_PATH = "/keymap.json";

// Mirrors the buffer sizes of WebhookSender, longer values are rejected on save
static constexpr size_t MAX_URL_LEN = 159;
static constexpr size_t MAX_HEADER_LEN = 127;
static constexpr size_t MAX_BODY_LEN = 191;

/**
 * Append a string as a quoted JSON value, escaping what the format requires.
 *
 * @param out Document being built.
 * @param value Raw string value.
 * @return void
 */
static void appendJsonString(String& out, const String& value) {
    out += (char)34;
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (c == (char)34 || c == (char)92) {
            out += (char)92;
            out += c;
        } else if (c >= 0 && c < 0x20) {
            char escaped[8];
            snprintf(escaped, sizeof(escaped), R"(\u%04x)", (unsigned int)c);
            out += escaped;
        } else {
            out += c;
        }
    }
    out += (char)34;
}

void KeyRemapper::begin() {
    _fsReady = LittleFS.begin(true);
    if (!_fsReady) {
        Serial.println("[KEYMAP] LittleFS mount failed, rules cannot be stored");
        return;
    }
    load();
    Serial.printf("[KEYMAP] %u rule(s) loaded\n", (unsigned int)_count);
}

void KeyRemapper::apply(const uint8_t in[8], uint8_t out[8]) {
    uint8_t source[8];
    memcpy(source, in, 8);
    memcpy(out, source, 8);
    if (_learnMode || _count == 0) {
        memcpy(_previousKeys, source, 8);
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
        bool held = false;
        for (uint8_t previous = 2; previous < 8; previous++) {
            held |= _previousKeys[previous] == key;
        }
        if (!held && rule->url.length() > 0 && _actionHandler) {
            _actionHandler(*rule);
        }

        out[slot] = rule->outKey;
        if (rule->mods == 0) {
            out[0] |= rule->outMods;
        } else if (!modsReplaced) {
            out[0] = rule->outMods;
            modsReplaced = true;
        }
    }
    memcpy(_previousKeys, source, 8);
}

void KeyRemapper::setActionHandler(ActionHandler handler) {
    _actionHandler = handler;
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
        const char* url = rule["url"] | "";
        const char* method = rule["method"] | "";
        const char* header = rule["header"] | "";
        const char* body = rule["body"] | "";
        if (strlen(url) > MAX_URL_LEN || strlen(header) > MAX_HEADER_LEN || strlen(body) > MAX_BODY_LEN) {
            error = "url, header or body too long";
            return false;
        }

        _rules[count].mods = (uint8_t)mods;
        _rules[count].key = (uint8_t)key;
        _rules[count].outMods = (uint8_t)outMods;
        _rules[count].outKey = (uint8_t)outKey;
        _rules[count].url = url;
        _rules[count].method = strcmp(method, "POST") == 0 ? "POST" : "GET";
        _rules[count].header = header;
        _rules[count].body = body;
        count++;
    }
    _count = count;
    return true;
}

String KeyRemapper::toJson() const {
    String out;
    out.reserve(64 + _count * 96);
    out += R"({"learn":)";
    out += _learnMode ? "true" : "false";
    out += R"(,"rules":[)";
    for (size_t i = 0; i < _count; i++) {
        char item[72];
        snprintf(item, sizeof(item), R"(%s{"mods":%u,"key":%u,"outMods":%u,"outKey":%u)", i ? "," : "",
                 (unsigned int)_rules[i].mods, (unsigned int)_rules[i].key, (unsigned int)_rules[i].outMods,
                 (unsigned int)_rules[i].outKey);
        out += item;
        if (_rules[i].url.length() > 0) {
            out += R"(,"url":)";
            appendJsonString(out, _rules[i].url);
            out += R"(,"method":)";
            appendJsonString(out, _rules[i].method);
            if (_rules[i].header.length() > 0) {
                out += R"(,"header":)";
                appendJsonString(out, _rules[i].header);
            }
            if (_rules[i].body.length() > 0) {
                out += R"(,"body":)";
                appendJsonString(out, _rules[i].body);
            }
        }
        out += "}";
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
