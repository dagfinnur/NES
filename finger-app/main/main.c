#include <stdio.h>
#include "fingerprint.c"
#include "string.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#define SLEEP_IN_US 3000000 // 3,000,000 microseconds = 3 seconds

void app_main(void)
{
    // Grace wakeup
    printf("Sleeping for %d seconds\n", SLEEP_IN_US / (1000 * 1000));
    fflush(stdout);
    esp_sleep_enable_timer_wakeup(SLEEP_IN_US);
    esp_light_sleep_start();

    int rx_pin = GPIO_NUM_15;
    int tx_pin = GPIO_NUM_14;
    init_uart();

    InitFingerprint();

    // The finger module starts off at 9600 when powered on
    configure_uart(9600, rx_pin, tx_pin);

    Open();

    // ChangeBaudrate(115200);
    // configure_uart(115200, rx_pin, tx_pin);

    // // Re-initialize module for good meassure
    // Open();
    GetEnrolledCount();

    LedOn();
    // GetRawImage();
    GetTemplate(1);
    LedOff();


//    GetImage();

    ExitFingerprint();
}
