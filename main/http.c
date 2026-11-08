#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "display.h"
#include "http.h"
#include "command.h"

static const char *TAG = "STATS";

#define MAX_REQUEST_SIZE 1024
#define MAX_PROCESSES 5
#define MAX_PROCESS_NAME 20


// Skip JSON whitespace.
static const char *skip_spaces(const char *p)
{
    while (*p == ' ' || *p == '\t' ||
           *p == '\n' || *p == '\r') {
        p++;
    }

    return p;
}


// Find a named field in a simple JSON object.
static const char *find_json_key(const char *json, const char *key)
{
    char search_key[32];

    int n = snprintf(search_key, sizeof(search_key),
                     "\"%s\"", key);

    if (n < 0 || (size_t)n >= sizeof(search_key)) {
        return NULL;
    }

    const char *p = strstr(json, search_key);

    if (p == NULL) {
        return NULL;
    }

    p += strlen(search_key);
    p = skip_spaces(p);

    if (*p != ':') {
        return NULL;
    }

    return skip_spaces(p + 1);
}


// Parse an integer in the range 0-100.
static bool parse_integer(const char **p, int *value)
{
    const char *start = *p;

    if (*start < '0' || *start > '9') {
        return false;
    }

    errno = 0;

    char *end;
    long number = strtol(start, &end, 10);

    if (errno == ERANGE || end == start ||
        number < 0 || number > 100) {
        return false;
    }

    if (*end == '.' || *end == 'e' || *end == 'E' ||
        (*end >= '0' && *end <= '9')) {
        return false;
    }

    *p = skip_spaces(end);

    if (**p != ',' && **p != '}') {
        return false;
    }

    *value = (int)number;
    return true;
}


// Extract an integer field from the top-level object.
static bool get_json_integer(const char *json,
                             const char *key,
                             int *value)
{
    const char *p = find_json_key(json, key);

    if (p == NULL) {
        return false;
    }

    return parse_integer(&p, value);
}


// Parse a process object: {"name":"chrome.exe","cpu":12}
static bool parse_process(const char **p,
                         proocess_info_t *process)
{
    *p = skip_spaces(*p);

    if (**p != '{') {
        return false;
    }

    (*p)++;
    *p = skip_spaces(*p);

    // Read the process name.
    if (strncmp(*p, "\"name\"", 6) != 0) {
        return false;
    }

    *p += 6;
    *p = skip_spaces(*p);

    if (**p != ':') {
        return false;
    }

    (*p)++;
    *p = skip_spaces(*p);

    if (**p != '"') {
        return false;
    }

    (*p)++;

    size_t name_len = 0;

    while (**p != '\0' && **p != '"') {
        // Reject escaped strings in this lightweight parser.
        if (**p == '\\' || (unsigned char)**p < 32) {
            return false;
        }

        if (name_len >= MAX_PROCESS_NAME - 1) {
            return false;
        }

        process->process_name[name_len++] = **p;
        (*p)++;
    }

    if (**p != '"') {
        return false;
    }

    process->process_name[name_len] = '\0';

    (*p)++;
    *p = skip_spaces(*p);

    if (**p != ',') {
        return false;
    }

    (*p)++;
    *p = skip_spaces(*p);

    // Read the CPU percentage.
    if (strncmp(*p, "\"cpu\"", 5) != 0) {
        return false;
    }

    *p += 5;
    *p = skip_spaces(*p);

    if (**p != ':') {
        return false;
    }

    (*p)++;
    *p = skip_spaces(*p);

    if (!parse_integer(p, &process->process_cpu)) {
        return false;
    }

    if (**p != '}') {
        return false;
    }

    (*p)++;
    *p = skip_spaces(*p);

    return true;
}


// Parse up to five processes from the processes array.
static bool parse_processes(const char *json,
                            proocess_info_t output[MAX_PROCESSES],
                            int *count)
{
    const char *p = find_json_key(json, "processes");

    if (p == NULL || *p != '[') {
        return false;
    }

    p++;
    p = skip_spaces(p);

    *count = 0;

    if (*p == ']') {
        return true;
    }

    while (*count < MAX_PROCESSES) {
        if (!parse_process(&p, &output[*count])) {
            return false;
        }

        (*count)++;

        if (*p == ']') {
            return true;
        }

        if (*p != ',') {
            return false;
        }

        p++;
        p = skip_spaces(p);
    }

    // Only accept five processes, followed by the array terminator.
    return *p == ']';
}


static esp_err_t stats_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 ||
        req->content_len > MAX_REQUEST_SIZE) {

        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid request size"
        );

        return ESP_FAIL;
    }

    size_t body_length = (size_t)req->content_len;

    char *buffer = malloc(body_length + 1);

    if (buffer == NULL) {
        httpd_resp_send_err(
            req,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Out of memory"
        );

        return ESP_FAIL;
    }

    size_t total_received = 0;

    while (total_received < body_length) {
        int received = httpd_req_recv(
            req,
            buffer + total_received,
            body_length - total_received
        );

        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }

        if (received <= 0) {
            free(buffer);

            httpd_resp_send_err(
                req,
                HTTPD_400_BAD_REQUEST,
                "Failed to receive request body"
            );

            return ESP_FAIL;
        }

        total_received += (size_t)received;
    }

    buffer[total_received] = '\0';

    int cpu_value;
    int ram_value;
    int gpu_value;

    proocess_info_t new_processes[MAX_PROCESSES] = {0};
    int process_count = 0;

    bool valid =
        get_json_integer(buffer, "cpu", &cpu_value) &&
        get_json_integer(buffer, "ram", &ram_value) &&
        get_json_integer(buffer, "gpu", &gpu_value) &&
        parse_processes(buffer, new_processes, &process_count);

    free(buffer);

    if (!valid) {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid statistics or process list"
        );

        return ESP_FAIL;
    }

    // Update the statistics.
    info.cpu = cpu_value;
    info.ram = ram_value;
    info.gpu = gpu_value;

    // Clear old process entries so stale names disappear.
    memset(info.process, 0, sizeof(info.process));

    for (int i = 0; i < process_count; i++) {
        info.process[i] = new_processes[i];
    }

    ESP_LOGI(
        TAG,
        "CPU=%d%% RAM=%d%% GPU=%d%% Processes=%d",
        info.cpu,
        info.ram,
        info.gpu,
        process_count
    );

    for (int i = 0; i < process_count; i++) {
        ESP_LOGI(
            TAG,
            "%d. %s: %d%% CPU",
            i + 1,
            info.process[i].process_name,
            info.process[i].process_cpu
        );
    }

    httpd_resp_set_type(req, "application/json");

    return httpd_resp_send(
        req,
        "{\"status\":\"ok\"}",
        HTTPD_RESP_USE_STRLEN
    );
}


void webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        // httpd_uri_t uri1 = {
        //     .uri = "/",
        //     .method = HTTP_GET,
        //     .handler = root,
        // };
        // httpd_uri_t uri2 = {
        //     .uri = "/hello",
        //     .method = HTTP_GET,
        //     .handler = hello,
        // };
        httpd_uri_t uri3 = {
            .uri = "/api/stats",
            .method = HTTP_POST,
            .handler = stats_handler,
            .user_ctx = NULL
        };
        // httpd_register_uri_handler(server, &uri1);
        // httpd_register_uri_handler(server, &uri2);
        httpd_register_uri_handler(server, &uri3);
        commands_register(server);
        return;
    }
    printf("no webpage available\n");
    return;
}
