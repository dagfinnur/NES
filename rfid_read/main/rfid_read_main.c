#include "tags.h"
#include "network.h"

static const char* TAG1 = "rc522-demo";
static rc522_handle_t scanner;
static int mode = 0; //0 for read; 1 for write
uint64_t master_tag = 903303070856; //basecamp tag
uint64_t authorized_tags[MAX_AUTH_TAGS] /*= 813411536073;*/ //blue rfid original tag

int isAuthorized(uint64_t tag) {
    //include logic for checking if tag is already on the list
    int size = sizeof(authorized_tags) / sizeof(authorized_tags[0]);  // Size of your array

    for (int i = 0; i < size; ++i) {
        if (authorized_tags[i] == number) {
            return 1;
        }
    }
    return 0;
}

void addTag() {
    //include logic to add tag to the current list in the NVS
    printf("Added tag.\n");
}

void removeTag() {
    //include logic to remove tag to the current list in the NVS
    printf("Removed tag.\n");
    ;
}

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    char message[64];
    char ip[16];
    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG1, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                if (mode == 1) { 
                    if (isAuthorized(tag->serial_number)) {
                        addTag();
                    }
                    else {
                        removeTag();
                    }
                    mode = 0;
                }
                else if (tag->serial_number == master_tag) {
                    printf("MASTER TAG.\n");
                    printf("Please insert the tag you want to add/remove.\n");
                    mode = 1;
                }
                else if (tag->serial_number == authorized_tag) {
                    printf("AUTHORIZED READ\n");
                    getip(&ip);
                    sprintf(message, "%lld\n", tag->serial_number);
                    printf("message to send: %s\n", message);
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

void nvs_init {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    
}

void app_main() {    
    nvs_init();
    connectToANetwork();
    rfid_read();
}