#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdio.h>

static const struct device *gpio_ct_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

typedef struct DataPacket {
    
};

int main()
{
    int x = 2;
    int y = 3;
    int z = 4;
    int num = 20;

    int result = 0;

    result = (x * y) + (y * z) + (z * x) + (x * y * z);
    
    
    printf("result: %d", result);
    //, CONFIG_BOARD);
    
    return 0;
}
