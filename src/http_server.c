#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
// #include <uv.h>
#include <unistd.h>
#include "user_config.h"
#include "http_server.h"
#include "cJSON.h"

#define HTTP_CONNECT_MAX   40

http_main http_info;

extern set_parameters port_manage[];
extern int alive_number;

static http_parser_settings parser_settings;

typedef struct write_req_t{
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

typedef struct write_handle{
    write_req_t *message;
    uv_tcp_t *positioning;
    http_parser *in_parser;
    char *read_buf;
    char *http_url;
    char connection_timing;
}write_handle;

typedef struct http_msg_structure{
    struct stat fd_state;
    struct http_parser getin_parser;
    const char *ok_hand;
    const char *Access;
    const char *fdtype_hand;
    const char *fdlen_hand;
    const char *OPTIONS_handle;
    char *body_data;
    char nmb[24];
    char fd_type[24];
    char json_handle[24];
    char get_method[8];
}http_msg_structure;

static struct http_msg_structure http_send_msg = {
    .ok_hand = "HTTP/1.1 200 OK\r\n",
    .Access = "Access-Control-Allow-Origin:*\r\n",
    .fdtype_hand = "Content-Type: ",
    .fdlen_hand = "Content-Length: ",
    .OPTIONS_handle = "HTTP/1.1 200 OK\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: OPTIONS,DELETE,GET,PUT,POST\r\n"
    "Access-Control-Allow-Headers: x-requested-with, accept, origin, content-type, token\r\n"
    "Access-Control-Max-Age: 3600\r\n"
    "Content-Type: application/json;charset=utf-8\r\n"
    "Content-Length: 0\r\n"
    "Allow: HEAD, GET, POST, PUT\r\n\r\n",
    .body_data = NULL
};

static write_handle http_connect[HTTP_CONNECT_MAX];
/*******************************************Debug*******************************************/
// static uv_buf_t send_url;
/*********************************************************************************************/

/*********************************************************************************************/
static int on_message_begin(http_parser* parser);
static int on_url(http_parser* parser, const char* at, size_t length);
static int on_status(http_parser* parser, const char* at, size_t length);
static int on_header_field(http_parser* parser, const char* at, size_t length);
static int on_header_value(http_parser* parser, const char* at, size_t length);
static int on_headers_complete(http_parser* parser);
static int on_body(http_parser* parser, const char* at, size_t length);
static int on_message_complete(http_parser* parser);
static int on_chunk_header(http_parser* parser);
static int on_chunk_complete(http_parser* parser);
static void on_http_connect(uv_stream_t* server, int status);
static void Http_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void HTTP_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
static void on_close(uv_handle_t* handle);
static void HTTP_cb_write(uv_write_t *req, int status);
static int Create_Json(struct http_msg_structure * msg, uv_buf_t * send_buf);
static char * assemble_json(const char * json_handle);
static void http_connet_timeout_cb(uv_timer_t *uv_timer_handle);
static int GET_file(struct http_msg_structure * msg, uv_buf_t * send_buf);
static char * analysis_json(const char * json_handle, const char * get_json);
static char * http_create_token(struct http_token *tokenlist);
static int http_delete_token(struct http_token *tokenlist, char *token);
static int http_check_token(struct http_token *tokenlist, char *get_token);
static unsigned long http_get_runtime(void);
/*********************************************************************************************/
/*************************Memory management*******************************/
#define BUFFER_BYTE     10240

static int Memory_Arrange(void)
{
    int i;
    const char *file_root = "/www";
    // send_url.base = (char*)malloc(256);
    memcpy(http_info.get_msg.get_url, file_root, 5);

    for(i = 0; i < HTTP_CONNECT_MAX; i++){
        http_connect[i].message = (write_req_t *)malloc(sizeof(write_req_t));
        // http_connect[i].in_parser = (http_parser *)malloc(sizeof(http_parser));
        http_connect[i].read_buf = (char *)malloc(BUFFER_BYTE);
        // printf("Memory_Arrange: [%d]read_buf: %p\n", i,  http_connect[i].read_buf);
        // http_connect[i].http_url = (char *)malloc(128);
        http_connect[i].message->buf.base = NULL;
        http_connect[i].positioning = NULL;
        // http_connect[i].busy = http_connect[i].ready = false;
        if(http_connect[i].message == NULL){
            return -1;
        }
        // if(http_connect[i].in_parser == NULL){
        //     return -2;
        // }
        if(http_connect[i].read_buf == NULL){
            return -3;
        }
    }
    return 0;
}
/******************************************************************************/

int Http_Server_Init(uv_loop_t * loop){
    int i;

    for(i = 0; i < 10; i++){
        memset(http_info.tokenlist[i].token, 32, 12);
    }
    for(i = 0; i < alive_number; i++){
        http_info.serial_set[i] = &port_manage[i];
    }
    parser_settings.on_message_begin = on_message_begin;
    parser_settings.on_url = on_url;
    parser_settings.on_status = on_status;
    parser_settings.on_header_field = on_header_field;
    parser_settings.on_header_value = on_header_value;
    parser_settings.on_headers_complete = on_headers_complete;
    parser_settings.on_body = on_body;
    parser_settings.on_message_complete = on_message_complete;
    parser_settings.on_chunk_header = on_chunk_header;
    parser_settings.on_chunk_complete = on_chunk_complete;

    if((i = Memory_Arrange()) < 0){
        return i;
    }

    uv_tcp_t * uv_handle_http_server = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, uv_handle_http_server);
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    uv_ip4_addr("0.0.0.0", 80, addr);
    uv_tcp_bind(uv_handle_http_server, (const struct sockaddr*)addr, 0);
    uv_listen((uv_stream_t*)uv_handle_http_server, 128, on_http_connect);

    uv_timer_t * uv_handle_http_timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, uv_handle_http_timer);
    uv_timer_start(uv_handle_http_timer, http_connet_timeout_cb, 180000, 180000);
    return 0;
}

static int on_message_begin(http_parser* parser) {
    #if debug
    printf("***MESSAGE BEGIN***\n");
    #endif
    return 0;
}

static int on_url(http_parser* parser, const char* at, size_t length) {
    int i, j;
    #if debug
    printf("In url: ");
    #endif
    if(length < 64){
        memcpy(http_info.get_msg.get_url + 4, at, length);
        http_info.get_msg.get_url[length + 4] = 0;
        // *(send_url.base + 4 + length) = 0;
        j = strlen(http_info.get_msg.get_url);
        for(i = 0; i < j; i++){
            // if(http_info.get_msg.get_url[18 + i] == '?' || http_info.get_msg.get_url[18 + i] == 0){
            //     http_info.get_msg.get_url[18 + i] = 0;
            //     break;
            // }
            if(http_info.get_msg.get_url[i] == '?' || http_info.get_msg.get_url[i] == 0){
                http_info.get_msg.get_url[i] = 0;
                break;
            }
        }
    }
    #if debug
    printf("%s\n", http_info.get_msg.get_url);
    #endif
    return 0;
}

static int on_status(http_parser* parser, const char* at, size_t length) {
    #if debug
    printf("on_status\n");
    #endif
    return 0;
}

static int on_header_field(http_parser* parser, const char* at, size_t length) {
    if(length < 32){
        memcpy(http_info.get_msg.get_field, at, length);
        http_info.get_msg.get_field[length] = 0;
        #if debug
        // printf("title: %s\n", http_info.get_msg.get_field);
        #endif
        if(!strcmp(http_info.get_msg.get_field, "Token") || !strcmp(http_info.get_msg.get_field, "TOKEN")){
            memset(http_info.get_msg.get_token, 0, 12);
        }
    }
    return 0;
}

static int on_header_value(http_parser* parser, const char* at, size_t length) {
    if(length < 12){
        if(!strcmp(http_info.get_msg.get_field, "Token") || !strcmp(http_info.get_msg.get_field, "TOKEN")){
            memcpy(http_info.get_msg.get_token, at, length);
            http_info.get_msg.get_token[length] = 0;
        }
    }
    return 0;
}

static int on_headers_complete(http_parser* parser) {
    return 0;
}

static int on_body(http_parser* parser, const char* at, size_t length) {
    #if debug
    // printf("on_body\n");
    // char *get_body = (char *)malloc(256);
    // if(length < 256){
    //     memcpy(get_body, at, length);
    // }
    // printf("get_body: %s\n", get_body);
    // free(get_body);
    #endif
    http_send_msg.body_data = (char *)malloc(length + 1);
    memcpy(http_send_msg.body_data, at, length);
    http_send_msg.body_data[length] = 0;
    return 0;
}

static int on_message_complete(http_parser* parser) {
    #if debug
    printf("***MESSAGE COMPLETE***\n");
    #endif
    return 0;
}

static int on_chunk_header(http_parser* parser) {
    #if debug
    printf("\n***chunk_header***\n\n");
    #endif
    return 0;
}

static int on_chunk_complete(http_parser* parser) {
    #if debug
    printf("\n***chunk_complete***\n\n");
    #endif
    return 0;
}

static void on_http_connect(uv_stream_t* server, int status) {
    char i;

    if (status < 0) {
        // error!
        return;
    }

    uv_tcp_t *getin = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, getin);
    if (uv_accept(server, (uv_stream_t*) getin) != 0) {
        goto out;
    }
    // uv_tcp_getpeername(getin, (struct sockaddr *)&peername, &len);
    // fprintf(logs_fd, "TCP connect access, peer address = %s : %d\n", inet_ntop(AF_INET, &peername.sin_addr, ipAddr, sizeof(ipAddr)), ntohs(peername.sin_port));
    // fflush(logs_fd);
    for(i = 0; i < HTTP_CONNECT_MAX; i++){
        if(http_connect[i].positioning == NULL){
            http_connect[i].positioning = getin;
            // http_parser_init(http_connect[i].in_parser, HTTP_REQUEST);
            break;
        }
    }
    if(i >= HTTP_CONNECT_MAX){
        out:
        uv_close((uv_handle_t*) getin, on_close);
        return;
    }
    // uv_read_start((uv_stream_t*) http_connect[i].positioning, Http_Alloc_buffer, HTTP_read);
    uv_read_start((uv_stream_t*) http_connect[i].positioning, Http_Alloc_buffer, HTTP_read);
}

static void on_close(uv_handle_t* handle) {
    int i;
    #if debug
    printf("In close\n");
    #endif
    for( i = 0; i < HTTP_CONNECT_MAX; i++){
        if((uv_handle_t*)http_connect[i].positioning == handle ){
            http_connect[i].positioning = NULL;
            http_connect[i].connection_timing = 0;
            #if debug
            printf("http_connect[%d] closing\n", i);
            #endif
            break;
        }
    }
    free(handle);
}

static void Http_Alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    char cnt_index ;

    for(cnt_index = 0; cnt_index < HTTP_CONNECT_MAX; cnt_index++){
        if((uv_tcp_t *)handle == http_connect[cnt_index].positioning){
            buf->base = http_connect[cnt_index].read_buf;
            buf->len = suggested_size;
            #if debug
            printf("\nHttp_Alloc_buffer: http_connect[%d]\n", cnt_index);
            #endif
            return;
        }
    }
    if(cnt_index >= HTTP_CONNECT_MAX){
        buf->base = NULL;
        buf->len = 0;
        #if debug
        printf("Http_Alloc_buffer: not found\n");
        #endif
    }
}

static void HTTP_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    int cnt_index, j;

    #if debug
    printf("on HTTP_read, http msg:\n%s\n", buf->base);
    #endif
    if(nread == 0){
        uv_close((uv_handle_t *)client, on_close);
        return;
    }else if(nread < 0){
        #if debug
        printf("http msg error.\n");
        #endif
        uv_close((uv_handle_t *)client, on_close);
        return;
    }
    for(cnt_index = 0; cnt_index < HTTP_CONNECT_MAX; cnt_index++){
        if((uv_stream_t *)http_connect[cnt_index].positioning == client ){
            http_connect[cnt_index].connection_timing = 2;
            break;
        }
    }
    #if debug
    printf("http_connect[%d].positioning\n", cnt_index);
    #endif
    if(cnt_index >= HTTP_CONNECT_MAX){
        #if debug
        printf("connect not found\n");
        #endif
        return;
    }

    http_parser_init(&http_send_msg.getin_parser, HTTP_REQUEST);
    http_parser_execute(&http_send_msg.getin_parser, &parser_settings, buf->base, nread);
    for(j = 0; j < 8; j++){
        if(buf->base[j] == ' '){
            memcpy(http_send_msg.get_method, buf->base, j);
            http_send_msg.get_method[j] = 0;
            #if debug
            printf("get_method: %s\n", http_send_msg.get_method);
            #endif
            break;
        }
    }

    if(strcmp(http_info.get_msg.get_url, "/www/") == 0){
        strcat(http_info.get_msg.get_url, "index.html");
    }else if(strcmp(http_info.get_msg.get_url, "/www/login") == 0){
        strcat(http_info.get_msg.get_url, ".json");
    }
    for(j = 0; j < strlen(http_info.get_msg.get_url); j++){
        if(http_info.get_msg.get_url[j] == '.'){
            http_send_msg.fd_type[0] = 0;
            http_send_msg.json_handle[0] = 0;
            strcat(http_send_msg.fd_type, &http_info.get_msg.get_url[j + 1]);
            if(strcmp(http_send_msg.fd_type, "json") == 0){
                memcpy(http_send_msg.fd_type, "application/json", 17);
                strcat(http_send_msg.json_handle, http_info.get_msg.get_url + 5);
                #if debug
                printf("on HTTP_read json_handle: %s\n", http_send_msg.json_handle);
                printf("fd_type: %s\n", http_send_msg.fd_type);
                #endif
            }else if(strcmp(http_send_msg.fd_type, "html") == 0){
                memcpy(http_send_msg.fd_type, "text/html", 10);
            }else if(strcmp(http_send_msg.fd_type, "css") == 0){
                memcpy(http_send_msg.fd_type, "text/css", 9);
            }else if(strcmp(http_send_msg.fd_type, "js") == 0){
                memcpy(http_send_msg.fd_type, "text/javascript", 16);
            }else if(strcmp(http_send_msg.fd_type, "png") == 0){
                memcpy(http_send_msg.fd_type, "image/png", 10);
            }else if(strcmp(http_send_msg.fd_type, "ico") == 0){
                memcpy(http_send_msg.fd_type, "image/x-icon", 13);
            }else if(strcmp(http_send_msg.fd_type, "gif") == 0){
                memcpy(http_send_msg.fd_type, "image/gif", 10);
            }
            strcat(http_send_msg.fd_type, "\r\n");
            break;
        } else if(http_info.get_msg.get_url[j] == 0){
            break;
        }
    }

    if(strcmp(http_send_msg.fd_type, "application/json\r\n") == 0){
        if(Create_Json(&http_send_msg, &http_connect[cnt_index].message->buf) == 0){
            goto GET_send;
        }else return;
    }else if(GET_file(&http_send_msg, &http_connect[cnt_index].message->buf) == 0){
        GET_send:
        uv_write(&http_connect[cnt_index].message->req, (uv_stream_t *)http_connect[cnt_index].positioning, &http_connect[cnt_index].message->buf, 1,  HTTP_cb_write);
    }
}

static int Create_Json(struct http_msg_structure *msg, uv_buf_t * send_buf){
    char * json_str = NULL;
    if(strcmp(msg->get_method, "GET") == 0){
        json_str = assemble_json(msg->json_handle);
        goto transition;
    }
    if(strcmp(msg->get_method, "POST") == 0){
        json_str = analysis_json(msg->json_handle, msg->body_data);
        if(msg->body_data != NULL){
            free(msg->body_data);
            msg->body_data = NULL;
        }
        set_json_file("client.json", &http_info, port_manage);
        goto transition;
    }
    if(strcmp(msg->get_method, "OPTIONS") == 0){
        send_buf->len = strlen(msg->OPTIONS_handle) + 1;
        send_buf->base =  (char*)malloc(send_buf->len);
        memcpy(send_buf->base, msg->OPTIONS_handle, send_buf->len);
        return 0;
    }
    return -1;

    transition:
    #if debug
    printf("json_str:\n%s\n", json_str);
    #endif
    snprintf(msg->nmb, 23, "%d\r\n\r\n", strlen(json_str));
    send_buf->len = 17+31+14+strlen(msg->fd_type)+16+strlen(msg->nmb)+strlen(json_str);
    send_buf->base =  (char*)malloc(send_buf->len);
    memcpy(send_buf->base, msg->ok_hand, 18);
    strcat(send_buf->base, msg->Access);
    strcat(send_buf->base, msg->fdtype_hand);
    strcat(send_buf->base, msg->fd_type);
    strcat(send_buf->base, msg->fdlen_hand);
    strcat(send_buf->base, msg->nmb);
    strcat(send_buf->base, json_str);
    free(json_str);
    return 0;
}

static int GET_file(struct http_msg_structure *msg, uv_buf_t * send_buf){
    int send_fd;
    if((send_fd = open(http_info.get_msg.get_url, O_RDONLY)) < 0){
        #if debug
        printf("open file fail\n");
        #endif
        return -1;
    }
    fstat(send_fd, &msg->fd_state);
    sprintf(msg->nmb, "%ld\r\n\r\n", msg->fd_state.st_size);
    send_buf->len = 17 + 31 + 14 + strlen(msg->fd_type) + 16 + strlen(msg->nmb) + msg->fd_state.st_size;
    send_buf->base = (char*)malloc(send_buf->len);

    memcpy(send_buf->base, msg->ok_hand, 18); 
    strcat(send_buf->base, msg->Access); 
    strcat(send_buf->base, msg->fdtype_hand); 
    strcat(send_buf->base, msg->fd_type);  
    strcat(send_buf->base, msg->fdlen_hand); 
    strcat(send_buf->base, msg->nmb); 
    
    if(read(send_fd, send_buf->base + strlen(send_buf->base),  (size_t)msg->fd_state.st_size) < 0){
        #if debug
        printf("read file fail\n");
        #endif
        return -2;
    } 
    close(send_fd);
    return 0;
}

static void HTTP_cb_write(uv_write_t *req, int status){
    #if debug
    printf("In http write cb\n");
    #endif

    for(int i = 0; i < HTTP_CONNECT_MAX; i++){
        if((&http_connect[i].message->req) == req){
            #if debug
            printf("free file memory before, http_connect[%d]\n", i);
            #endif
            if(http_connect[i].message->buf.base != NULL){
                free(http_connect[i].message->buf.base);
                http_connect[i].message->buf.base = NULL;
                #if debug
                printf("free memory after\n");
                #endif
            }
            return;
        }
    }
}

static char * assemble_json(const char * json_handle){
    #if debug
    printf("on assemble_json json_handle: %s\n", json_handle);
    #endif
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    int err_code = -1;

    if(http_check_token(http_info.tokenlist, http_info.get_msg.get_token) < 0){
        err_code = -4;
        goto out;
    }
    if(strcmp(json_handle, "device_status.json") == 0){
        cJSON_AddStringToObject(data, "device_name", http_info.system_set.device_name);
        cJSON_AddStringToObject(data, "firmware_version", http_info.system_set.firmware_version);
        cJSON_AddStringToObject(data, "type", http_info.system_set.type);
        http_info.system_set.run_time = http_get_runtime();
        cJSON_AddNumberToObject(data, "run_time", (double)http_info.system_set.run_time);
        cJSON_AddStringToObject(data, "mac", http_info.network_set.mac);
        cJSON_AddStringToObject(data, "account", http_info.system_set.account);
        cJSON_AddStringToObject(data, "password", http_info.system_set.password);
        err_code = 0;
    }else if(strcmp(json_handle, "network_get.json") == 0){
        cJSON_AddNumberToObject(data, "IP_acquisition_method", (double)http_info.network_set.IP_acquisition_method);
        cJSON_AddStringToObject(data, "ip", http_info.network_set.ip);
        cJSON_AddStringToObject(data, "mask", http_info.network_set.mask);
        cJSON_AddStringToObject(data, "gateway", http_info.network_set.gateway);
        cJSON_AddStringToObject(data, "first_DNS", http_info.network_set.first_DNS);
        cJSON_AddStringToObject(data, "standby_DNS", http_info.network_set.standby_DNS);
        err_code = 0;
    }else{
        char get_port[12];
        int index = 10;
        memcpy(get_port, "port_0.json", 12);
        for(int i = 0; i < 4; i++){
            get_port[5]++;
            if(strcmp(json_handle, get_port) == 0){
                index = i;
                break;
            }
        }
        if(index == 10){
            err_code = -1;
            goto out;
        }
        cJSON_AddNumberToObject(data, "baud", (double)http_info.serial_set[index]->serial.BAUD);
        cJSON_AddNumberToObject(data, "data_bits", (double)http_info.serial_set[index]->serial.data_bits);
        cJSON_AddStringToObject(data, "check_bit", http_info.serial_set[index]->serial.check_bit);
        cJSON_AddNumberToObject(data, "stop_bit", (double)http_info.serial_set[index]->serial.stop_bit);
        cJSON_AddNumberToObject(data, "socket_A_working_mode", (double)http_info.serial_set[index]->socket_A.working_mode);
        cJSON_AddNumberToObject(data, "socket_A_connection_max", (double)http_info.serial_set[index]->socket_A.connection_max);
        cJSON_AddNumberToObject(data, "socket_A_connection_alive", (double)http_info.serial_set[index]->socket_A.connection_alive);
        cJSON_AddNumberToObject(data, "socket_A_listen_port", (double)http_info.serial_set[index]->socket_A.listen_port);
        cJSON_AddStringToObject(data, "socket_A_target_ip", http_info.serial_set[index]->socket_A.target_ip);
        cJSON_AddNumberToObject(data, "socket_A_target_port", (double)http_info.serial_set[index]->socket_A.target_port);
        cJSON_AddNumberToObject(data, "socket_B_working_mode", (double)http_info.serial_set[index]->socket_B.working_mode);
        cJSON_AddNumberToObject(data, "socket_B_listen_port", (double)http_info.serial_set[index]->socket_B.listen_port);
        cJSON_AddStringToObject(data, "socket_B_target_ip", http_info.serial_set[index]->socket_B.target_ip);
        cJSON_AddNumberToObject(data, "socket_B_target_port", (double)http_info.serial_set[index]->socket_B.target_port);
        err_code = 0;
    }
    out:
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddNumberToObject(root, "err_code", (double)err_code);
    if(err_code == 0){
        cJSON_AddStringToObject(root, "status", "successful");
    }else{
        #if debug
        printf("api not found.\n");
        #endif
        cJSON_AddStringToObject(root, "status", "failed");
    }
    char * json_str = cJSON_Print(root);
    cJSON_Delete(root);
    return json_str;
}

static char *analysis_json(const char * json_handle, const char * get_json){
    cJSON *get_root = cJSON_Parse(get_json);
    cJSON *get_key = NULL;
    cJSON *data = NULL;
    cJSON *send_root = cJSON_CreateObject();
    int err_code = 0;

    if(strcmp(json_handle, "device_status.json") == 0){
        if(http_check_token(http_info.tokenlist, http_info.get_msg.get_token) < 0){
            err_code = -4;
            goto out;
        }
        get_key = cJSON_GetObjectItem(get_root, "device_name");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 24){
                memcpy(http_info.system_set.device_name, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.system_set.device_name, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "account");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 24){
                memcpy(http_info.system_set.account, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.system_set.account, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "password");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 24){
                memcpy(http_info.system_set.password, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.system_set.password, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
    }else if(strcmp(json_handle, "network_get.json") == 0){
        if(http_check_token(http_info.tokenlist, http_info.get_msg.get_token) < 0){
            err_code = -4;
            goto out;
        }
        get_key = cJSON_GetObjectItem(get_root, "IP_acquisition_method");
        if(get_key != NULL){
            if(!(get_key->valueint > 1 || get_key->valueint < 0)){
                http_info.network_set.IP_acquisition_method = get_key->valueint;
            }
            if(http_info.network_set.IP_acquisition_method != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "ip");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(http_info.network_set.ip, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.network_set.ip, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "mask");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(http_info.network_set.mask, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.network_set.mask, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "gateway");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(http_info.network_set.gateway, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.network_set.gateway, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "first_DNS");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(http_info.network_set.first_DNS, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.network_set.first_DNS, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "standby_DNS");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(http_info.network_set.standby_DNS, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(http_info.network_set.standby_DNS, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
    }else if(strcmp(json_handle, "login.json") == 0){
        if((get_key = cJSON_GetObjectItem(get_root, "account")) != NULL){
            if((strlen(get_key->valuestring) < 24) && (strcmp(http_info.system_set.account, get_key->valuestring) == 0)){
                if((get_key = cJSON_GetObjectItem(get_root, "password")) != NULL){
                    if((strlen(get_key->valuestring) < 24) && (strcmp(http_info.system_set.password, get_key->valuestring) == 0)){
                        char * send_token = http_create_token(http_info.tokenlist);
                        if(send_token != NULL){
                            data = cJSON_CreateObject();
                            cJSON_AddStringToObject(data, "token", send_token);
                        }else{
                            err_code = -7;
                            goto out;
                        }
                    }else{
                        err_code = -6;
                        goto out;
                    }
                }
            }else{
                err_code = -5;
                goto out;
            }

        }
    }else{
        if(http_check_token(http_info.tokenlist, http_info.get_msg.get_token) < 0){
            err_code = -4;
            goto out;
        }
        char get_port[12];
        int index = 10;
        memcpy(get_port, "port_0.json", 12);
        for(int i = 0; i < 4; i++){
            get_port[5]++;
            if(strcmp(json_handle, get_port) == 0){
                index = i;
                break;
            }
        }
        if(index == 10){
            err_code = -3;
            goto out;
        }
        get_key = cJSON_GetObjectItem(get_root, "baud");
        if(get_key != NULL){
            if(!(get_key->valueint > 4000000 || get_key->valueint < 600)){
                port_manage[index].serial.BAUD = get_key->valueint;
            }
            if(port_manage[index].serial.BAUD != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "data_bits");
        if(get_key != NULL){
            if(!(get_key->valueint > 8 || get_key->valueint < 5)){
                port_manage[index].serial.data_bits = get_key->valueint;
            }
            if(port_manage[index].serial.data_bits != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "check_bit");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 5){
                memcpy(port_manage[index].serial.check_bit, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(port_manage[index].serial.check_bit, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "stop_bit");
        if(get_key != NULL){
            if(!(get_key->valueint > 2 || get_key->valueint < 1)){
                port_manage[index].serial.stop_bit = get_key->valueint;
            }
            if(port_manage[index].serial.stop_bit != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_A_working_mode");
        if(get_key != NULL){
            if(!(get_key->valueint > 4 || get_key->valueint < 0)){
                port_manage[index].socket_A.working_mode = get_key->valueint;
            }
            if(port_manage[index].socket_A.working_mode != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_A_connection_max");
        if(get_key != NULL){
            if(!(get_key->valueint > 16 || get_key->valueint < 1)){
                port_manage[index].socket_A.connection_max = get_key->valueint;
            }
            if(port_manage[index].socket_A.connection_max != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_A_listen_port");
        if(get_key != NULL){
            if(!(get_key->valueint > 65535 || get_key->valueint < 0)){
                port_manage[index].socket_A.listen_port = get_key->valueint;
            }
            if(port_manage[index].socket_A.listen_port != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_A_target_ip");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(port_manage[index].socket_A.target_ip, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(port_manage[index].socket_A.target_ip, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_A_target_port");
        if(get_key != NULL){
            if(!(get_key->valueint > 65535 || get_key->valueint < 0)){
                port_manage[index].socket_A.target_port = get_key->valueint;
            }
            if(port_manage[index].socket_A.target_port != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_B_working_mode");
        if(get_key != NULL){
            if(!(get_key->valueint > 4 || get_key->valueint < 0)){
                port_manage[index].socket_B.working_mode = get_key->valueint;
            }
            if(port_manage[index].socket_B.working_mode != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_B_listen_port");
        if(get_key != NULL){
            if(!(get_key->valueint > 65535 || get_key->valueint < 0)){
                port_manage[index].socket_B.listen_port = get_key->valueint;
            }
            if(port_manage[index].socket_B.listen_port != get_key->valueint){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_B_target_ip");
        if(get_key != NULL){
            if(strlen(get_key->valuestring) < 16){
                memcpy(port_manage[index].socket_B.target_ip, get_key->valuestring, strlen(get_key->valuestring) + 1);
            }
            if(strcmp(port_manage[index].socket_B.target_ip, get_key->valuestring) != 0){
                err_code = -2;
            }
        }
        get_key = cJSON_GetObjectItem(get_root, "socket_B_target_port");
        if(get_key != NULL){
            if(!(get_key->valueint > 65535 || get_key->valueint < 0)){
                port_manage[index].socket_B.target_port = get_key->valueint;
            }
            if(port_manage[index].socket_B.target_port != get_key->valueint){
                err_code = -2;
            }
        }
    }
    out:
    cJSON_Delete(get_root);
    if(data != NULL){
        cJSON_AddItemToObject(send_root, "data", data);
    }
    cJSON_AddNumberToObject(send_root, "err_code", (double)err_code);
    if(err_code == 0){
        cJSON_AddStringToObject(send_root, "status", "successful");
    }else{
        cJSON_AddStringToObject(send_root, "status", "failed");
    }
    char * json_str = cJSON_Print(send_root);
    cJSON_Delete(send_root);
    return json_str;
}

static void http_connet_timeout_cb(uv_timer_t *uv_timer_handle){
    for(int i = 0; i < HTTP_CONNECT_MAX; i++){
        if(http_connect[i].connection_timing){
            http_connect[i].connection_timing--;
            if(!http_connect[i].connection_timing){
                uv_close((uv_handle_t *)http_connect[i].positioning, on_close);
            }
        }
    }
    for(int i = 0; i < 10; i++){
        if(http_info.tokenlist[i].timeout){
            http_info.tokenlist[i].timeout--;
            if(!http_info.tokenlist[i].timeout){
                http_delete_token(http_info.tokenlist, http_info.tokenlist[i].token);
                // memset(http_info.tokenlist[i].token, 32, 12);
                #if debug
                printf("token[%d] delete.\n", i);
                #endif
            }
        }
    }
}

static char * http_create_token(struct http_token * tokenlist){
    int i , index = 99;
    for(i = 0; i < 10; i++){
        if(tokenlist[i].timeout == 0){
            tokenlist[i].timeout = 11;
            index = i;
            break;
        }
    }
    if(index == 99){
        return NULL;
    }
    sprintf(tokenlist[index].token, "%d", rand());
    return tokenlist[index].token;
}

static int http_delete_token(struct http_token * tokenlist, char *token){
    int i;
    for(i = 0; i < 10; i++){
        if(strcmp(tokenlist[i].token, token) == 0){
            memset(tokenlist[i].token, 32, 12);
            break;
        }
    }
    if(i > 9){
        return -1;
    }
    return 0;
}

static int http_check_token(struct http_token *tokenlist, char *get_token){
    int i;
    for(i = 0; i < 10; i++){
        if(strcmp(tokenlist[i].token, get_token) == 0){
            return i;
        }
    }
    return -1;
}

static unsigned long http_get_runtime(void){
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec;
}
