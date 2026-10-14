#include "HttpApi.h"

#include <WiFi.h>

static constexpr uint16_t DEFAULT_PULSES = 1;
static constexpr uint16_t DEFAULT_ON_MS = 200;
static constexpr uint16_t DEFAULT_OFF_MS = 200;

HttpApi::HttpApi(uint16_t port, VibrationMotor& motor) : _server(port), _motor(motor) {}

void HttpApi::begin() {
    _server.on("/vibrate", [this]() { handleVibrate(); });
    _server.on("/stop", [this]() { handleStop(); });
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
                 "GET /stop\n");
}
