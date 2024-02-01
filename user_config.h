#ifndef __user_config_h
#define __user_config_h

// #include <libconfig.h>
#include "uv_server.h"
#include "http_server.h"

enum socket_A_working_mode
{
    ATCP_SERVER = 0,
    ATCP_CLIENT,
    AUDP_SERVER,
    AUDP_CLIENT,
    AHTTPD_CLIENT
};

enum socket_B_working_mode
{
    BNONE = 0,
    BTCP_CLIENT,
    BUDP_CLIENT,
};

// int get_socket_seting(config_setting_t * set_class, set_parameters *target_fd);
// int get_device_seting(config_setting_t * set_class, struct http_main *device);
// int get_uart_seting(config_setting_t * set_class, set_parameters *port  );

int set_config_file(const char * cfg_file, struct http_main *device, struct set_parameters *port);
int set_json_file(const char * json_file, struct http_main *device, struct set_parameters *port);
int read_json_config(const char *json_file, struct http_main *device, struct set_parameters *port);

#endif
