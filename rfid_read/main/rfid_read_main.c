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

#include "nvs.h"
#include "nvs_flash.h"

#include "tags.h"
#include "network.h"

static const char* TAG1 = "rc522-demo";
static rc522_handle_t scanner;
static int mode = 0; //0 for read; 1 for write
uint64_t master_tag = 903303070856; //basecamp tag
/*uint64_t authorized_tags[MAX_TAGS] = 813411536073;*/ //blue rfid original tag



static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    char message[64];
    char key[16];
    char mac_address[MAC_ADDRESS_SIZE];
    get_mac_address(mac_address);
    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
            rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
            ESP_LOGI(TAG1, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
            sprintf(key, "%lld", tag->serial_number);
            if (mode == 1) { 
                if (isAuthorized(key)) {
                    removeTag(key);
                }
                else {
                    addTag(key, tag->serial_number);
                }
                mode = 0;
            }
            else if (tag->serial_number == master_tag) {
                printf("MASTER TAG.\n");
                printf("Please insert the tag you want to add/remove.\n");
                mode = 1;
            }
            else if (isAuthorized(key)) {
                printf("AUTHORIZED READ\n");
                sprintf(message, "rfid-%s-%lld", mac_address, tag->serial_number);
                printf("message to send: %s\n", message);
                sendSyslogMessage(message);
            }
            else {
                printf("UNAUTHORIZED READ\n");
            }         
        }
        //print_nvs_entries(); NOT WORKING ATM
        break;
    }
}

void rfid_read(void) {
    //RFID SETUP
    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = 35,
        .spi.mosi_gpio = 14,
        .spi.sck_gpio = 33,
        .spi.sda_gpio = 32,
    };
    
    //RC522 FRAMEWORK EVENT LOGIC
    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
}

void app_main() {    
    init_nvs();
    connectToANetwork();
    rfid_read();
}