#pragma once

#include <Arduino.h>
#include <functional>

/**
 * One remapping rule. Modifiers are HID bitmasks (1 left Ctrl, 2 left Shift,
 * 4 left Alt, 8 left Win, 16 right Ctrl, 32 right Shift, 64 right Alt, 128 right Win),
 * keys are HID usage IDs of the keyboard page.
 * When url is set, pressing the trigger also fires an HTTP request.
 */
struct KeyRule {
    uint8_t mods;
    uint8_t key;
    uint8_t outMods;
    uint8_t outKey;
    String url;
    String method;
    String header;
    String body;
};

/**
 * Rewrites keyboard reports between the USB keyboard and the BLE host.
 * A rule with mods 0 matches the key whatever modifiers are held and keeps them,
 * a rule with a modifier mask only matches that exact combination and replaces the
 * modifiers as well. An output key of 0 swallows the key.
 * A rule may also carry an HTTP request, fired once on every new key press.
 * Rules are stored as JSON on LittleFS and survive a restart.
 */
class KeyRemapper {
public:
    using ActionHandler = std::function<void(const KeyRule& rule)>;

    static constexpr size_t MAX_RULES = 32;

    /**
     * Mount the filesystem and load the stored rules.
     *
     * @return void
     */
    void begin();

    /**
     * Rewrite one 8 byte boot keyboard report. In learn mode the report is passed through
     * unchanged so the editor page can capture the original keys.
     *
     * @param in Report coming from the USB keyboard.
     * @param out Report to send to the BLE host, may be the same buffer as in.
     * @return void
     */
    void apply(const uint8_t in[8], uint8_t out[8]);

    /**
     * Set the handler called when a rule carrying an HTTP request is pressed.
     *
     * @param handler Called from the USB task, so it must not block.
     * @return void
     */
    void setActionHandler(ActionHandler handler);

    /**
     * Switch the learn mode, which bypasses all rules.
     *
     * @param on True to bypass the rules.
     * @return void
     */
    void setLearnMode(bool on);

    /**
     * Check whether the rules are currently bypassed.
     *
     * @return true while learn mode is on.
     */
    bool isLearnMode() const;

    /**
     * Replace every rule from a JSON document and store it on the filesystem.
     *
     * @param json Document of the form {"rules":[{"mods":1,"key":6,"outMods":0,"outKey":62}]}.
     * @param error Receives a short error message when the document is rejected.
     * @return true if the rules were accepted and saved.
     */
    bool loadFromJson(const String& json, String& error);

    /**
     * Serialize the current state for the editor page.
     *
     * @return JSON document with the learn mode flag and the rules.
     */
    String toJson() const;

    /**
     * Number of rules currently active.
     *
     * @return The rule count.
     */
    size_t ruleCount() const;

private:
    /**
     * Find the rule that applies to one pressed key.
     * An exact modifier match wins over a plain key rule.
     *
     * @param mods Modifier bitmask of the incoming report.
     * @param key HID usage ID of the pressed key.
     * @return The matching rule, or nullptr.
     */
    const KeyRule* findRule(uint8_t mods, uint8_t key) const;

    /**
     * Replace every rule from a JSON document without touching the filesystem.
     *
     * @param json Document holding the rules array.
     * @param error Receives a short error message when the document is rejected.
     * @return true if the rules were accepted.
     */
    bool parse(const String& json, String& error);

    /**
     * Write the rules to /keymap.json.
     *
     * @return true on success.
     */
    bool save() const;

    /**
     * Read the rules from /keymap.json, missing file is not an error.
     *
     * @return true if the file was read or simply absent.
     */
    bool load();

    KeyRule _rules[MAX_RULES];
    size_t _count = 0;
    ActionHandler _actionHandler;
    uint8_t _previousKeys[8] = {0};
    volatile bool _learnMode = false;
    bool _fsReady = false;
};
