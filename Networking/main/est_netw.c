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
#define EXAMPLE_ESP_WIFI_SSID      "Jesper"
#define EXAMPLE_ESP_WIFI_PASS      "12345677"

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
    uint64_t tag_number;
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
   // Variables to store the divided parts
    char token[7]; // Assuming a maximum token length of 6
    char mac_address[MAC_ADDRESS_SIZE];
    char finger[255];

    // Use sscanf to parse the string
    if (sscanf(buffer, "%[^-]-%[^-]-%[^-]", token, mac_address, finger) == 3) {
        // Print the results
        ESP_LOGI(TAG, "First String: %s\n", token);
        ESP_LOGI(TAG, "Second String: %s\n", mac_address);
        ESP_LOGI(TAG, "Last string: %s\n ", finger);
    } else {
        // Parsing failed
        printf("Error parsing the string.\n");
    }

    // Check if the number is in the array
    for (int i = 0; i < MAX_AUTH_PEOPLE; i++) {
        if (strcmp(finger_list[i].finger, finger) == 0) {
            ESP_LOGI(TAG, "FINGER OK, checking MAC");
            ESP_LOGI(TAG, "mac from list: %s | mac from message %s ", finger_list[i].mac_address, mac_address);
            return strcmp(finger_list[i].mac_address, mac_address) == 0;  // COMPARE MAC ADDRESS
        }
    }
    
    return false;
}

bool check_rfid(char* buffer) {
    // Variables to store the divided parts
    char token[6]; // Assuming a maximum token length of 6
    char mac_address[MAC_ADDRESS_SIZE];
    uint64_t tag;

    // Use sscanf to parse the string
    if (sscanf(buffer, "%[^-]-%[^-]-%" SCNu64, token, mac_address, &tag) == 3) {
        // Print the results
        ESP_LOGI(TAG, "First String: %s\n", token);
        ESP_LOGI(TAG, "Second String: %s\n", mac_address);
        ESP_LOGI(TAG, "Number: %" PRIu64 "\n", tag);
    } else {
        // Parsing failed
        printf("Error parsing the string.\n");
    }

    // Check if the number is in the array
    for (int i = 0; i < MAX_AUTH_PEOPLE; i++) {
        if (rfid_list[i].tag_number == tag) {
            ESP_LOGI(TAG, "TAG OK, checking MAC");
            ESP_LOGI(TAG, "mac from list: %s | mac from message %s ", rfid_list[i].mac_address, mac_address);
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
    TickType_t rfid_tick = 0;
    TickType_t finger_tick = 0;

    while (1) {
        struct sockaddr_in sourceAddr;
        socklen_t addrLen = sizeof(sourceAddr);
        int bytesRead = recvfrom(receiverSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&sourceAddr, &addrLen);


        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            char addrStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sourceAddr.sin_addr, addrStr, sizeof(addrStr));
            ESP_LOGI(TAG, "Received syslog message: %s from %s", buffer, addrStr);

            // Here we can parse the message and utilize it as needed for the lock system
            if (strstr(buffer, "rfid") != NULL) {
                // Handle RFID VALIDATION
                if(check_rfid(buffer)) {
                    // Get Tick
                    rfid_tick = xTaskGetTickCount();
                    ESP_LOGI(TAG, "RFID VALIDATION success.");
                } else {
                    rfid_tick = 0;
                    ESP_LOGI(TAG, "RFID VALIDATION failed.");
                }
            } else if (strstr(buffer, "finger") != NULL) {
                // Handle FINGERPRINT VALIDATION
                if(check_finger_print(buffer)) {
                    finger_tick = xTaskGetTickCount();
                    ESP_LOGI(TAG, "FINGERPRINT VALIDATION sucess.");
                } else {
                    finger_tick = 0;
                    ESP_LOGI(TAG, "FINGERPRINT VALIDATION failed");
                }
            }
            // if diff between finger message and rfid message < 3000 ticks
            if( rfid_tick != 0 && finger_tick != 0) {
                ESP_LOGI(TAG, "TICK NON ZERO");
                if (abs((int) rfid_tick - (int) finger_tick) < pdMS_TO_TICKS(3000) ) {
                // Authentication SUCCESSFUL 
                // do routine to open the SOLENOID / Light up the LED
                ESP_LOGI(TAG, "Unlocking");
                onSuccess();
                } else {
                    ESP_LOGI(TAG, "More than 3 seconds between authentications. Still Locked");
                }
            } else {
                ESP_LOGI(TAG, "Missing one authentication factor. Still locked");
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

    mac_tag tag = {};
    strcpy(tag.mac_address, "70:B8:F6:12:BD:64");
    tag.tag_number = 585971602005;
    rfid_list[0] = tag;

    mac_finger finger = {};
    strcpy(finger.mac_address, "70:B8:F6:12:BD:64");
    strcpy(finger.finger, "finger");
    finger_list[0] = finger;


    // Sending syslog message to internal host
    //sendSyslogMessage("Greetings from ESP32!"); // send RFID or fingerprint data instead here

    // Receiving syslog messages from external host
    receiveSyslogMessages();
}
