#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

#include "VibrationMotor.h"

/**
 * Small HTTP API that triggers the vibration motor.
 * The server is started as soon as WiFi is up and serves:
 *   GET /vibrate?times=2&on=1000&off=300  start a pattern
 *   GET /stop                             stop the motor
 *   GET /                                 short usage text
 */
class HttpApi {
public:
    using ActivityHandler = std::function<void()>;

    /**
     * Create the API.
     *
     * @param port TCP port to listen on.
     * @param motor Motor driver the endpoints control.
     */
    HttpApi(uint16_t port, VibrationMotor& motor);

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

    WebServer _server;
    VibrationMotor& _motor;
    ActivityHandler _activityHandler;
    bool _started = false;
};
