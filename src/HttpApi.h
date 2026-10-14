#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

#include "KeyRemapper.h"
#include "VibrationMotor.h"

/**
 * Small HTTP API that triggers the vibration motor.
 * The server is started as soon as WiFi is up and serves:
 *   GET  /vibrate?times=2&on=1000&off=300  start a pattern
 *   GET  /stop                             stop the motor
 *   GET  /keymap                           the key remapping editor page
 *   GET  /keymap.json                      the stored rules
 *   POST /keymap.json                      replace the rules
 *   GET  /learn?on=1                       bypass the rules while capturing keys
 *   GET  /                                 short usage text
 */
class HttpApi {
public:
    using ActivityHandler = std::function<void()>;

    /**
     * Create the API.
     *
     * @param port TCP port to listen on.
     * @param motor Motor driver the endpoints control.
     * @param remapper Key remapper the editor endpoints configure.
     */
    HttpApi(uint16_t port, VibrationMotor& motor, KeyRemapper& remapper);

    /**
     * Register the request handlers. The socket is opened later, once WiFi is connected.
     *
     * @return void
     */
    void begin();

    /**
     * Start the server when WiFi comes up and serve pending requests. Call from loop().
     *
     * @return void
     */
    void handle();

    /**
     * Set the handler called on every accepted request, used to reset the idle timer.
     *
     * @param handler Called before the request is answered.
     * @return void
     */
    void setActivityHandler(ActivityHandler handler);

private:
    /**
     * Read a positive integer query argument.
     *
     * @param name Argument name.
     * @param fallback Value returned when the argument is missing or not a number.
     * @return The parsed value, or the fallback.
     */
    uint16_t argValue(const char* name, uint16_t fallback);

    /**
     * Handle GET /vibrate.
     *
     * @return void
     */
    void handleVibrate();

    /**
     * Handle GET /stop.
     *
     * @return void
     */
    void handleStop();

    /**
     * Handle GET / with a short usage text.
     *
     * @return void
     */
    void handleRoot();

    /**
     * Handle GET /keymap, the editor page.
     *
     * @return void
     */
    void handleKeymapPage();

    /**
     * Handle GET /keymap.json.
     *
     * @return void
     */
    void handleKeymapGet();

    /**
     * Handle POST /keymap.json.
     *
     * @return void
     */
    void handleKeymapPost();

    /**
     * Handle GET /learn.
     *
     * @return void
     */
    void handleLearn();

    WebServer _server;
    VibrationMotor& _motor;
    KeyRemapper& _remapper;
    ActivityHandler _activityHandler;
    bool _started = false;
};
