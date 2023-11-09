#include <esp_log.h>
#include <inttypes.h>
#include "rc522.h"
#include <unistd.h>

#include "network.h"
#include <string.h>

static const char* TAG1 = "rc522-demo";
static rc522_handle_t scanner;
static int mode = 0; //0 for read; 1 for write
uint64_t master_tag = 903303070856; //basecamp tag
uint64_t authorized_tag = 813411536073; //blue rfid original tag
char* myip = "hello";


void wifi(void) {
    /*
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();*/

    ///////////////////////////////////////////////////////////////////////////////////////////////////

     // Initialize NVS
    

    // Sending syslog message to internal host
    //sendSyslogMessage("Greetings from ESP32!"); // send RFID or fingerprint data instead here

    // Receiving syslog messages from external host
    receiveSyslogMessages();
}

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    char* message = "";
    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG1, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                if (mode == 1) { 
                    authorized_tag = tag->serial_number;
                    mode = 0;
                }
                else if (tag->serial_number == master_tag) {
                    printf("MASTER TAG.\n");
                    printf("Please insert the new authorized key.\n");
                    mode = 1;
                }
                else if (tag->serial_number == authorized_tag) {
                    printf("AUTHORIZED READ\n");
                    //wifi()
                    message = sprintf("Greetings from the RFID-BOARD!\n IP: %s\n  Tag Read: %d\n", myip, tag->serial_number);
                    sendSyslogMessage(message);
                }
                else {
                    printf("UNAUTHORIZED READ\n");
                }
        }
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
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    connectToANetwork();
    /*
    esp_netif_t *netif = esp_netif_create_default_wifi_sta(); // Create a default WiFi station network interface
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
    {
        myip = &ip_info.ip;
    } else {
        ESP_LOGE("APP", "Failed to get IP info");
    }*/
    rfid_read();
}