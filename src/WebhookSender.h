#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

/**
 * Sends HTTP requests triggered by key rules.
 * The request runs in its own task, so a slow or unreachable server never delays
 * the keyboard: send() only copies the request into a queue and returns at once.
 * Plain http:// targets only, https:// is rejected.
 */
class WebhookSender {
public:
    static constexpr size_t MAX_URL = 160;
    static constexpr size_t MAX_HEADER = 128;
    static constexpr size_t MAX_BODY = 192;

    /**
     * Create the queue and start the worker task.
     *
     * @return void
     */
    void begin();

    /**
     * Queue one request.
     *
     * @param method "GET" or "POST", anything else is treated as GET.
     * @param url Target URL, must start with http://.
     * @param header Optional single header line in "Name: value" form.
     * @param body Optional request body, only used for POST.
     * @return true if the request was queued.
     */
    bool send(const String& method, const String& url, const String& header, const String& body);

private:
    struct Request {
        char method[8];
        char url[MAX_URL];
        char header[MAX_HEADER];
        char body[MAX_BODY];
    };

    /**
     * Task that performs the queued requests one after the other.
     *
     * @param arg Pointer to the WebhookSender instance.
     * @return void
     */
    static void workerTask(void* arg);

    /**
     * Perform one request and log the result.
     *
     * @param request The request to run.
     * @return void
     */
    static void perform(const Request& request);

    QueueHandle_t _queue = nullptr;
};
