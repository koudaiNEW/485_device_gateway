#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
// #include <uv.h>
#include <linux/serial.h>
// #include <libconfig.h>
#include "http_server.h"
#include "uv_server.h"
#include "cJSON.h"

extern FILE * logs_fd;
extern int alive_number;

static int open_port(serial_parameters *config)
{
    struct termios newtios = {0}; 
	int portfd = 0;

#if (__GNUC__ == 4 && __GNUC_MINOR__ == 3)
	struct my_serial_rs485 rs485conf;
	struct my_serial_rs485 rs485conf_bak;
#else
	struct serial_rs485 rs485conf;
	// struct serial_rs485 rs485conf_bak;
#endif	
	if((portfd = open(config->driver_file,O_RDWR | O_NOCTTY, 0)) < 0 )
	{
   		fprintf( logs_fd, "open serial port %s fail \n ",config->driver_file);
        fflush(logs_fd);
   		return portfd;
	}
	tcgetattr(portfd,&newtios);
	cfmakeraw(&newtios); /*see man page */
    unsigned int temp_cflag = 0;
    switch (config->data_bits)
    {
    case 5:
        temp_cflag |= CS5;
        break;
    case 6:
        temp_cflag |= CS6;
        break;
    case 7:
        temp_cflag |= CS7;
        break;
    case 8:
        temp_cflag |= CS8;
        break;
    default:
        temp_cflag |= CS8;
        break;
    }
    if(config->stop_bit == 2){
        temp_cflag |= CSTOPB;
    }else{
        temp_cflag &= ~CSTOPB;
    }
	newtios.c_cc[VMIN] = config->packaging_length; /* block until 1 char received */
	newtios.c_cc[VTIME] = config->packaging_time; /*no inter-character timer */
    if(strcmp(config->check_bit, "None") == 0 ){
        temp_cflag &= ~PARENB;
    }else if(strcmp(config->check_bit, "Odd") == 0 ){
        temp_cflag |= PARENB;
        temp_cflag |= PARODD;
    }else if(strcmp(config->check_bit, "Even") == 0 ){
        temp_cflag |= PARENB;
        temp_cflag &= ~PARODD;
    }
    if(strcmp(config->stream_control, "On") == 0 ){
        newtios.c_iflag |= IXON|IXOFF|IXANY;
    }else{
        newtios.c_iflag &= ~(IXON|IXOFF|IXANY);
    }
    temp_cflag |= CREAD | CLOCAL | HUPCL;
    newtios.c_cflag = temp_cflag;
    newtios.c_oflag &= ~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET | OFILL); 
	//newtios.c_iflag |=IGNPAR; /*ignore parity on input */
    switch (config->BAUD)
    {
    case 600:
        cfsetospeed(&newtios,B600);
        cfsetispeed(&newtios,B600);
        break;
    case 1200:
        cfsetospeed(&newtios,B1200);
        cfsetispeed(&newtios,B1200);
        break;
    case 2400:
        cfsetospeed(&newtios,B2400);
        cfsetispeed(&newtios,B2400);
        break;
    case 4800:
        cfsetospeed(&newtios,B4800);
        cfsetispeed(&newtios,B4800);
        break;
    case 9600:
        cfsetospeed(&newtios,B9600);
        cfsetispeed(&newtios,B9600);
        break;
    case 19200:
        cfsetospeed(&newtios,B19200);
        cfsetispeed(&newtios,B19200);
        break;
    case 38400:
        cfsetospeed(&newtios,B38400);
        cfsetispeed(&newtios,B38400);
        break;
    case 57600:
        cfsetospeed(&newtios,B57600);
        cfsetispeed(&newtios,B57600);
        break;
    case 115200:
        cfsetospeed(&newtios,B115200);
        cfsetispeed(&newtios,B115200);
        break;
    case 230400:
        cfsetospeed(&newtios,B230400);
        cfsetispeed(&newtios,B230400);
        break;
    case 460800:
        cfsetospeed(&newtios,B460800);
        cfsetispeed(&newtios,B460800);
        break;
    case 500000:
        cfsetospeed(&newtios,B500000);
        cfsetispeed(&newtios,B500000);
        break;
    case 576000:
        cfsetospeed(&newtios,B576000);
        cfsetispeed(&newtios,B576000);
        break;
    case 921600:
        cfsetospeed(&newtios,B921600);
        cfsetispeed(&newtios,B921600);
        break;
    case 1000000:
        cfsetospeed(&newtios,B1000000);
        cfsetispeed(&newtios,B1000000);
        break;
    case 1152000:
        cfsetospeed(&newtios,B1152000);
        cfsetispeed(&newtios,B1152000);
        break;
    case 1500000:
        cfsetospeed(&newtios,B1500000);
        cfsetispeed(&newtios,B1500000);
        break;
    case 2000000:
        cfsetospeed(&newtios,B2000000);
        cfsetispeed(&newtios,B2000000);
        break;
    case 2500000:
        cfsetospeed(&newtios,B2500000);
        cfsetispeed(&newtios,B2500000);
        break;
    case 3000000:
        cfsetospeed(&newtios,B3000000);
        cfsetispeed(&newtios,B3000000);
        break;
    case 3500000:
        cfsetospeed(&newtios,B3500000);
        cfsetispeed(&newtios,B3500000);
        break;
    case 4000000:
        cfsetospeed(&newtios,B4000000);
        cfsetispeed(&newtios,B4000000);
        break;

    default:
        cfsetospeed(&newtios,B115200);
        cfsetispeed(&newtios,B115200);
        break;
    }
	/* register cleanup stuff */
    /*
	atexit(reset_tty_atexit);
	memset(&sa,0,sizeof sa);
	sa.sa_handler = reset_tty_handler;
	sigaction(SIGHUP,&sa,NULL);
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGPIPE,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
    */
	/*apply modified termios */
	tcflush(portfd,TCIFLUSH);
	tcsetattr(portfd,TCSADRAIN,&newtios);
	
		
	if (ioctl (portfd, TIOCGRS485, &rs485conf) < 0) 
	{
		/* Error handling.*/ 
		fprintf(logs_fd, "ioctl TIOCGRS485 error.\n");
	}
	/* Enable RS485 mode: */
	rs485conf.flags |= SER_RS485_ENABLED;

	/* Set logical level for RTS pin equal to 1 when sending: */
	rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	//rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;

	/* set logical level for RTS pin equal to 0 after sending: */ 
	rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);
	//rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

	/* Set rts delay after send, if needed:   ms*/ 
	rs485conf.delay_rts_after_send = 20;

	if (ioctl (portfd, TIOCSRS485, &rs485conf) < 0)
	{
		/* Error handling.*/ 
		fprintf(logs_fd, "ioctl TIOCSRS485 error.\n");
	}

	// if (ioctl (portfd, TIOCGRS485, &rs485conf_bak) < 0)
	// {
	// 	/* Error handling.*/ 
	// 	fprintf(logs_fd, "ioctl TIOCGRS485 error.\n");
	// }
    fflush(logs_fd);
	return portfd;
}

// int get_uart_seting(config_setting_t * set_class, set_parameters *port ){
//     int rs485_alive_number, i, strcount;
//     char set_port[7];
//     memcpy(set_port, "port_0", 7);

//     if((rs485_alive_number = config_setting_length(set_class)) > PORT_SET_MAX){
//         return -12;
//     }
//     #if debug
//     printf("rs485_alive_number: %d\n", rs485_alive_number);
//     #endif
//     for(i = 0; i < rs485_alive_number; i++){
//         set_port[5]++;
//         config_setting_t *group_485 = config_setting_get_member(set_class, set_port);
//         char *get_str;

//         if(config_setting_lookup_string(group_485, "driver_file", (const char **)(&get_str))){
//             // memcpy(port[i].serial.driver_file, get_str, 24);
//             if((strcount = strlen(get_str) + 1) < 25){
//                 memcpy(port[i].serial.driver_file, get_str, strcount);
//             }else return -3;
//             #if debug 
//             printf("driver_file: %s\n", port[i].serial.driver_file);
//             #endif
//         }else return -3;
//         if(config_setting_lookup_int(group_485, "baud", &port[i].serial.BAUD)){
//             if(port[i].serial.BAUD > 4000000){
//                 port[i].serial.BAUD = 4000000;
//             }else if(port[i].serial.BAUD < 600){
//                 port[i].serial.BAUD = 600;
//             }
//         }
//         else return -4;
//         if(config_setting_lookup_int(group_485, "data_bits", &port[i].serial.data_bits)){
//             if(port[i].serial.data_bits > 8){
//                 port[i].serial.data_bits = 8;
//             }else if(port[i].serial.data_bits < 5){
//                 port[i].serial.data_bits = 5;
//             }
//         }
//         else return -5;
//         if(config_setting_lookup_string(group_485, "check_bit", (const char **)(&get_str))){
//             // memcpy(port[i].serial.check_bit, get_str, 5);
//             if((strcount = strlen(get_str) + 1) < 6){
//                 memcpy(port[i].serial.check_bit, get_str, strcount);
//             }else return -106;
//             #if debug 
//             printf("check_bit: %s\n", port[i].serial.check_bit);
//             #endif
//             if(!((strcmp(port[i].serial.check_bit, "None") == 0) || (strcmp(port[i].serial.check_bit, "Odd") == 0) || (strcmp(port[i].serial.check_bit, "Even") == 0))){
//             // if(!(port[i].serial.check_bit == "None" || port[i].serial.check_bit == "Odd" || port[i].serial.check_bit == "Even")){
//                 memcpy(port[i].serial.check_bit, "None", 5);
//             }
//         }
//         else return -6;
//         if(config_setting_lookup_string(group_485, "stream_control", (const char **)(&get_str))){
//             // memcpy(port[i].serial.stream_control, get_str, 5);
//             if((strcount = strlen(get_str) + 1) < 6){
//                 memcpy(port[i].serial.stream_control, get_str, strcount);
//             }else return -7;
//             #if debug 
//             printf("stream_control: %s\n", port[i].serial.stream_control);
//             #endif
//             if(!((strcmp(port[i].serial.stream_control, "Off") == 0) || (strcmp(port[i].serial.stream_control, "On") == 0))){
//             // if(!(port[i].serial.stream_control == "Off" || port[i].serial.stream_control == "On")){
//                 memcpy(port[i].serial.stream_control, "Off", 4);
//             }
//         }
//         else return -7;
//         if(config_setting_lookup_int(group_485, "stop_bit", &port[i].serial.stop_bit)){
//             if(port[i].serial.stop_bit > 2){
//                 port[i].serial.stop_bit = 2;
//             }else if(port[i].serial.stop_bit < 1){
//                 port[i].serial.stop_bit = 1;
//             }
//         }
//         else return -8;
//         if(config_setting_lookup_int(group_485, "packaging_length", &port[i].serial.packaging_length)){
//             if(port[i].serial.packaging_length > 255){
//                 port[i].serial.packaging_length = 255;
//             }else if(port[i].serial.packaging_length < 1){
//                 port[i].serial.packaging_length = 1;
//             }
//         }
//         else return -9;
//         if(config_setting_lookup_int(group_485, "packaging_time", &port[i].serial.packaging_time)){
//             if(port[i].serial.packaging_time > 255){
//                 port[i].serial.packaging_time = 255;
//             }else if(port[i].serial.packaging_time < 0){
//                 port[i].serial.packaging_time = 0;
//             }
//         }
//         else return -10;

//         if((port[i].serial.fd = open_port(&port[i].serial)) < 0){
//             return -11;
//         }
//     }
//     return rs485_alive_number;
// }


// int get_socket_seting(config_setting_t * set_class, set_parameters *port){
//     int i, j, alive, strcount;
//     char set_port[7];
//     memcpy(set_port, "port_0", 7);

//     if((alive = config_setting_length(set_class)) > PORT_SET_MAX){
//         return -14;
//     }
//     for(i = 0; i < alive; i++){
//         set_port[5]++;
//         config_setting_t *group_socket = config_setting_get_member(set_class, set_port);
//         char * get_target_ip;

//         if(config_setting_lookup_int(group_socket, "socket_A_working_mode", &port[i].socket_A.working_mode)){
//             if(port[i].socket_A.working_mode > 4 || port[i].socket_A.working_mode < 0){
//                 port[i].socket_A.working_mode = 0;
//             }
//         }else return -15;
//         if(config_setting_lookup_int(group_socket, "socket_A_listen_port", &port[i].socket_A.listen_port)){
//             if(port[i].socket_A.listen_port > 65535){
//                 port[i].socket_A.listen_port = 65535;
//             }else if(port[i].socket_A.listen_port < 1){
//                 port[i].socket_A.listen_port = 1;
//             }
//         }else return -16;
//         if(config_setting_lookup_int(group_socket, "socket_A_target_port", &port[i].socket_A.target_port)){
//             if(port[i].socket_A.target_port > 65535){
//                 port[i].socket_A.target_port = 65535;
//             }else if(port[i].socket_A.target_port < 1){
//                 port[i].socket_A.target_port = 1;
//             }
//         }else return -17;
//         if(config_setting_lookup_int(group_socket, "socket_A_connection_max", &port[i].socket_A.connection_max)){
//             if(port[i].socket_A.connection_max > 16){
//                 port[i].socket_A.connection_max = 16;
//             }else if(port[i].socket_A.connection_max < 1){
//                 port[i].socket_A.connection_max = 1;
//             }
//         }else return -18;
//         if(config_setting_lookup_int(group_socket, "socket_B_working_mode", &port[i].socket_B.working_mode)){
//             if(port[i].socket_B.working_mode > 4 || port[i].socket_B.working_mode < 0){
//                 port[i].socket_B.working_mode = 0;
//             }
//         }else return -19;
//         if(config_setting_lookup_int(group_socket, "socket_B_target_port", &port[i].socket_B.target_port)){
//             if(port[i].socket_B.target_port > 65535){
//                 port[i].socket_B.target_port = 65535;
//             }else if(port[i].socket_B.target_port < 1){
//                 port[i].socket_B.target_port = 1;
//             }
//         }else return -20;
//         if(config_setting_lookup_string(group_socket, "socket_A_target_ip", (const char **)(&get_target_ip))){
//             // memcpy(port[i].socket_A.target_ip, get_target_ip, 16);
//             if((strcount = strlen(get_target_ip) + 1) < 17){
//                 memcpy(port[i].socket_A.target_ip, get_target_ip, strcount);
//             }else return -21;
//             #if debug
//             printf("target_ip A: %s\n", port[i].socket_A.target_ip);
//             #endif
//         }else return -21;
//         if(config_setting_lookup_string(group_socket, "socket_B_target_ip", (const char **)(&get_target_ip))){
//             // memcpy(port[i].socket_B.target_ip, get_target_ip, 16);
//             if((strcount = strlen(get_target_ip) + 1) < 17){
//                 memcpy(port[i].socket_B.target_ip, get_target_ip, strcount);
//             }else return -22;
//             #if debug
//             printf("target_ip B: %s\n", port[i].socket_B.target_ip);
//             #endif
//         }else return -22;
//         if(config_setting_lookup_int(group_socket, "socket_B_listen_port", &port[i].socket_B.listen_port)){
//             if(port[i].socket_B.listen_port > 65535){
//                 port[i].socket_B.listen_port = 65535;
//             }else if(port[i].socket_B.listen_port < 1){
//                 port[i].socket_B.listen_port = 1;
//             }
//         }else return -23;
//     }
//     return alive;
// }

// int get_device_seting(config_setting_t * set_class, struct http_main *device){
//     char * get_device_str;
//     int strcount;

//     if(config_setting_lookup_string(set_class, "device_name", (const char **)(&get_device_str))){
//         // memcpy(device->system_set.device_name, get_device_str, 24);
//         if((strcount = strlen(get_device_str) + 1) < 25){
//             memcpy(device->system_set.device_name, get_device_str, strcount);
//         }else return -26;
//         #if debug
//         printf("device_name: %s\n", device->system_set.device_name);
//         #endif
//     }else return -26;
//     if(config_setting_lookup_string(set_class, "firmware_version", (const char **)(&get_device_str))){
//         // memcpy(device->system_set.firmware_version, get_device_str, 24);
//         if((strcount = strlen(get_device_str) + 1) < 25){
//             memcpy(device->system_set.firmware_version, get_device_str, strcount);
//         }else return -27;
//         #if debug
//         printf("firmware_version: %s\n", device->system_set.firmware_version);
//         #endif
//     }else return -27;
//     if(config_setting_lookup_string(set_class, "type", (const char **)(&get_device_str))){
//         // memcpy(device->system_set.type, get_device_str, 12);
//         if((strcount = strlen(get_device_str) + 1) < 13){
//             memcpy(device->system_set.type, get_device_str, strcount);
//         }else return -28;
//         #if debug
//         printf("type: %s\n", device->system_set.type);
//         #endif
//     }else return -28;
//     if(config_setting_lookup_string(set_class, "mac", (const char **)(&get_device_str))){
//         // memcpy(device->network_set.mac, get_device_str, 18);
//         if((strcount = strlen(get_device_str) + 1) < 19){
//             memcpy(device->network_set.mac, get_device_str, strcount);
//         }else return -29;
//         #if debug
//         printf("mac: %s\n", device->network_set.mac);
//         #endif
//     }else return -29;
//     if(config_setting_lookup_string(set_class, "ip", (const char **)(&get_device_str))){
//         // memcpy(device->network_set.ip, get_device_str, 16);
//         if((strcount = strlen(get_device_str) + 1) < 17){
//             memcpy(device->network_set.ip, get_device_str, strcount);
//         }else return -30;
//         #if debug
//         printf("ip: %s\n", device->network_set.ip);
//         #endif
//     }else return -30;
//     if(config_setting_lookup_string(set_class, "mask", (const char **)(&get_device_str))){
//         // memcpy(device->network_set.mask, get_device_str, 16);
//         if((strcount = strlen(get_device_str) + 1) < 17){
//             memcpy(device->network_set.mask, get_device_str, strcount);
//         }else return -31;
//         #if debug
//         printf("mask: %s\n", device->network_set.mask);
//         #endif
//     }else return -31;
//     if(config_setting_lookup_string(set_class, "gateway", (const char **)(&get_device_str))){
//         // memcpy(device->network_set.gateway, get_device_str, 16);
//         if((strcount = strlen(get_device_str) + 1) < 17){
//             memcpy(device->network_set.gateway, get_device_str, strcount);
//         }else return -32;
//         #if debug
//         printf("gateway: %s\n", device->network_set.gateway);
//         #endif
//     }else return -32;
//     if(config_setting_lookup_string(set_class, "first_DNS", (const char **)(&get_device_str))){
//         // memcpy(device->network_set.first_DNS, get_device_str, 16);
//         if((strcount = strlen(get_device_str) + 1) < 17){
//             memcpy(device->network_set.first_DNS, get_device_str, strcount);
//         }else return -33;
//         #if debug
//         printf("first_DNS: %s\n", device->network_set.first_DNS);
//         #endif
//     }else return -33;
//     if(config_setting_lookup_string(set_class, "standby_DNS", (const char **)(&get_device_str))){
//         // memcpy(device->network_set.standby_DNS, get_device_str, 16);
//         if((strcount = strlen(get_device_str) + 1) < 17){
//             memcpy(device->network_set.standby_DNS, get_device_str, strcount);
//         }else return -34;
//         #if debug
//         printf("standby_DNS: %s\n", device->network_set.standby_DNS);
//         #endif
//     }else return -34;
//     if(config_setting_lookup_int(set_class, "IP_acquisition_method", &device->network_set.IP_acquisition_method)){
//         #if debug
//         printf("IP_acquisition_method: %d\n", device->network_set.IP_acquisition_method);
//         #endif
//         if(device->network_set.IP_acquisition_method > 1){
//             device->network_set.IP_acquisition_method = 1;
//         }else if(device->network_set.IP_acquisition_method < 0){
//             device->network_set.IP_acquisition_method = 0;
//         }
//     }else return -35;
//     return 0;
// }

/*DO NOT USE*/
// int set_config_file(const char * cfg_file, struct http_main *device, struct set_parameters *port){
//     #if debug
//     printf("on config_read_file.\n");
//     #endif
//     int i;
//     char port_name[7];
//     config_t config;
//     config_setting_t *setting = NULL;
//     config_setting_t *set_port = NULL;
//     config_setting_t *member = NULL;
//     // FILE *cfg = fopen(cfg_file, "rt+");
//     // if(cfg == NULL){
//     //     return -5;
//     // }
//     // rewind(cfg);
//     #if debug
//     printf("after malloc.\n");
//     #endif

//     config_init(&config);

//     #if debug
//     printf("after config_init.\n");
//     #endif
//     if(!config_read_file(&config, cfg_file)){
//         return -1;
//     }
//     #if debug
//     printf("config_read_file.\n");
//     #endif

//     if((setting = config_lookup(&config, "device_config")) != NULL){
//         if((member = config_setting_get_member(setting, "device_name")) != NULL){
//             config_setting_set_string(member, device->system_set.device_name);
//         }
//         if((member = config_setting_get_member(setting, "mac")) != NULL){
//             config_setting_set_string(member, device->network_set.mac);
//         }
//         if((member = config_setting_get_member(setting, "ip")) != NULL){
//             config_setting_set_string(member, device->network_set.ip);
//         }
//         if((member = config_setting_get_member(setting, "mask")) != NULL){
//             config_setting_set_string(member, device->network_set.mask);
//         }
//         if((member = config_setting_get_member(setting, "gateway")) != NULL){
//             config_setting_set_string(member, device->network_set.gateway);
//         }
//         if((member = config_setting_get_member(setting, "first_DNS")) != NULL){
//             config_setting_set_string(member, device->network_set.first_DNS);
//         }
//         if((member = config_setting_get_member(setting, "standby_DNS")) != NULL){
//             config_setting_set_string(member, device->network_set.standby_DNS);
//         }
//         if((member = config_setting_get_member(setting, "IP_acquisition_method")) != NULL){
//             config_setting_set_int(member, device->network_set.IP_acquisition_method);
//         }
//     }else {
//         config_destroy(&config);
//         return -2;
//     }
//     #if debug
//     printf("config_lookup device_config.\n");
//     #endif

//     // char *port_name = (char *)malloc(7);
//     memcpy(port_name, "port_0", 7);
//     if((setting = config_lookup(&config, "rs485_config")) != NULL){
//         for(i = 0; i < alive_number; i++){
//             port_name[5]++;
//             if((set_port = config_setting_get_member(setting, port_name)) != NULL){
//                 if((member = config_setting_get_member(set_port, "baud")) != NULL){
//                     config_setting_set_int(member, port[i].serial.BAUD);
//                 }
//                 if((member = config_setting_get_member(set_port, "data_bits")) != NULL){
//                     config_setting_set_int(member, port[i].serial.data_bits);
//                 }
//                 if((member = config_setting_get_member(set_port, "check_bit")) != NULL){
//                     config_setting_set_string(member, port[i].serial.check_bit);
//                 }
//                 if((member = config_setting_get_member(set_port, "stop_bit")) != NULL){
//                     config_setting_set_int(member, port[i].serial.stop_bit);
//                 }
//                 if((member = config_setting_get_member(set_port, "packaging_length")) != NULL){
//                     config_setting_set_int(member, port[i].serial.packaging_length);
//                 }
//                 if((member = config_setting_get_member(set_port, "packaging_time")) != NULL){
//                     config_setting_set_int(member, port[i].serial.packaging_time);
//                 }
//                 if((member = config_setting_get_member(set_port, "stream_control")) != NULL){
//                     config_setting_set_string(member, port[i].serial.stream_control);
//                 }
//             }
//         }
//     }else {
//         config_destroy(&config);
//         return -3;
//     }
//     #if debug
//     printf("config_lookup rs485_config.\n");
//     #endif
    
//     port_name[5] = '0';
//     if((setting = config_lookup(&config, "socket_config")) != NULL){
//         for(i = 0; i < alive_number; i++){
//             port_name[5]++;
//             if((set_port = config_setting_get_member(setting, port_name)) != NULL){
//                 if((member = config_setting_get_member(set_port, "socket_A_working_mode")) != NULL){
//                     config_setting_set_int(member, port[i].socket_A.working_mode);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_A_listen_port")) != NULL){
//                     config_setting_set_int(member, port[i].socket_A.listen_port);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_A_target_ip")) != NULL){
//                     config_setting_set_string(member, port[i].socket_A.target_ip);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_A_target_port")) != NULL){
//                     config_setting_set_int(member, port[i].socket_A.target_port);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_A_connection_max")) != NULL){
//                     config_setting_set_int(member, port[i].socket_A.connection_max);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_B_working_mode")) != NULL){
//                     config_setting_set_int(member, port[i].socket_B.working_mode);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_B_listen_port")) != NULL){
//                     config_setting_set_int(member, port[i].socket_B.listen_port);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_B_target_ip")) != NULL){
//                     config_setting_set_string(member, port[i].socket_B.target_ip);
//                 }
//                 if((member = config_setting_get_member(set_port, "socket_B_target_port")) != NULL){
//                     config_setting_set_int(member, port[i].socket_B.target_port);
//                 }
//             }
//         }
//     }else {
//         config_destroy(&config);
//         return -4;
//     }
//     #if debug
//     printf("config_lookup socket_config.\n");
//     #endif
//     // rewind(cfg);
//     config_write_file(&config, cfg_file);
//     #if debug
//     printf("config_write_file.\n");
//     #endif
//     config_destroy(&config);
//     #if debug
//     printf("config_destroy.\n");
//     #endif
//     return 0;
// }

int set_json_file(const char * json_file, struct http_main *device, struct set_parameters *port){
    int json_fd, errcode = 0;
    if((json_fd = open(json_file, O_RDWR)) < 0){
        #if debug
        printf("open file fail\n");
        #endif
        errcode = -1;
        goto out;
    }
    struct stat json_fd_info;
    fstat(json_fd, &json_fd_info);
    char *json_buf = NULL;
    json_buf = malloc(json_fd_info.st_size + 1024);
    if(read(json_fd, json_buf,  (size_t)json_fd_info.st_size) < 0){
        #if debug
        printf("read file fail\n");
        #endif
        errcode = -2;
        goto out;
    } 
    json_buf[json_fd_info.st_size + 1] = 0;
    cJSON *json_root = cJSON_Parse(json_buf);
    cJSON *get_key = NULL;
    cJSON *get_member = NULL;
    char portname[7];
    memcpy(portname, "port_0", 7);
    if(((get_key = cJSON_GetObjectItem(json_root, "port_config")) == NULL)){
        errcode = -3;
        goto out;
    }
    for(int i = 0; i < alive_number; i++){
        portname[5]++;
        if((get_member = cJSON_GetObjectItem(get_key, portname)) != NULL){
            cJSON_ReplaceItemInObject(get_member, "baud", cJSON_CreateNumber((double)port[i].serial.BAUD));
            cJSON_ReplaceItemInObject(get_member, "data_bits", cJSON_CreateNumber((double)port[i].serial.data_bits));
            cJSON_ReplaceItemInObject(get_member, "check_bit", cJSON_CreateString(port[i].serial.check_bit));
            cJSON_ReplaceItemInObject(get_member, "stop_bit", cJSON_CreateNumber((double)port[i].serial.stop_bit));
            cJSON_ReplaceItemInObject(get_member, "packaging_length", cJSON_CreateNumber((double)port[i].serial.packaging_length));
            cJSON_ReplaceItemInObject(get_member, "packaging_time", cJSON_CreateNumber((double)port[i].serial.packaging_time));
            cJSON_ReplaceItemInObject(get_member, "stream_control", cJSON_CreateString(port[i].serial.stream_control));
            cJSON_ReplaceItemInObject(get_member, "socket_A_working_mode", cJSON_CreateNumber((double)port[i].socket_A.working_mode));
            cJSON_ReplaceItemInObject(get_member, "socket_A_listen_port", cJSON_CreateNumber((double)port[i].socket_A.listen_port));
            cJSON_ReplaceItemInObject(get_member, "socket_A_target_ip", cJSON_CreateString(port[i].socket_A.target_ip));
            cJSON_ReplaceItemInObject(get_member, "socket_A_target_port", cJSON_CreateNumber((double)port[i].socket_A.target_port));
            cJSON_ReplaceItemInObject(get_member, "socket_A_connection_max", cJSON_CreateNumber((double)port[i].socket_A.connection_max));
            cJSON_ReplaceItemInObject(get_member, "socket_B_working_mode", cJSON_CreateNumber((double)port[i].socket_B.working_mode));
            cJSON_ReplaceItemInObject(get_member, "socket_B_listen_port", cJSON_CreateNumber((double)port[i].socket_B.listen_port));
            cJSON_ReplaceItemInObject(get_member, "socket_B_target_ip", cJSON_CreateString(port[i].socket_B.target_ip));
            cJSON_ReplaceItemInObject(get_member, "socket_B_target_port", cJSON_CreateNumber((double)port[i].socket_B.target_port));
        }
    }
    if(((get_member = cJSON_GetObjectItem(json_root, "device_config")) != NULL)){
        cJSON_ReplaceItemInObject(get_member, "device_name", cJSON_CreateString(device->system_set.device_name));
        cJSON_ReplaceItemInObject(get_member, "type", cJSON_CreateString(device->system_set.type));
        cJSON_ReplaceItemInObject(get_member, "mac", cJSON_CreateString(device->network_set.mac));
        cJSON_ReplaceItemInObject(get_member, "ip", cJSON_CreateString(device->network_set.ip));
        cJSON_ReplaceItemInObject(get_member, "mask", cJSON_CreateString(device->network_set.mask));
        cJSON_ReplaceItemInObject(get_member, "gateway", cJSON_CreateString(device->network_set.gateway));
        cJSON_ReplaceItemInObject(get_member, "first_DNS", cJSON_CreateString(device->network_set.first_DNS));
        cJSON_ReplaceItemInObject(get_member, "standby_DNS", cJSON_CreateString(device->network_set.standby_DNS));
        cJSON_ReplaceItemInObject(get_member, "IP_acquisition_method", cJSON_CreateNumber((double)device->network_set.IP_acquisition_method));
    }
    char * json_str = cJSON_Print(json_root);
    lseek(json_fd, 0, SEEK_SET);
    ftruncate(json_fd, 0);
    if(write(json_fd, json_str,  strlen(json_str)) < 0){
        #if debug
        printf("write file fail\n");
        #endif
        errcode = -4;
        goto out;
    } 
    out:
    if(json_fd != 0){
        close(json_fd);
    }
    if(json_root != NULL){
        cJSON_Delete(json_root);
    }
    if(json_buf != NULL){
        free(json_buf);
    }
    if(json_str != NULL){
        free(json_str);
    }
    return errcode;
}

int read_json_config(const char *json_file, struct http_main *device, struct set_parameters *port){
    int json_fd, errcode = 0;
    if((json_fd = open(json_file, O_RDONLY)) < 0){
        #if debug
        printf("open file fail\n");
        #endif
        errcode = -1;
        goto out;
    }
    struct stat json_fd_info;
    fstat(json_fd, &json_fd_info);
    char *json_buf = NULL;
    json_buf = malloc(json_fd_info.st_size + 1024);
    if(read(json_fd, json_buf,  (size_t)json_fd_info.st_size) < 0){
        #if debug
        printf("read file fail\n");
        #endif
        errcode = -2;
        goto out;
    } 
    json_buf[json_fd_info.st_size + 1] = 0;

    cJSON *json_root = cJSON_Parse(json_buf);
    cJSON *get_key = NULL;
    cJSON *get_member = NULL;
    cJSON *get_option = NULL;
    char portname[7];
    memcpy(portname, "port_0", 7);
    if(((get_key = cJSON_GetObjectItem(json_root, "port_config")) == NULL)){
        errcode = -3;
        goto out;
    }
    alive_number = cJSON_GetArraySize(get_key);
    int str_len;
    for(int i = 0; i < alive_number; i++){
        portname[5]++;
        if((get_member = cJSON_GetObjectItem(get_key, portname)) != NULL){
            if((get_option = cJSON_GetObjectItem(get_member, "driver_file")) != NULL){
                if((str_len = strlen(get_option->valuestring) + 1) < 25){
                    memcpy(port[i].serial.driver_file, get_option->valuestring, str_len);
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "baud")) != NULL){
                if(get_option->valueint > 4000000 || get_option->valueint < 600){
                    port[i].serial.BAUD = 115200;
                }else{
                    port[i].serial.BAUD = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "data_bits")) != NULL){
                if(get_option->valueint > 8 || get_option->valueint < 5){
                    port[i].serial.data_bits = 8;
                }else{
                    port[i].serial.data_bits = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "check_bit")) != NULL){
                if((str_len = strlen(get_option->valuestring) + 1) < 6){
                    memcpy(port[i].serial.check_bit, get_option->valuestring, str_len);
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "stop_bit")) != NULL){
                if(get_option->valueint > 2 || get_option->valueint < 1){
                    port[i].serial.stop_bit = 1;
                }else{
                    port[i].serial.stop_bit = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "packaging_length")) != NULL){
                if(get_option->valueint > 255 || get_option->valueint < 0){
                    port[i].serial.packaging_length = 0;
                }else{
                    port[i].serial.packaging_length = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "packaging_time")) != NULL){
                if(get_option->valueint > 255 || get_option->valueint < 0){
                    port[i].serial.packaging_time = 0;
                }else{
                    port[i].serial.packaging_time = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "stream_control")) != NULL){
                if((str_len = strlen(get_option->valuestring) + 1) < 6){
                    memcpy(port[i].serial.stream_control, get_option->valuestring, str_len);
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_A_working_mode")) != NULL){
                if(get_option->valueint > 4 || get_option->valueint < 0){
                    port[i].socket_A.working_mode = 0;
                }else{
                    port[i].socket_A.working_mode = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_A_listen_port")) != NULL){
                if(get_option->valueint > 65535 || get_option->valueint < 0){
                    port[i].socket_A.listen_port = 2001 + i;
                }else{
                    port[i].socket_A.listen_port = get_option->valueint;
                }
                // #if debug
                // printf("socket_A.listen_port: %d\n", port[i].socket_A.listen_port);
                // #endif
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_A_target_ip")) != NULL){
                if((str_len = strlen(get_option->valuestring) + 1) < 17){
                    memcpy(port[i].socket_A.target_ip, get_option->valuestring, str_len);
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_A_target_port")) != NULL){
                if(get_option->valueint > 65535 || get_option->valueint < 0){
                    port[i].socket_A.target_port = 0;
                }else{
                    port[i].socket_A.target_port = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_A_connection_max")) != NULL){
                if(get_option->valueint > 16 || get_option->valueint < 0){
                    port[i].socket_A.connection_max = 16;
                }else{
                    port[i].socket_A.connection_max = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_B_working_mode")) != NULL){
                if(get_option->valueint > 2 || get_option->valueint < 0){
                    port[i].socket_B.working_mode = 0;
                }else{
                    port[i].socket_B.working_mode = get_option->valueint;
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_B_listen_port")) != NULL){
                if(get_option->valueint > 65535 || get_option->valueint < 0){
                    port[i].socket_B.listen_port = 3001 + i;
                }else{
                    port[i].socket_B.listen_port = get_option->valueint;
                }
                // #if debug
                // printf("socket_B.listen_port: %d\n", port[i].socket_B.listen_port);
                // #endif
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_B_target_ip")) != NULL){
                if((str_len = strlen(get_option->valuestring) + 1) < 17){
                    memcpy(port[i].socket_B.target_ip, get_option->valuestring, str_len);
                }
            }
            if((get_option = cJSON_GetObjectItem(get_member, "socket_B_target_port")) != NULL){
                if(get_option->valueint > 65535 || get_option->valueint < 0){
                    port[i].socket_B.target_port = 0;
                }else{
                    port[i].socket_B.target_port = get_option->valueint;
                }
            }
        }
        if((port[i].serial.fd = open_port(&port[i].serial)) < 0){
            errcode = -4;
            goto out;
        }
    }
    if(((get_key = cJSON_GetObjectItem(json_root, "device_config")) != NULL)){
        if((get_member = cJSON_GetObjectItem(get_key, "device_name")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 25){
                memcpy(device->system_set.device_name, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "firmware_version")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 25){
                memcpy(device->system_set.firmware_version, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "type")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 13){
                memcpy(device->system_set.type, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "mac")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 19){
                memcpy(device->network_set.mac, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "ip")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 17){
                memcpy(device->network_set.ip, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "mask")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 17){
                memcpy(device->network_set.mask, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "gateway")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 17){
                memcpy(device->network_set.gateway, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "first_DNS")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 17){
                memcpy(device->network_set.first_DNS, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "standby_DNS")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 17){
                memcpy(device->network_set.standby_DNS, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "IP_acquisition_method")) != NULL){
            if(get_member->valueint > 1 || get_member->valueint < 0){
                device->network_set.IP_acquisition_method = 0;
            }else{
                device->network_set.IP_acquisition_method = get_member->valueint;
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "account")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 25){
                memcpy(device->system_set.account, get_member->valuestring, str_len);
            }
        }
        if((get_member = cJSON_GetObjectItem(get_key, "password")) != NULL){
            if((str_len = strlen(get_member->valuestring) + 1) < 25){
                memcpy(device->system_set.password, get_member->valuestring, str_len);
            }
        }
    }else{
        errcode = -5;
        goto out;
    }
    out:
    if(json_fd != 0){
        close(json_fd);
    }
    if(json_root != NULL){
        cJSON_Delete(json_root);
    }
    if(json_buf != NULL){
        free(json_buf);
    }
    return errcode;
}




