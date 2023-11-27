#include <stdio.h>
#include "fingerprint.c"
#include "string.h"
#include "driver/gpio.h"
#include "network.h"

void app_main(void)
{
    char message[64];

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
        esp_light_sleep_start();

        // Try enroll 1
        bool f = CaptureFingerFast();
        if (f)
        {
            Enroll(i);
            LedOff();
            esp_light_sleep_start();
            IsFingerPressed();
        }

    }
    LedOff();

    c = GetEnrolledCount();
    */

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
                    GetTemplate(id);  

                    // Now that the template is loaded, get the SHA256 of it
                    uint8_t hash = get_sha256_of_template(id);
                    sprintf(message, "finger-%s-%d", mac_address, hash);
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
