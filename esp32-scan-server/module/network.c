#include "network.h"

//ESP-IDF v5.1
static const char *TAG = "wifi_station";

int tcp_sock = -1;
int udp_sock = -1;

esp_ping_handle_t ping;
int ping_flag = -1;
enum {
    PING_IP_OK = 0,
    PING_IP_ERR
};
struct sockaddr_in dest_addr;

/**
  * @brief  连接WiFi事件处理器---处理WiFi和IP事件
  * @param  arg:用户定义的参数
  * @param  event_base:事件基类型
  * @param  event_id:事件具体ID
  * @param  event_data:指向事件数据的指针
  * @retval void
  */
void connect_wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char addr_str[IP4ADDR_STRLEN_MAX];
        ESP_LOGI(TAG, "got ip:%s", esp_ip4addr_ntoa(&event->ip_info.ip, addr_str, IP4ADDR_STRLEN_MAX));
    }
}

/**
  * @brief  设备连接WiFi AP热点
  * @param  wifi_ssid:WiFi 热点ID
  * @param  wifi_passwd:WiFi密码
  * @retval void
  */
void device_connect_wifi_ap(uint8_t *wifi_ssid, uint8_t *wifi_passwd)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &connect_wifi_ap_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_wifi_ap_event_handler, NULL));

    wifi_config_t wifi_config;
    memcpy(wifi_config.sta.ssid, wifi_ssid, strlen((char *)wifi_ssid)+1);
    memcpy(wifi_config.sta.password, wifi_passwd, strlen((char *)wifi_passwd)+1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
  * @brief  udp客户端初始化
  * @param  None
  * @retval 成功返回0，失败返回-1
  */
int udp_client_init(void)
{
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;  
#if defined(CONFIG_EXAMPLE_IPV4)
    //struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(host_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
    struct sockaddr_in6 dest_addr = {0};
    inet6_aton(HOST_IP_ADDR, &dest_addr.sin6_addr);
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(PORT);
    dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
    struct sockaddr_storage dest_addr = {0};
    ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_DGRAM, &ip_protocol, &addr_family, &dest_addr));
#endif
    extern int udp_sock;
    udp_sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (udp_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
    return 0;
}

/**
  * @brief  tcp客户端初始化
  * @param  None
  * @retval 成功返回0，失败返回-1
  */
int tcp_client_init(char ip_addr[])
{
    //char host_ip[128] = HOST_IP_ADDR;
    char host_ip[128] = {0};
    memcpy(host_ip, ip_addr, strlen(ip_addr));
    int addr_family = 0;
    int ip_protocol = 0;
    int err = 0;
#if defined(CONFIG_EXAMPLE_IPV4)
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(host_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
    struct sockaddr_in6 dest_addr = { 0 };
    inet6_aton(host_ip, &dest_addr.sin6_addr);
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(PORT);
    dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
    struct sockaddr_storage dest_addr = { 0 };
    ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif
    extern int tcp_sock;
    tcp_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (tcp_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "Socket created, connecting to server");
    DPRINTF("Socket created, connecting to %s:%d\n", host_ip, PORT);
    err = connect(tcp_sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "Successfully connected");
    return 0;
}

/**
  * @brief  设备连接服务器
  * @param  target_ip:目标IP地址
  * @retval 成功返回0，失败返回-1
  */
int device_connect_to_server(char *target_ip)
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    //#define PORT 5005
    DPRINTF("FUNCTION:%s\tLINE:%d\n", __FUNCTION__, __LINE__);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DPRINTF("\n Socket creation error \n");
        close(sock);
        return -1;
    }
    DPRINTF("FUNCTION:%s\tLINE:%d\n", __FUNCTION__, __LINE__);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, target_ip, &serv_addr.sin_addr)<=0) {
        DPRINTF("\nInvalid address/ Address not supported \n");
        close(sock);
        return -1;
    }
    DPRINTF("FUNCTION:%s\tLINE:%d\n", __FUNCTION__, __LINE__);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        //send(sock, "hello", strlen("hello"), 0);
        DPRINTF("\nConnection Failed %s\t%d\n", target_ip, PORT);
        close(sock);
        return -1;
    }
    DPRINTF("Connected to %s:%d\n", target_ip, PORT);
    //char str[] = "hello server!";
    //send(sock, str, strlen(str)+1, 0);
    close(sock);

    return 0;
}

/**
  * @brief  TCP扫描同一网段下的目标服务器设备
  * @param  target_ip:目标IP地址
  * @retval 成功返回0， 失败返回-1
  */
int device_scan_dest_server(char ip_addr[])
{
    char ip_local_addr[128]  = {0};
    char ip_search_addr[256] = {0};
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info); 
    memcpy(ip_local_addr, ip4addr_ntoa((ip4_addr_t *)&ip_info.ip), strlen(ip4addr_ntoa((ip4_addr_t *)&ip_info.ip)));
    
    //char *ip4addr_ntoa(const ip4_addr_t *addr);
    printf("device ip:%s\n", ip_local_addr);
    //192.168.1.8
    int i, self_ip_com = 0;
    int tok_cnt = 1;
    int ret = -1;
    for(i = 0; i < 128; i++)
    {
        if(ip_local_addr[i] == '.')//获取所在网段
        {
            if(tok_cnt == 3)
            {
                self_ip_com =  ip_local_addr[i+1] - '0';
                break;
            }
            tok_cnt++;
        }
        //DPRINTF("%c %d\n", ip_local_addr[i], i);
    }
    DPRINTF("self_ip_com:%d\n", self_ip_com);
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
    char ip_addr_temp[128] = {0};
    strncpy(ip_addr_temp, ip_local_addr, i+1);
    DPRINTF("%s\n", ip_addr_temp);
    int com;
    for(com = 1; com < 255; com++)
    {
        if(com == self_ip_com)
            continue;
        sprintf(ip_search_addr, "%s%d", ip_addr_temp, com);
        //DPRINTF("%s\n", ip_search_addr);
        DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
        device_ping_ip_addr(ip_search_addr);//ping传入的ip地址
        vTaskDelay(500 / portTICK_PERIOD_MS);
        //ping通ip地址后，继续判断是否能够连接端口(5005)
        if(ping_flag == PING_IP_OK)
        {
            esp_ping_stop(ping);
            esp_ping_delete_session(ping);
            DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
            ret = device_connect_to_server(ip_search_addr);
            if (ret == 0)
            {
                DPRINTF("Found the target server.\n");
                memcpy(ip_addr, ip_search_addr, strlen(ip_search_addr));
                break;
            }
        }else{
            DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
            //vTaskDelay(300 / portTICK_PERIOD_MS);
            esp_ping_stop(ping);
            esp_ping_delete_session(ping);
        }
        DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
        //DPRINTF("try again.\n");
        memset(ip_search_addr, 0, sizeof(ip_search_addr));
    }
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
    printf("com = %d\n", com);
    if(com < 255)
    {
        DPRINTF("Device scan server success.\n");
        
        return 0;
    }else{
        DPRINTF("Device scan server failed.\n");
        return -1;
    }
}

/**
  * @brief  ping ip地址成功的回调函数
  * @param  handle:ping ip句柄
  * @param  args:传入给回调函数的参数
  * @retval None
  */
void ping_ip_success(esp_ping_handle_t handle, void *args)
{
    // optionally, get callback arguments
    // const char* str = (const char*) args;
    // DPRINTF("%s\r\n", str); // "foo"
    ping_flag = PING_IP_OK;
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(handle, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(handle, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(handle, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    printf("%ld bytes from %s icmp_seq=%d ttl=%d time=%ld ms\n",
           recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
}

/**
  * @brief  ping ip地址超时的回调函数
  * @param  handle:ping ip句柄
  * @param  args:传入给回调函数的参数
  * @retval None
  */
void ping_ip_timeout(esp_ping_handle_t handle, void *args)
{
    ping_flag = PING_IP_ERR;
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(handle, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    printf("From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), seqno);
}

/**
  * @brief  ping ip地址结束的回调函数
  * @param  handle:ping ip句柄
  * @param  args:传入给回调函数的参数
  * @retval None
  */
void ping_ip_end(esp_ping_handle_t handle, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    ping_flag = -1;
    esp_ping_get_profile(handle, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(handle, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(handle, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    DPRINTF("%ld packets transmitted, %ld received, time %ldms\n", transmitted, received, total_time_ms);
}

/**
  * @brief  ping ip地址
  * @param  ip_str:字符串格式的IP地址   "192.168.1.8"
  * @retval None
  */
void device_ping_ip_addr(char *ip_str)
{
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
    ip_addr_t target_addr;
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.interval_ms = 400;
    ping_config.timeout_ms  = 400;
    memset(&target_addr, 0, sizeof(ip_addr_t));
    //字符串IP地址转换为ping_addr
    if (ipaddr_aton(ip_str, &target_addr)) {
        //DPRINTF("转换成功.\n");
        ping_config.target_addr = target_addr;
    } else {
        DPRINTF("Invalid IP address\n");
        return ;
    }
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
    //ping_config.target_addr = target_addr;          // target IP address
    ping_config.count = ESP_PING_COUNT_INFINITE;    // ping in infinite mode, esp_ping_stop can stop it
    
    //设置回调函数
    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = ping_ip_success;
    cbs.on_ping_timeout = ping_ip_timeout;
    cbs.on_ping_end = ping_ip_end;
    cbs.cb_args = "ping ip";  // arguments that will feed to all callback functions, can be NULL
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
    esp_ping_new_session(&ping_config, &cbs, &ping);
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
    esp_ping_start(ping);
    DPRINTF("%s\t%d\n", __FUNCTION__,__LINE__);
}
