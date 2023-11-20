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

#include "nvs_flash.h"
#include "nvs.h"

#define MAX_TAGS 100
#define TAG_NAMESPACE "authorized_tags"

// Initialize the NVS
esp_err_t init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

// Print all tags
void print_nvs_entries() {
    printf("PRINTING NVS namespace!\n");
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(TAG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS namespace!\n");
        return;
    }

// Example of listing all the key-value pairs of any type under specified partition and namespace
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(TAG_NAMESPACE, NVS_READWRITE, NVS_TYPE_ANY, &it);
    while(res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
        printf("key '%s', type '%d' \n", info.key, info.type);
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
}


// Check if a tag is already registered
bool isAuthorized(const char* tag) {
    printf("Checking tag: %s\n", tag);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(TAG_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        return false;
    }

    uint64_t value;
    err = nvs_get_u64(my_handle, tag, &value);
    nvs_close(my_handle);
    err==ESP_OK ? printf("Tag found\n") : printf("Tag not found\n") ;
    return (err == ESP_OK);
}

// Add a new tag
esp_err_t addTag(const char* tag, u_int64_t value) {
    printf("Add a new tag: %s with value %lld\n", tag, value);
    if (isAuthorized(tag)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(TAG_NAMESPACE, NVS_READWRITE, &my_handle);
    printf("Opening NVS\n");
    if (err == ESP_OK) {
        err = nvs_set_u64(my_handle, tag, value);
        printf("Setting an u64\n");
        if (err == ESP_OK) {
            err = nvs_commit(my_handle);
            printf("Commiting to NVS\n");
        }
        printf("Closing NVS\n");
        nvs_close(my_handle);
    }

    return err;
}

// Remove a tag
esp_err_t removeTag(const char* tag) {
    printf("Removing tag: %s\n", tag);
    if (!isAuthorized(tag)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(TAG_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_erase_key(my_handle, tag);
        if (err == ESP_OK) {
            err = nvs_commit(my_handle);
        }
        nvs_close(my_handle);
    }

    return err;
}
