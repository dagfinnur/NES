#include <stdio.h>
#include "fingerprint.c"
#include "string.h"
#include "driver/gpio.h"
#include "network.h"

#define SLEEP_IN_US 1000000 // 3,000,000 microseconds = 3 seconds

void app_main(void)
{

    connectToANetwork();
    //esp_sleep_enable_timer_wakeup(SLEEP_IN_US);

    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

    while (!connected)
    {
        printf("connecting to WiFi...\n");
        fflush(stdout);
        vTaskDelay( xDelay );
    }
    // Grace wakeup
    printf("Sleeping for %d seconds\n", SLEEP_IN_US / (1000 * 1000));
    fflush(stdout);
    // esp_sleep_enable_timer_wakeup(SLEEP_IN_US);
    // esp_light_sleep_start();

    int rx_pin = GPIO_NUM_15;
    int tx_pin = GPIO_NUM_14;
    init_uart();

    InitFingerprint();

    // The finger module starts off at 9600 when powered on
    configure_uart(9600, rx_pin, tx_pin);

    Open();

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

    LedOn();

    bool exit = false;
    bool auth = false;
    while (!exit)
    {
        // esp_light_sleep_start();
        bool f = CaptureFingerFast();
        if (f)
        {
            auth = Identification();
            if (auth)
            {
                // send msg
                sendSyslogMessage("finger print hello");
            }
        }
    }

    LedOff();

    ExitFingerprint();
}
