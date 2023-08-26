#ifndef HTTP_SERVER
#define HTTP_SERVER

#include <esp_http_server.h>

esp_err_t get_handler(httpd_req_t *req);

void server_initiation();

#endif