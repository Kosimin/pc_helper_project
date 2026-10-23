#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include <esp_http_server.h>
#include "wifi.h"

#include "http.h"

esp_err_t root(httpd_req_t *req)
{
    const char mesg[] = "Hlo, I am Adihtya...";
    httpd_resp_send(req, mesg, sizeof(mesg) - 1);
    return ESP_OK;
}


esp_err_t hello(httpd_req_t *req)
{
    const char mesg[] = "hlo,world";
    httpd_resp_send(req, mesg, sizeof(mesg) - 1);
    return ESP_OK;
}


void webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t uri1 = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root,
        };
        httpd_uri_t uri2 = {
            .uri = "/hello",
            .method = HTTP_GET,
            .handler = hello,
        };
        httpd_register_uri_handler(server, &uri1);
        httpd_register_uri_handler(server, &uri2);
        return;
    }
    printf("no webpage available\n");
    return;
}
