#ifndef __uv_server_h
#define __uv_server_h

#define TCP_CONNECT_MAX   16
#define PORT_SET_MAX   8

#include <uv.h>

#define debug 1

typedef struct write_parameters{
    uv_write_t * uv_handle_write;
    char busy_flag;
}write_parameters;

typedef struct client_parameters{
    uv_buf_t buf;
    uv_tcp_t * uv_handle_tcp_client;
}client_parameters;

typedef struct socket_parameters
{
    struct client_parameters client[TCP_CONNECT_MAX];
    struct sockaddr_in listen_addr;
    struct sockaddr_in udp_target_addr;
    struct write_parameters write[TCP_CONNECT_MAX];
    uv_tcp_t * uv_handle_tcp;
    uv_connect_t * uv_handle_tcp_connect;
    uv_udp_t * uv_handle_udp;
    uv_udp_send_t * udp_send_handle;
    int working_mode;
    int listen_port;
    int target_port;
    int connection_max;
    int connection_alive;
    // const char *target_ip;
    char target_ip[16];
}socket_parameters;

typedef struct serial_parameters
{
    struct write_parameters write[TCP_CONNECT_MAX + 1];
    uv_buf_t buf;
    uv_tty_t * uv_handle_serial;
    int BAUD;
    int data_bits;
    int stop_bit;
    int packaging_length;
    int packaging_time;
    int fd;
    char driver_file[24];
    char check_bit[5];
    char stream_control[5];
}serial_parameters;

typedef struct set_parameters
{
    struct serial_parameters serial;
    struct socket_parameters socket_A;
    struct socket_parameters socket_B;
}set_parameters;



#endif
