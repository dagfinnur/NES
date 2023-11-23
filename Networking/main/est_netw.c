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
#include "driver/gpio.h"
#include "lwip/inet.h"

#include "nvs.h"
#include "nvs_flash.h"

// Define SSID and PSK
#define EXAMPLE_ESP_WIFI_SSID      "Diogo"
#define EXAMPLE_ESP_WIFI_PASS      "abcdefgh"

#define IP "192.168.1.246"

// Define the RFID & fingerprint system to utilize each message independently
//#define ESP_RFID_HOST "SOMEVALIDATION OF BEING A RFID"
//#define ESP_FINGERPRINT_HOST "SOMEVALIDATION OF BEING A FINGERPRINT" 

#define PORT 514

// Additional includes for syslog
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static const char *TAG = "wifi";

#define FINGER_SIZE 100
#define MAC_ADDRESS_SIZE 18
#define MAX_AUTH_PEOPLE 10
#define RELAY_GPIO_PIN 2

typedef struct mac_tag {
    char mac_address[MAC_ADDRESS_SIZE];
    int tag_number;
}mac_tag;

typedef struct mac_finger {
    char mac_address[MAC_ADDRESS_SIZE];
    char finger[FINGER_SIZE];
}mac_finger;

static mac_finger finger_list[MAX_AUTH_PEOPLE] = {};
static mac_tag rfid_list[MAX_AUTH_PEOPLE] = {};

// Maybe change this for the solenoid
void onSuccess() {
    gpio_set_direction(RELAY_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(RELAY_GPIO_PIN, 0);
}

bool check_finger_print(char* buffer) {
    // Convert the string argument to integer
    strtok(buffer, "-");
    char* mac_address = strtok(NULL, "-");
    char* finger = strtok(NULL, "-");

    // Check if the number is in the array
    for (int i = 0; i < MAX_AUTH_PEOPLE; i++) {
        if (strcmp(finger_list[i].finger, finger) == 0) {
            return strcmp(finger_list[i].mac_address, mac_address) == 0;  // COMPARE MAC ADDRESS
        }
    }

    return false;  // Number is not in the array
}

bool check_rfid(char* buffer) {
    // Convert the string argument to integer
    
    char* token = strtok(buffer, "-");
    char* mac_address = strtok(NULL, "-");
    int tag = atoi(token);

    // Check if the number is in the array
    for (int i = 0; i < MAX_AUTH_PEOPLE; i++) {
        if ((int) rfid_list[i].tag_number == tag) {
            return strcmp(rfid_list[i].mac_address, mac_address) == 0;  // COMPARE MAC ADDRESS
        }
    }

    return false;  // Number is not in the array
}

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
    destAddr.sin_port = htons(PORT); // Specify the communication port for sending syslog messages
    destAddr.sin_addr.s_addr = inet_addr(IP); // CHANGE THIS TO THE IP OF RECEIVER HOST

    int result = sendto(senderSocket, message, strlen(message), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
    if (result == -1) {
        ESP_LOGE(TAG, "Syslog message sending failed");
    } else {
        ESP_LOGI(TAG, "Syslog message sent: %s", message);
    }

    close(senderSocket);
}


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
    //char buffer[65535]; // Maximum buffer size for UDP packets

    while (1) {
        struct sockaddr_in sourceAddr;
        socklen_t addrLen = sizeof(sourceAddr);
        int bytesRead = recvfrom(receiverSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&sourceAddr, &addrLen);

        TickType_t rfid_tick = 0;
        TickType_t finger_tick = 0;

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            char addrStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sourceAddr.sin_addr, addrStr, sizeof(addrStr));
            ESP_LOGI(TAG, "Received syslog message: %s from %s", buffer, addrStr);

            // Here we can parse the message and utilize it as needed for the lock system
            if (strstr(buffer, "rfid") != NULL) {
                if(check_rfid(buffer)) {
                    // Get Tick
                    rfid_tick = xTaskGetTickCount();
                }
                // Handle RFID VALIDATION
                ESP_LOGI(TAG, "RFID VALIDATION received. Waiting for FINGERPRINT VALIDATION.");
            } else if (strstr(buffer, "finger") != NULL) {
                if(check_finger_print(buffer)) {
                    finger_tick = xTaskGetTickCount();
                }
                // Handle FINGERPRINT VALIDATION
                ESP_LOGI(TAG, "FINGERPRINT VALIDATION received. Opening lock.");
            }
            // if diff between finger message and rfid message < 3000 ticks
            if( rfid_tick != 0 && finger_tick != 0 && (abs((int) rfid_tick - (int) finger_tick) < pdMS_TO_TICKS(3000) )) {
                // Authentication SUCCESSFUL 
                // do routine to open the SOLENOID / Light up the LED
                onSuccess();
            }

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

// Connect to the network and start listening for syslog messages
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    connectToANetwork();


    // Sending syslog message to internal host
    //sendSyslogMessage("Greetings from ESP32!"); // send RFID or fingerprint data instead here

    // Receiving syslog messages from external host
    receiveSyslogMessages();
}
