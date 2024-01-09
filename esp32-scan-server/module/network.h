#ifndef __NETWORK_H
#define __NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "sdkconfig.h"
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_ping.h"
#include "ping/ping_sock.h"
#include "driver/gpio.h"

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>

//#define __DEBUG__    //宏定义调试打印开关

#ifdef __DEBUG__
    #define DPRINTF(...) printf(__VA_ARGS__)  //宏调试打印函数定义
#else
    #define DPRINTF(...)
#endif


#define WIFI_SSID "tony"
#define WIFI_PASS "1008611123"

#if defined(CONFIG_EXAMPLE_IPV4)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#elif defined(CONFIG_EXAMPLE_IPV6)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#else
#define HOST_IP_ADDR "192.168.1.100"
#endif

#define PORT 6666



void connect_wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void device_connect_wifi_ap(uint8_t *wifi_ssid, uint8_t *wifi_passwd);
int  udp_client_init(void);
int  tcp_client_init(char ip_addr[]);
void udp_client_task(void *pvParameters);
void tcp_client_task(void *pvParameters);
void key_udp_task(void *pvParameters);
void key_tcp_task(void *pvParameters);

int  device_connect_to_server(char *target_ip);
int  device_scan_dest_server(char ip_addr[]);
void ping_ip_success(esp_ping_handle_t handle, void *args);
void ping_ip_timeout(esp_ping_handle_t handle, void *args);
void ping_ip_end(esp_ping_handle_t handle, void *args);
void device_ping_ip_addr(char *ip_str);

extern esp_ping_handle_t ping;
extern int ping_flag;
extern int tcp_sock;
extern int udp_sock;
extern struct sockaddr_in dest_addr;

#endif