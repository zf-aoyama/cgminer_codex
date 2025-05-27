#ifndef DRIVER_BM1370_H
#define DRIVER_BM1370_H

#include "miner.h"
#include "bf16-uartdevice.h"

#define BM1370_DEFAULT_DEV "/dev/ttyUSB0"

struct bm1370_info {
    device_t uart;
    int frequency;
    int asic_count;
};

extern struct device_drv bm1370_drv;

#endif // DRIVER_BM1370_H
