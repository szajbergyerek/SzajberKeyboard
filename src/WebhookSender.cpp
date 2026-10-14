#include "WebhookSender.h"

#include <HTTPClient.h>
#include <WiFi.h>

static constexpr uint32_t REQUEST_TIMEOUT_MS = 4000;

void WebhookSender::begin() {
    _queue = xQueueCreate(4, sizeof(Request));
    if (_queue == nullptr) {
        Serial.println("[HOOK] Could not create the request queue");
        return;
    }
    xTaskCreatePinnedToCore(workerTask, "webhook", 8192, this, 3, nullptr, 1);
    Serial.println("[HOOK] Webhook sender ready");
}

bool WebhookSender::send(const String& method, const String& url, const String& header, const String& body) {
    if (_queue == nullptr || url.length() == 0) {
        return false;
    }
    if (url.length() >= MAX_URL || header.length() >= MAX_HEADER || body.length() >= MAX_BODY) {
        Serial.println("[HOOK] Request dropped, a field is too long");
        return false;
    }

    Request request;
    snprintf(request.method, sizeof(request.method), "%s", method.length() ? method.c_str() : "GET");
    snprintf(request.url, sizeof(request.url), "%s", url.c_str());
    snprintf(request.header, sizeof(request.header), "%s", header.c_str());
    snprintf(request.body, sizeof(request.body), "%s", body.c_str());

    if (xQueueSend(_queue, &request, 0) != pdTRUE) {
        Serial.println("[HOOK] Request dropped, the queue is full");
        return false;
    }
    return true;
}

void WebhookSender::workerTask(void* arg) {
    WebhookSender* self = static_cast<WebhookSender*>(arg);
    Request request;
    while (true) {
        if (xQueueReceive(self->_queue, &request, portMAX_DELAY) == pdTRUE) {
            perform(request);
        }
    }
}

void WebhookSender::perform(const Request& request) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HOOK] Request skipped, WiFi is not connected");
        return;
    }
    if (strncmp(request.url, "http://", 7) != 0) {
        Serial.printf("[HOOK] Request skipped, only http:// is supported: %s\n", request.url);
        return;
    }

    HTTPClient http;
    http.setTimeout(REQUEST_TIMEOUT_MS);
    http.setConnectTimeout(REQUEST_TIMEOUT_MS);
    if (!http.begin(request.url)) {
        Serial.println("[HOOK] Could not parse the URL");
        return;
    }

    bool hasContentType = false;
    String header(request.header);
    int colon = header.indexOf(':');
    if (colon > 0) {
        String name = header.substring(0, colon);
        String value = header.substring(colon + 1);
        name.trim();
        value.trim();
        hasContentType = name.equalsIgnoreCase("Content-Type");
        http.addHeader(name, value);
    }

    bool isPost = strcmp(request.method, "POST") == 0;
    if (isPost && !hasContentType) {
        http.addHeader("Content-Type", "application/json");
    }

    int code = isPost ? http.POST(String(request.body)) : http.GET();
    if (code > 0) {
        Serial.printf("[HOOK] %s %s -> %d\n", request.method, request.url, code);
    } else {
        Serial.printf("[HOOK] %s %s failed: %s\n", request.method, request.url, http.errorToString(code).c_str());
    }
    http.end();
}
