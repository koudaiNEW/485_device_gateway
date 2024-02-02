#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <uv.h>
#include "user_config.h"
#include <linux/serial.h>
// #include <libconfig.h>
#include <pthread.h>
// #include "http_server.h"
// #include "uv_server.h"

extern struct http_main http_info;
set_parameters port_manage[PORT_SET_MAX];


/**********************************TCP****************************************/
#define DEFAULT_BACKLOG 128

/******************************************************************************/

/*********************************RS485***************************************/
/* Driver-specific ioctls: ...\linux-3.10.x\include\uapi\asm-generic\ioctls.h */
#define TIOCGRS485      0x542E
#define TIOCSRS485      0x542F

int alive_number;
/******************************************************************************/

/*********************************Other***************************************/

typedef struct write_req_t{
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;
static uv_loop_t *loop = NULL;
FILE * logs_fd = NULL;
/******************************************************************************/
/*================================================================*/
static void Socket_A_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void Socket_B_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void UDP_A_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void UDP_B_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void Serial_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_close(uv_handle_t* handle);
static void socket_write_cb(uv_write_t *serial_write_handle, int status);
static void udp_write_cb(uv_udp_send_t *socket_write_handle, int status);
static void serial_write_cb(uv_write_t *socket_write_handle, int status);
static void socket_A_socket_read(uv_stream_t *client_handle, ssize_t nread, const uv_buf_t *buf);
static void socket_B_socket_read(uv_stream_t *socket_handle, ssize_t nread, const uv_buf_t *buf);
static void UDP_A_read(uv_udp_t *socket_handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr* addr, unsigned flags);
static void UDP_B_read(uv_udp_t *socket_handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr* addr, unsigned flags);
static void on_tcp_client_connection(uv_connect_t *tcp_client_handle, int status);
static void on_tcp_server_connection(uv_stream_t *tcp_server_handle, int status);
static void serial_read(uv_stream_t *tty_handle, ssize_t nread, const uv_buf_t *buf);
/*================================================================*/
/*************************Memory management*******************************/
#define BUFFER_BYTE     2048
/***********************************Config*************************************/
static int System_config(const char *filename){
    int i, j, err_code = read_json_config(filename, &http_info, port_manage);
    if(err_code != 0){
        printf("power on seting  failed, error code: %d\n", err_code);
    }

    for(i = 0; i < alive_number; i++){
        port_manage[i].serial.uv_handle_serial = (uv_tty_t *)malloc(sizeof(uv_tty_t));
        port_manage[i].serial.buf.base = (char *)malloc(BUFFER_BYTE);
        port_manage[i].socket_A.uv_handle_tcp = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
        port_manage[i].socket_B.uv_handle_tcp = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
        port_manage[i].socket_A.uv_handle_tcp_connect = (uv_connect_t *)malloc(sizeof(uv_connect_t));
        port_manage[i].socket_B.uv_handle_tcp_connect = (uv_connect_t *)malloc(sizeof(uv_connect_t));
        port_manage[i].socket_A.uv_handle_udp = (uv_udp_t *)malloc(sizeof(uv_udp_t));
        port_manage[i].socket_B.uv_handle_udp = (uv_udp_t *)malloc(sizeof(uv_udp_t));
        port_manage[i].socket_A.udp_send_handle = (uv_udp_send_t *)malloc(sizeof(uv_udp_send_t));
        port_manage[i].socket_B.udp_send_handle = (uv_udp_send_t *)malloc(sizeof(uv_udp_send_t));
        if(port_manage[i].serial.uv_handle_serial == NULL || port_manage[i].socket_A.uv_handle_tcp == NULL || port_manage[i].socket_B.uv_handle_tcp == NULL || port_manage[i].socket_A.uv_handle_udp == NULL || port_manage[i].socket_B.uv_handle_udp == NULL){
            return -24;
        }

        for(j = 0; j < TCP_CONNECT_MAX; j++){
            port_manage[i].socket_A.client[j].uv_handle_tcp_client = NULL;
            port_manage[i].socket_B.client[j].uv_handle_tcp_client = NULL;
            port_manage[i].socket_B.client[j].buf.base = NULL;
            port_manage[i].socket_A.client[j].buf.base = (char *)malloc(BUFFER_BYTE);
            port_manage[i].socket_A.write[j].uv_handle_write = (uv_write_t *)malloc(sizeof(uv_write_t));
            port_manage[i].socket_B.write[j].uv_handle_write = (uv_write_t *)malloc(sizeof(uv_write_t));
            port_manage[i].serial.write[j].uv_handle_write = (uv_write_t *)malloc(sizeof(uv_write_t));
        }
        port_manage[i].serial.write[j].uv_handle_write = (uv_write_t *)malloc(sizeof(uv_write_t));
        uv_tty_init(loop, port_manage[i].serial.uv_handle_serial, port_manage[i].serial.fd, 1);
        uv_tty_set_mode(port_manage[i].serial.uv_handle_serial, UV_TTY_MODE_NORMAL);
        uv_read_start((uv_stream_t*) port_manage[i].serial.uv_handle_serial, Serial_Alloc_buffer, serial_read);

        switch (port_manage[i].socket_A.working_mode)
        {
        case ATCP_SERVER:
            TCP_server:
            uv_tcp_init(loop, port_manage[i].socket_A.uv_handle_tcp);
            uv_ip4_addr("0.0.0.0", port_manage[i].socket_A.listen_port, &port_manage[i].socket_A.listen_addr);
            uv_tcp_bind(port_manage[i].socket_A.uv_handle_tcp, (const struct sockaddr*)&port_manage[i].socket_A.listen_addr, 0);
            uv_listen((uv_stream_t*) port_manage[i].socket_A.uv_handle_tcp, DEFAULT_BACKLOG, on_tcp_server_connection);
            break;

        case ATCP_CLIENT:
            port_manage[i].socket_A.client[0].uv_handle_tcp_client = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
            uv_tcp_init(loop, port_manage[i].socket_A.client[0].uv_handle_tcp_client);
            uv_ip4_addr(port_manage[i].socket_A.target_ip, port_manage[i].socket_A.target_port, &port_manage[i].socket_A.listen_addr);
            uv_tcp_connect(port_manage[i].socket_A.uv_handle_tcp_connect, port_manage[i].socket_A.client[0].uv_handle_tcp_client, (struct  sockaddr *)&port_manage[i].socket_A.listen_addr, on_tcp_client_connection);
            break;

        case AUDP_SERVER:
            uv_udp_init(loop, port_manage[i].socket_A.uv_handle_udp);
            uv_ip4_addr("0.0.0.0", port_manage[i].socket_A.listen_port, &port_manage[i].socket_A.listen_addr);
            uv_udp_bind(port_manage[i].socket_A.uv_handle_udp, (const struct sockaddr*)&port_manage[i].socket_A.listen_addr, 0);
            uv_udp_recv_start(port_manage[i].socket_A.uv_handle_udp, UDP_A_Alloc_buffer, UDP_A_read);
            break;

        case AUDP_CLIENT:
            uv_udp_init(loop, port_manage[i].socket_A.uv_handle_udp);
            uv_ip4_addr("0.0.0.0", port_manage[i].socket_A.listen_port, &port_manage[i].socket_A.listen_addr);
            uv_udp_bind(port_manage[i].socket_A.uv_handle_udp, (const struct sockaddr*)&port_manage[i].socket_A.listen_addr, 0);
            uv_ip4_addr(port_manage[i].socket_A.target_ip, port_manage[i].socket_A.target_port, &port_manage[i].socket_A.udp_target_addr);
            uv_udp_recv_start(port_manage[i].socket_A.uv_handle_udp, UDP_A_Alloc_buffer, UDP_A_read);
            break;

        case AHTTPD_CLIENT:
            break;
        default:
            port_manage[i].socket_A.working_mode = ATCP_SERVER;
            goto TCP_server;
        }

        switch (port_manage[i].socket_B.working_mode)
        {
        case BNONE:
            break;

        case BTCP_CLIENT:
            port_manage[i].socket_B.client[0].buf.base = (char *)malloc(BUFFER_BYTE);
            port_manage[i].socket_B.client[0].uv_handle_tcp_client = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
            uv_tcp_init(loop, port_manage[i].socket_B.client[0].uv_handle_tcp_client);
            uv_ip4_addr(port_manage[i].socket_B.target_ip, port_manage[i].socket_B.target_port, &port_manage[i].socket_B.listen_addr);
            uv_tcp_connect(port_manage[i].socket_B.uv_handle_tcp_connect, port_manage[i].socket_B.client[0].uv_handle_tcp_client, (struct  sockaddr *)&port_manage[i].socket_B.listen_addr, on_tcp_client_connection);
            break;

        case BUDP_CLIENT:
            port_manage[i].socket_B.client[0].buf.base = (char *)malloc(BUFFER_BYTE);
            port_manage[i].socket_B.client[0].uv_handle_tcp_client = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
            uv_udp_init(loop, port_manage[i].socket_B.uv_handle_udp);
            uv_ip4_addr("0.0.0.0", port_manage[i].socket_B.listen_port, &port_manage[i].socket_B.listen_addr);
            uv_udp_bind(port_manage[i].socket_B.uv_handle_udp, (const struct sockaddr*)&port_manage[i].socket_B.listen_addr, 0);
            uv_ip4_addr(port_manage[i].socket_B.target_ip, port_manage[i].socket_B.target_port, &port_manage[i].socket_B.udp_target_addr);
            uv_udp_recv_start(port_manage[i].socket_B.uv_handle_udp, UDP_B_Alloc_buffer, UDP_B_read);
            break;

        default:
            port_manage[i].socket_B.working_mode = BNONE;
            break;
        }
    }
    
    return 0;
}
/******************************************************************************/

static void Socket_A_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    char i ,j;

    for(i = 0; i < alive_number; i++){
        for(j = 0; j < TCP_CONNECT_MAX; j++){
            if((uv_tcp_t *)handle == port_manage[i].socket_A.client[j].uv_handle_tcp_client){
                buf->base = port_manage[i].socket_A.client[j].buf.base;
                buf->len = suggested_size;
                return;
            }
        }
    }
    if(i >= alive_number){
        fprintf(logs_fd, "Socket_A on port_manage not found\n");
        fflush(logs_fd);
        buf->base = NULL;
        buf->len = 0;
    }
}

static void Socket_B_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    for(int i = 0; i < alive_number; i++){
        if((uv_tcp_t *)handle == port_manage[i].socket_B.client[0].uv_handle_tcp_client){
            buf->base = port_manage[i].socket_B.client[0].buf.base;
            buf->len = suggested_size;
            return;
        }
    }
    fprintf(logs_fd, "Socket_B on port_manage not found\n");
    fflush(logs_fd);
    buf->base = NULL;
    buf->len = 0;
}

static void UDP_A_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    char i ,j;

    for(i = 0; i < alive_number; i++){
        if(port_manage[i].socket_A.uv_handle_udp == (uv_udp_t *)handle){
            buf->base = port_manage[i].socket_A.client[0].buf.base;
            buf->len = suggested_size;
            return;
        }
    }
    fprintf(logs_fd, "UDP_A on port_manage not found\n");
    fflush(logs_fd);
    buf->base = NULL;
    buf->len = 0;
}

static void UDP_B_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    char i ,j;

    for(i = 0; i < alive_number; i++){
        if(port_manage[i].socket_B.uv_handle_udp == (uv_udp_t *)handle){
            buf->base = port_manage[i].socket_B.client[0].buf.base;
            buf->len = suggested_size;
            return;
        }
    }
    fprintf(logs_fd, "UDP_B on port_manage not found\n");
    fflush(logs_fd);
    buf->base = NULL;
    buf->len = 0;
}

static void Serial_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    char i;

    for(i = 0; i < alive_number; i++){
        if((uv_tty_t *)handle == port_manage[i].serial.uv_handle_serial){
            buf->base = port_manage[i].serial.buf.base;
            buf->len = suggested_size;
            return;
        }
    }
    if(i >= alive_number){
        fprintf(logs_fd, "serial on port_manage not found\n");
        fflush(logs_fd);
        buf->base = NULL;
        buf->len = 0;
    }
}

static void on_close(uv_handle_t* handle) {
    char i;

    for(i = 0; i < alive_number; i++){
        if((uv_handle_t *)port_manage[i].serial.uv_handle_serial == handle){
            return;
        }
        if((uv_handle_t *)port_manage[i].socket_A.uv_handle_tcp_connect == handle){
            return;
        }
        if((uv_handle_t *)port_manage[i].socket_B.uv_handle_tcp_connect == handle){
            return;
        }
    }
    free(handle);
}

static void socket_write_cb(uv_write_t *serial_write_handle, int status){
    char i, j;

    for(i = 0; i < alive_number; i++){
        for(j = 0; j < TCP_CONNECT_MAX + 1; j++){
            if(port_manage[i].serial.write[j].uv_handle_write == serial_write_handle){
                port_manage[i].serial.write[j].busy_flag = 0;
                return;
            }
        }
    }
    fprintf(logs_fd, "serial_write_handle on port_manage not found\n");
    fflush(logs_fd);
}

static void udp_write_cb(uv_udp_send_t *socket_write_handle, int status){

}

static void serial_write_cb(uv_write_t *socket_write_handle, int status){
    char i, j;

    for(i = 0; i < alive_number; i++){
        for(j = 0; j < TCP_CONNECT_MAX; j++){
            if(port_manage[i].socket_A.write[j].uv_handle_write == socket_write_handle){
                port_manage[i].socket_A.write[j].busy_flag = 0;
                return;
            }
            if(port_manage[i].socket_B.write[j].uv_handle_write == socket_write_handle){
                port_manage[i].socket_B.write[j].busy_flag = 0;
                return;
            }
        }
    }
    fprintf(logs_fd, "socket_write_handle on port_manage not found\n");
    fflush(logs_fd);
}

static void socket_A_socket_read(uv_stream_t *socket_handle, ssize_t nread, const uv_buf_t *buf) {
    char i, j, k;

    if (nread > 0) {
        for(i = 0; i < alive_number; i++){
            for(j = 0; j < TCP_CONNECT_MAX; j++){
                if((uv_stream_t *)port_manage[i].socket_A.client[j].uv_handle_tcp_client == socket_handle){
                    port_manage[i].socket_A.client[j].buf.len = nread;
                    for(k = 0; k < TCP_CONNECT_MAX; k++){
                        if(!port_manage[i].socket_A.write[k].busy_flag){
                            uv_write(port_manage[i].socket_A.write[k].uv_handle_write, (uv_stream_t *)port_manage[i].serial.uv_handle_serial, &port_manage[i].socket_A.client[j].buf, 1,  serial_write_cb);
                            port_manage[i].socket_A.write[k].busy_flag = 1;
                            return;
                        }
                    }
                }
            }
        }
    }

    if (nread < 0) {
        if (nread != UV_EOF){
            fprintf(logs_fd, "TCP Read error %s\n", uv_err_name(nread));
            fflush(logs_fd);
        }
    }

    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peername;
    int len = sizeof(peername);
    uv_tcp_getpeername((uv_tcp_t *)socket_handle, (struct sockaddr *)&peername, &len);
    fprintf(logs_fd, "TCP connect disconnected, peer address = %s : %d; uv handle closing\n", inet_ntop(AF_INET, &peername.sin_addr, ipAddr, sizeof(ipAddr)), ntohs(peername.sin_port));
    fflush(logs_fd);
    for(i = 0; i < alive_number; i++){
        for(j = 0; j < TCP_CONNECT_MAX; j++){
            if((uv_stream_t *)port_manage[i].socket_A.client[j].uv_handle_tcp_client == socket_handle){
                uv_close((uv_handle_t*) socket_handle, on_close);
                port_manage[i].socket_A.client[j].uv_handle_tcp_client = NULL;
                return;
            }
        }
    }
}

static void socket_B_socket_read(uv_stream_t *socket_handle, ssize_t nread, const uv_buf_t *buf) {
    char i, j, k;

    if (nread > 0) {
        for(i = 0; i < alive_number; i++){
            if((uv_stream_t *)port_manage[i].socket_B.client[0].uv_handle_tcp_client == socket_handle){
                port_manage[i].socket_B.client[0].buf.len = nread;
                for(k = 0; k < TCP_CONNECT_MAX; k++){
                    if(!port_manage[i].socket_B.write[k].busy_flag){
                        uv_write(port_manage[i].socket_B.write[k].uv_handle_write, (uv_stream_t *)port_manage[i].serial.uv_handle_serial, &port_manage[i].socket_B.client[0].buf, 1,  serial_write_cb);
                        port_manage[i].socket_B.write[k].busy_flag = 1;
                        return;
                    }
                }
            }
        }
    }

    if (nread < 0) {
        if (nread != UV_EOF){
            fprintf(logs_fd, "TCP Read error %s\n", uv_err_name(nread));
            fflush(logs_fd);
        }
    }

    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peername;
    int len = sizeof(peername);
    uv_tcp_getpeername((uv_tcp_t *)socket_handle, (struct sockaddr *)&peername, &len);
    fprintf(logs_fd, "TCP connect disconnected, peer address = %s : %d; uv handle closing\n", inet_ntop(AF_INET, &peername.sin_addr, ipAddr, sizeof(ipAddr)), ntohs(peername.sin_port));
    fflush(logs_fd);
    for(i = 0; i < alive_number; i++){
        for(j = 0; j < TCP_CONNECT_MAX; j++){
            if((uv_stream_t *)port_manage[i].socket_B.client[j].uv_handle_tcp_client == socket_handle){
                uv_close((uv_handle_t*) socket_handle, on_close);
                port_manage[i].socket_B.client[j].uv_handle_tcp_client = NULL;
                return;
            }
        }
    }
}

static void UDP_A_read(uv_udp_t *socket_handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr* addr, unsigned flags) {
    char i, j;

    if (nread > 0) {
        for(i = 0; i < alive_number; i++){
            if(port_manage[i].socket_A.uv_handle_udp == socket_handle){
                port_manage[i].socket_A.client[0].buf.len = nread;
                if(port_manage[i].socket_A.working_mode == AUDP_SERVER){
                    memcpy(&port_manage[i].socket_A.udp_target_addr, addr, 16);
                }
                // uv_udp_getpeername((uv_udp_t *)socket_handle, (struct sockaddr *)&port_manage[i].socket_A.udp_target_addr, &len);
                for(j = 0; j < TCP_CONNECT_MAX; j++){
                    if(!port_manage[i].socket_A.write[j].busy_flag){
                        uv_write(port_manage[i].socket_A.write[j].uv_handle_write, (uv_stream_t *)port_manage[i].serial.uv_handle_serial, &port_manage[i].socket_A.client[0].buf, 1,  serial_write_cb);
                        port_manage[i].socket_A.write[j].busy_flag = 1;
                        return;
                    }
                }
            }
        }
    }

    if (nread < 0) {
        if (nread != UV_EOF){
            fprintf(logs_fd, "UDP Read error %s\n", uv_err_name(nread));
            fflush(logs_fd);
        }
    }
    return ;
}

static void UDP_B_read(uv_udp_t *socket_handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr* addr, unsigned flags) {
    char i, j;

    if (nread > 0) {
        for(i = 0; i < alive_number; i++){
            if(port_manage[i].socket_B.uv_handle_udp == socket_handle){
                port_manage[i].socket_B.client[0].buf.len = nread;
                // uv_udp_getpeername((uv_udp_t *)socket_handle, (struct sockaddr *)&port_manage[i].socket_A.udp_target_addr, &len);
                for(j = 0; j < TCP_CONNECT_MAX; j++){
                    if(!port_manage[i].socket_B.write[j].busy_flag){
                        uv_write(port_manage[i].socket_B.write[j].uv_handle_write, (uv_stream_t *)port_manage[i].serial.uv_handle_serial, &port_manage[i].socket_B.client[0].buf, 1,  serial_write_cb);
                        port_manage[i].socket_B.write[j].busy_flag = 1;
                        return;
                    }
                }
            }
        }
    }

    if (nread < 0) {
        if (nread != UV_EOF){
            fprintf(logs_fd, "UDP Read error %s\n", uv_err_name(nread));
            fflush(logs_fd);
        }
    }
    return ;
}

static void on_tcp_client_connection(uv_connect_t *tcp_client_handle, int status){
    if (status < 0) {
        fprintf(logs_fd, "New server connection error %s\n", uv_strerror(status));
        fflush(logs_fd);
        for(int i = 0; i < alive_number; i ++){
            if((uv_connect_t *)port_manage[i].socket_A.uv_handle_tcp_connect == tcp_client_handle){
                uv_close((uv_handle_t*) port_manage[i].socket_A.client[0].uv_handle_tcp_client, on_close);
                port_manage[i].socket_A.client[0].uv_handle_tcp_client = NULL;
                return;
            }
            if((uv_connect_t *)port_manage[i].socket_B.uv_handle_tcp_connect == tcp_client_handle){
                uv_close((uv_handle_t*) port_manage[i].socket_B.client[0].uv_handle_tcp_client, on_close);
                port_manage[i].socket_B.client[0].uv_handle_tcp_client = NULL;
                return;
            }
        }
        return;
    }
    for(int i = 0; i < alive_number; i ++){
        if((uv_connect_t *)port_manage[i].socket_A.uv_handle_tcp_connect == tcp_client_handle){
            uv_read_start((uv_stream_t*) port_manage[i].socket_A.client[0].uv_handle_tcp_client, Socket_A_Alloc_buffer, socket_A_socket_read);
            return;
        }
        if((uv_connect_t *)port_manage[i].socket_B.uv_handle_tcp_connect == tcp_client_handle){
            uv_read_start((uv_stream_t*) port_manage[i].socket_B.client[0].uv_handle_tcp_client, Socket_B_Alloc_buffer, socket_B_socket_read);
            return;
        }
    }
    fprintf(logs_fd, "client connect not found, closing new connection\n");
    fflush(logs_fd);
}

static void on_tcp_server_connection(uv_stream_t *tcp_server_handle, int status) {
    char i, j, ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peername;
    int len = sizeof(peername);

    if (status < 0) {
        fprintf(logs_fd, "New client connection error %s\n", uv_strerror(status));
        fflush(logs_fd);
        return;
    }
    uv_tcp_t *getin = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, getin);
    if (uv_accept(tcp_server_handle, (uv_stream_t*) getin) != 0) {
        fprintf(logs_fd, "TCP connection accept error\n");
        goto out;
    }
    uv_tcp_getpeername(getin, (struct sockaddr *)&peername, &len);
    for(i = 0; i < alive_number; i++){
        if((uv_stream_t *)port_manage[i].socket_A.uv_handle_tcp == tcp_server_handle){
            fprintf(logs_fd, "TCP connect access on port server %d, peer address = %s : %d\n", i, inet_ntop(AF_INET, &peername.sin_addr, ipAddr, sizeof(ipAddr)), ntohs(peername.sin_port));
            fflush(logs_fd);
            if(port_manage[i].socket_A.connection_alive >= port_manage[i].socket_A.connection_max){
                fprintf(logs_fd, "port_server %d connection reached maximum limit\n", i);
                goto out;
            }else{
                for(j = 0; j < port_manage[i].socket_A.connection_max; j++){
                    if(port_manage[i].socket_A.client[j].uv_handle_tcp_client == NULL){
                        port_manage[i].socket_A.client[j].uv_handle_tcp_client = getin;
                        port_manage[i].socket_A.connection_alive++;
                        break;
                    }
                }
            }
            break;
        }
    }
    if(i >= alive_number){
        fprintf(logs_fd, "port server not found, closing new connection\n");
        out:
        fflush(logs_fd);
        uv_close((uv_handle_t*) getin, on_close);
        return;
    }
    uv_read_start((uv_stream_t*) getin, Socket_A_Alloc_buffer, socket_A_socket_read);
}

static void serial_read(uv_stream_t *tty_handle, ssize_t nread, const uv_buf_t *buf) {
    char i, j, k = 0;

     if (nread > 0){
        for(i = 0; i < alive_number; i++){
            if((uv_stream_t *)port_manage[i].serial.uv_handle_serial == tty_handle){
                port_manage[i].serial.buf.len = nread;
                switch (port_manage[i].socket_A.working_mode)
                {
                case ATCP_SERVER:
                    for(j = 0; j < TCP_CONNECT_MAX + 1; j++){
                        if(!port_manage[i].serial.write[j].busy_flag){
                            for(;k < TCP_CONNECT_MAX; k++){
                                if(port_manage[i].socket_A.client[k].uv_handle_tcp_client != NULL){
                                    uv_write(port_manage[i].serial.write[j].uv_handle_write, (uv_stream_t *)port_manage[i].socket_A.client[k].uv_handle_tcp_client, &port_manage[i].serial.buf, 1,  socket_write_cb);
                                    port_manage[i].serial.write[j].busy_flag = 1;
                                    k++;
                                    break;
                                }
                            }
                        }
                        if(k >= TCP_CONNECT_MAX){
                            break;
                        }
                    }
                    break;

                case ATCP_CLIENT:
                    if(port_manage[i].socket_A.client[0].uv_handle_tcp_client == NULL){
                        break;
                    }
                    for(j = 0; j < TCP_CONNECT_MAX + 1; j++){
                        if(!port_manage[i].serial.write[j].busy_flag){
                            uv_write(port_manage[i].serial.write[j].uv_handle_write, (uv_stream_t *)port_manage[i].socket_A.client[0].uv_handle_tcp_client, &port_manage[i].serial.buf, 1,  socket_write_cb);
                            port_manage[i].serial.write[j].busy_flag = 1;
                            break;
                        }
                    }
                    break;

                case AUDP_SERVER:
                case AUDP_CLIENT:
                    uv_udp_send(port_manage[i].socket_A.udp_send_handle, port_manage[i].socket_A.uv_handle_udp, &port_manage[i].serial.buf, 1, (struct sockaddr *)&port_manage[i].socket_A.udp_target_addr, udp_write_cb);
                    break;

                case AHTTPD_CLIENT:
                    break;
                default:
                    break;
                }

                switch (port_manage[i].socket_B.working_mode)
                {
                case BNONE:
                    break;

                case BTCP_CLIENT:
                    if(port_manage[i].socket_B.client[0].uv_handle_tcp_client == NULL){
                        break;
                    }
                    for(j = 0; j < TCP_CONNECT_MAX; j++){
                        if(!port_manage[i].serial.write[j].busy_flag){
                            uv_write(port_manage[i].serial.write[j].uv_handle_write, (uv_stream_t *)port_manage[i].socket_B.client[0].uv_handle_tcp_client, &port_manage[i].serial.buf, 1,  socket_write_cb);
                            port_manage[i].serial.write[j].busy_flag = 1;
                            break;
                        }
                    }
                    break;

                case BUDP_CLIENT:
                    uv_udp_send(port_manage[i].socket_B.udp_send_handle, port_manage[i].socket_B.uv_handle_udp, &port_manage[i].serial.buf, 1, (struct sockaddr *)&port_manage[i].socket_B.udp_target_addr, udp_write_cb);
                    break;

                default:
                    break;
                }
                return;
            }
        }
     }

    if (nread < 0) {
        if (nread != UV_EOF){
            fprintf(logs_fd, "Read error %s\n", uv_err_name(nread));
            fflush(logs_fd);
        }
    }
}

int main() {
    int i;

    if((logs_fd = fopen("tcp_uart_sys.log", "a")) ==   NULL)
    {
        printf("Create or open logs file fail.\n");
        exit(1);
    }
    fprintf(logs_fd, "\n\nLoging - >>\n");
    fflush(logs_fd);

    loop = uv_default_loop();

    if((i = System_config("client.json")) < 0){
        fprintf(logs_fd, "System initial setup error, error code: %d\n", i);
        fflush(logs_fd);
        exit(1);
    }

    if((i = Http_Server_Init(loop)) < 0){
        fprintf(logs_fd, "Http setup error, error code: %d\n", i);
        fflush(logs_fd);
        exit(1);
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}