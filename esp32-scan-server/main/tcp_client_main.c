/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "network.h"


/**
  * @brief  扫描WiFi信号任务
  * @param  param:线程任务传参
  * @retval None
  */
void wifi_scan_task(void *param)
{
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };
    //通常认为-70dbm以上为理想的信号强度，-70 dbm～-80 dbm为中等信号强度，
    //小于-80dbm为弱信号，多数企业级AP可以接收到-95dbm左右的信号。
    while (1)
    {
        esp_wifi_scan_start(&scan_config, true);
        uint16_t ap_num = 0;
        esp_wifi_scan_get_ap_num(&ap_num);
        wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_num);
        esp_wifi_scan_get_ap_records(&ap_num, ap_list);
        printf("ap_num:%d\n", ap_num);
        if(ap_num == 0)
        {
            printf("扫描失败!\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        printf("扫描成功:\n");
        for (int i = 0; i < ap_num; i++) 
        {
            printf("WiFi ID: %s\t信号强度: %d\n", (char *)ap_list[i].ssid, ap_list[i].rssi);
        }
        
        free(ap_list);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    
    esp_wifi_stop();
    esp_wifi_deinit();
}



#if 0   //测试循环扫描WiFi信号
int app_main()
{
    printf("Program starting...\n");
    xTaskCreate(wifi_scan_task, "wifi_scan_task", 10240, NULL, 5, NULL);
    
    while (1)
    {
        vTaskDelay(1000);
    }

    return 0;
}

#endif

//设备启动接入WiFi后，循环扫描服务器ip是否在线，通信端口是否打开
#if 1
int app_main()
{
    printf("Program starting...\n");

    int ret = 0;
    char server_ip[128] = {0};
    uint8_t wifi_ssid[] = "imi-12-1719";
    uint8_t wifi_passwd[] = "imi?61415926";
    
    device_connect_wifi_ap(wifi_ssid, wifi_passwd);
    vTaskDelay(500);
#if 1
    //
    while(1)
    {
        ret = device_scan_dest_server(server_ip);
        if(ret == 0)
        {
            printf("Server ip:%s\n", server_ip);
            break;
        }
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
    
    //连接服务器
    extern int tcp_sock;
    tcp_client_init(server_ip);
    char str[] = "hello server!";
    send(tcp_sock, str, strlen(str)+1, 0);

#endif

    return 0;
}
#endif

