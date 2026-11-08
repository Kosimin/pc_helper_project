// commands.c  (ESP-IDF v5.x)
//
// GET /api/command  ->  {"volume": 0-100, "lock_seq": N}
//
// The laptop polls this a few times a second:
//   * when "volume" changes, it sets the laptop volume to that value
//   * when "lock_seq" goes up, it locks the laptop
// Every request must carry the header  X-Auth-Token: <AUTH_TOKEN>.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "command.h"
#include "volume.h"        // get_volume()

#define AUTH_TOKEN "45094509"   // must match TOKEN in command_sync.py

static const char *TAG = "COMMANDS";

// Incremented once per confirmed lock request. A counter (not a flag) means a
// missed reply can never cause a double lock or a lost lock.
static volatile int lock_seq = 0;

void commands_request_lock(void)
{
    lock_seq++;
    ESP_LOGI(TAG, "Lock requested (seq %d)", lock_seq);
}

static bool authorized(httpd_req_t *req)
{
    char token[48] = {0};
    if (httpd_req_get_hdr_value_str(req, "X-Auth-Token", token, sizeof(token)) != ESP_OK)
        return false;
    return strcmp(token, AUTH_TOKEN) == 0;
}

static esp_err_t command_get_handler(httpd_req_t *req)
{
    if (!authorized(req))
    {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"error\":\"unauthorized\"}", HTTPD_RESP_USE_STRLEN);
    }

    char body[64];
    snprintf(body, sizeof(body), "{\"volume\":%d,\"lock_seq\":%d}", get_volume(), (int)lock_seq);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

void commands_register(httpd_handle_t server)
{
    static const httpd_uri_t uri = {
        .uri      = "/api/command",
        .method   = HTTP_GET,
        .handler  = command_get_handler,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri));
}