#include "config.h"
#include "miner.h"
#include "logging.h"
#include "driver-bm1370.h"

/*
 * Stub driver for BM1370 ASIC based on ESP-Miner reference implementation.
 * This only implements minimal hooks required by cgminer and does not
 * support real hardware interaction. It serves as a placeholder for future
 * development.
 */

static void bm1370_detect(__maybe_unused bool hotplug)
{
    applog(LOG_INFO, "BM1370 driver stub loaded");
}

struct device_drv bm1370_drv = {
    .drv_id    = DRIVER_bm1370,
    .dname     = "BM1370",
    .name      = "BM1370",
    .drv_detect = bm1370_detect,
    .hash_work  = hash_driver_work,
    .get_statline_before = blank_get_statline_before,
};

