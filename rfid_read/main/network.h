#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "lwip/inet.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "rc522.h"

// Define SSID and PSK
#define EXAMPLE_ESP_WIFI_SSID      "Jesper" /* "Diogo" "Basecamp Resident 2E" "Basecamp Guest" */
#define EXAMPLE_ESP_WIFI_PASS      "12345677"/* "abcdefgh"  "9SkinSaturdayNoon000" "AVeryGoodPass" */
#define IP                         "172.20.10.8"

// Define the RFID & fingerprint system to utilize each message independently
//#define ESP_RFID_HOST "SOMEVALIDATION OF BEING A RFID"
//#define ESP_FINGERPRINT_HOST "SOMEVALIDATION OF BEING A FINGERPRINT"

// Additional includes for syslog
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static const char *TAG = "wifi";

// Create a socket and send a syslog message to Host B
static void sendSyslogMessage(const char* message)
{
    int senderSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (senderSocket == -1) {
        ESP_LOGE(TAG, "Socket creation failed");
        return;
    }

    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(514); // Specify the communication port for sending syslog messages
    destAddr.sin_addr.s_addr = inet_addr(IP); // CHANGE THIS TO THE IP OF RECEIVER HOST

    int result = sendto(senderSocket, message, strlen(message), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
    if (result == -1) {
        ESP_LOGE(TAG, "Syslog message sending failed");
    } else {
        ESP_LOGI(TAG, "Syslog message sent: %s", message);
    }

    close(senderSocket);
}

/*
void wifi(void) {
    //sendSyslogMessage("Greetings from ESP32!"); // send RFID or fingerprint data instead here

    // Receiving syslog messages from external host
    receiveSyslogMessages();
}
*/

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
    
    }
}

// This function listens for incoming syslog messages on Host B
static void receiveSyslogMessages()
{
    int receiverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiverSocket == -1) {
        ESP_LOGE(TAG, "Socket creation failed");
        return;
    }

    struct sockaddr_in myAddr;
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(514); // Specify the communication port for receiving syslog messages
    myAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiverSocket, (struct sockaddr*)&myAddr, sizeof(myAddr)) == -1) {
        ESP_LOGE(TAG, "Bind failed");
        close(receiverSocket);
        return;
    }

    char buffer[1024]; // Adjust the buffer size as needed... not sure what the max size is, just picked 1024 for now

    while (1) {
        int bytesRead = recvfrom(receiverSocket, buffer, sizeof(buffer), 0, NULL, NULL);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            ESP_LOGI(TAG, "Received syslog message: %s", buffer);
            // Here we can parse the message and utilize it as needed for the lock system
            // sample: if buffer includes RFID VALIDATION then WAIT FOR FINGERPRINT VALIDATION
            // sample: if buffer includes FINGERPRINT VALIDATION then OPEN LOCK
            // AND VICE VERSA
        }
    }

    close(receiverSocket);
}

// connect to the network
static void connectToANetwork(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    esp_wifi_start();
}

char* getip(char* myip){
    // IP address.
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = netif_get_by_index(ESP_IF_WIFI_STA);
    esp_netif_get_ip_info(netif, &ip_info);
    printf("My IP: " IPSTR "\n", IP2STR(&ip_info.ip));
    sprintf(myip, IPSTR, IP2STR(&ip_info.ip));
    return myip;
}