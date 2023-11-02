#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#include "finger.h"

static const struct device *gpio_ct_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));


int main()
{
    printf("loading library const %d", myconst);
    return 0;
}
