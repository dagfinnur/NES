#include <stdio.h>
#include "fingerprint.c"
#include "string.h"
#include "driver/gpio.h"
#include "network.h"

void app_main(void)
{
    char message[100];

    char mac_address[MAC_ADDRESS_SIZE];
    get_mac_address(mac_address);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect to the access point
    connectToANetwork();

    // Define a wait delay of 500 milliseconds
    const TickType_t delay = 500 / portTICK_PERIOD_MS;

    while (!connected)
    {
        printf("connecting to WiFi...\n");
        fflush(stdout);
        vTaskDelay(delay);
    }
    
    // Grace startup
    printf("Sleeping for %ld milliseconds\n", delay*2);
    fflush(stdout);


    int rx_pin = GPIO_NUM_15;
    int tx_pin = GPIO_NUM_14;
    init_uart();

    InitFingerprint();

    // The finger module starts off at 9600 when powered on
    configure_uart(9600, rx_pin, tx_pin);

    Open();

    // Change the baudrate to allow for faster communication
    int new_baud = 57600;
    ChangeBaudrate(new_baud);
    configure_uart(new_baud, rx_pin, tx_pin);

    // Re-initialize module for good meassure
    Open();
    uint32_t c = GetEnrolledCount();

    /*
    * Enrollment logic below
    * Place finger when LED lights up
    * Lift finger when LED goes off
    * Repeat 3 times till finger print is stored
    
    EnrollStart(c + 1);

    for (int i = 1; i < 4; i++)
    {

        LedOn();
        vTaskDelay(delay);

        // Try enroll 1
        bool f = CaptureFingerFast();
        if (f)
        {
            Enroll(i);
            LedOff();
            vTaskDelay(delay);
            IsFingerPressed();
        }

    }
    LedOff();
    */

    c = GetEnrolledCount();

    bool exit = false;
    bool auth = false;
    uint8_t auth_id;

    int capture_attempts = 3;
    int attempt = 0;

    while (!exit)
    {
        attempt = 0;
        LedOn();

        // Brief wait before reading fingerprints
        vTaskDelay(delay / 10);

        // Keep reading fingerprints untill capture_attempts is reached
        while (attempt < capture_attempts)
        {

            // CaptureFingerSlow returns true if a fingerprint was read
            bool f = CaptureFingerSlow();
            if (f)
            {
                auth = Identification(&auth_id);
                if (auth)
                {
                    // Get the template of the matching the authenticated finger
                    GetTemplate(auth_id);  

                    // Now that the template is loaded, get the SHA256 of it
                    uint8_t sha256_len = 32;
                    uint8_t hash[sha256_len];
                    get_sha256_of_template(hash);


                    // Buffer for the SHA-256 hash in hexadecimal representation
                    char hash_hex[2 * sha256_len + 1]; // Each byte becomes two hex characters, plus null terminator
                    // Convert the binary hash to a hex string
                    for (int i = 0; i < sha256_len; ++i) {
                        sprintf(&hash_hex[2 * i], "%02x", hash[i]);
                    }

                    sprintf(message, "finger-%s-%s", mac_address, hash_hex);

                    // send msg
                    sendSyslogMessage(message);
                }
            }
            attempt++;
        }
        LedOff();
        vTaskDelay(delay);
    }

    LedOff();

    ExitFingerprint();
}
