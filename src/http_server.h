#ifndef __http_server_h
#define __http_server_h

#include "uv_server.h"
#include "http_parser.h"


typedef struct http_system_set
{
    unsigned long run_time;
    int websocket_port;
    int websocket_send_serial;
    int web_port;
    int time_nodata_reboot;
    char account[24];
    char password[24];
    char device_name[24];
    char firmware_version[24];
    char type[12];
}http_system_set;

typedef struct http_network_set
{
    int IP_acquisition_method;
    char mac[18];
    char ip[16];
    char first_DNS[16];
    char standby_DNS[16];
    char mask[16];
    char gateway[16];
}http_network_set;

typedef struct http_token
{
    int timeout;
    char token[12];
}http_token;

typedef struct http_get_info
{
    char get_url[64];
    char get_field[32];
    char get_token[12];
}http_get_info;

typedef struct http_main
{
    struct http_network_set network_set;
    struct set_parameters * serial_set[PORT_SET_MAX];
    struct http_system_set system_set;
    struct http_token tokenlist[10];
    struct http_get_info get_msg;
}http_main;

int Http_Server_Init(uv_loop_t * loop);


#endif 

