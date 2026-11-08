#ifndef COMMANDS_H
#define COMMANDS_H

#include "esp_http_server.h"

/* Request the laptop to lock */
void commands_request_lock(void);

/* Register the command API endpoint */
void commands_register(httpd_handle_t server);

#endif /* COMMANDS_H */