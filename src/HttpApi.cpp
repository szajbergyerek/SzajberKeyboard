#include "HttpApi.h"

#include <WiFi.h>

#include "KeymapPage.h"

static constexpr uint16_t DEFAULT_PULSES = 1;
static constexpr uint16_t DEFAULT_ON_MS = 200;
static constexpr uint16_t DEFAULT_OFF_MS = 200;

HttpApi::HttpApi(uint16_t port, VibrationMotor& motor, KeyRemapper& remapper, DeviceSettings& settings)
    : _server(port), _motor(motor), _remapper(remapper), _settings(settings) {}

void HttpApi::begin() {
    _server.on("/vibrate", [this]() { handleVibrate(); });
    _server.on("/stop", [this]() { handleStop(); });
    _server.on("/keymap", [this]() { handleKeymapPage(); });
    _server.on("/keymap.json", HTTP_GET, [this]() { handleKeymapGet(); });
    _server.on("/keymap.json", HTTP_POST, [this]() { handleKeymapPost(); });
    _server.on("/learn", [this]() { handleLearn(); });
    _server.on("/restart", [this]() { handleRestart(); });
    _server.on("/config.json", HTTP_GET, [this]() { handleConfigGet(); });
    _server.on("/config.json", HTTP_POST, [this]() { handleConfigPost(); });
    _server.on("/", [this]() { handleRoot(); });
}

void HttpApi::handle() {
    if (!_started) {
        if (WiFi.status() != WL_CONNECTED) {
            return;
        }
        _server.begin();
        _started = true;
        Serial.printf("[API] HTTP API ready at http://%s/\n", WiFi.localIP().toString().c_str());
        return;
    }
    _server.handleClient();
}

void HttpApi::setActivityHandler(ActivityHandler handler) {
    _activityHandler = handler;
}

uint16_t HttpApi::argValue(const char* name, uint16_t fallback) {
    if (!_server.hasArg(name)) {
        return fallback;
    }
    long value = _server.arg(name).toInt();
    if (value < 0 || value > 65535) {
        return fallback;
    }
    return (uint16_t)value;
}

void HttpApi::handleVibrate() {
    if (_activityHandler) {
        _activityHandler();
    }

    uint16_t times = argValue("times", DEFAULT_PULSES);
    uint16_t onMs = argValue("on", DEFAULT_ON_MS);
    uint16_t offMs = argValue("off", DEFAULT_OFF_MS);

    if (!_motor.play(times, onMs, offMs)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"value out of range\"}");
        return;
    }

    char body[96];
    snprintf(body, sizeof(body), "{\"ok\":true,\"times\":%u,\"on\":%u,\"off\":%u}", (unsigned int)times,
             (unsigned int)onMs, (unsigned int)offMs);
    _server.send(200, "application/json", body);
}

void HttpApi::handleStop() {
    if (_activityHandler) {
        _activityHandler();
    }
    _motor.stop();
    _server.send(200, "application/json", "{\"ok\":true,\"stopped\":true}");
}

void HttpApi::handleRoot() {
    _server.send(200, "text/plain",
                 "Szajber Keyboard\n"
                 "GET /vibrate?times=2&on=1000&off=300\n"
                 "GET /stop\n"
                 "GET /keymap\n"
                 "GET /restart\n"
                 "GET /config.json\n");
}

void HttpApi::handleConfigGet() {
    _server.send(200, "application/json", _settings.toJson());
}

void HttpApi::handleConfigPost() {
    String body = _server.arg("plain");
    int key = body.indexOf("idleTimeoutMin");
    if (key < 0) {
        _server.send(400, "application/json", R"({"ok":false,"error":"missing idleTimeoutMin"})");
        return;
    }
    int colon = body.indexOf(':', key);
    if (colon < 0) {
        _server.send(400, "application/json", R"({"ok":false,"error":"malformed request"})");
        return;
    }
    long minutes = body.substring(colon + 1).toInt();
    if (minutes <= 0 || minutes > 65535) {
        _server.send(400, "application/json", R"({"ok":false,"error":"value out of range"})");
        return;
    }

    String error;
    if (!_settings.setIdleTimeoutMin((uint16_t)minutes, error)) {
        _server.send(400, "application/json", String(R"({"ok":false,"error":")") + error + R"("})");
        return;
    }
    _server.send(200, "application/json", String(R"({"ok":true,)") + _settings.toJson().substring(1));
}

void HttpApi::handleKeymapPage() {
    _server.send_P(200, "text/html", KEYMAP_PAGE);
}

void HttpApi::handleKeymapGet() {
    _server.send(200, "application/json", _remapper.toJson(true));
}

void HttpApi::handleKeymapPost() {
    String body = _server.arg("plain");
    String error;
    if (!_remapper.loadFromJson(body, error)) {
        _server.send(400, "application/json", String(R"({"ok":false,"error":")") + error + R"("})");
        return;
    }
    char response[48];
    snprintf(response, sizeof(response), R"({"ok":true,"count":%u})", (unsigned int)_remapper.ruleCount());
    _server.send(200, "application/json", response);
}

void HttpApi::handleLearn() {
    bool on = argValue("on", 0) != 0;
    _remapper.setLearnMode(on);
    _server.send(200, "application/json",
                 on ? R"({"ok":true,"learn":true})" : R"({"ok":true,"learn":false})");
}

void HttpApi::handleRestart() {
    _server.send(200, "application/json", R"({"ok":true,"restarting":true})");
    _server.client().stop();
    Serial.println("[API] Restart requested over HTTP");
    Serial.flush();
    delay(200);
    ESP.restart();
}
