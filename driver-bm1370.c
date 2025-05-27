#include "config.h"
#include "miner.h"
#include "logging.h"
#include "driver-bm1370.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

extern char *opt_bm1370_dev;

/*
 * Minimal BM1370 driver built from the ESP-Miner reference implementation.
 * This driver communicates with a BM1370 ASIC chain via a UART interface.
 * It only implements a very small subset of the protocol sufficient for
 * experimentation and testing.  The implementation is deliberately simple
 * and is not intended for production use.
 */

/* ---------------------------------------------------------------------- */
/* UART helpers                                                           */
/* ---------------------------------------------------------------------- */

static bool bm1370_uart_open(struct bm1370_info *info, const char *dev)
{
    struct termios tio;

    info->uart.fd = -1;
    info->uart.fd = open(dev, O_RDWR | O_NOCTTY | O_SYNC);
    if (info->uart.fd < 0) {
        applog(LOG_ERR, "BM1370: failed to open %s: %s", dev, strerror(errno));
        return false;
    }

    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    /* Non-blocking reads with 100ms timeout to avoid hanging when
     * receive logic is added in future. */
    tio.c_cc[VTIME] = 1; /* 100ms */
    tio.c_cc[VMIN]  = 0;

    if (tcflush(info->uart.fd, TCIOFLUSH) < 0) {
        applog(LOG_ERR, "BM1370: tcflush failed: %s", strerror(errno));
        close(info->uart.fd);
        return false;
    }
    if (tcsetattr(info->uart.fd, TCSANOW, &tio) < 0) {
        applog(LOG_ERR, "BM1370: tcsetattr failed: %s", strerror(errno));
        close(info->uart.fd);
        return false;
    }

    info->uart.device = strdup(dev);
    return true;
}

static void bm1370_uart_close(struct bm1370_info *info)
{
    if (info->uart.fd >= 0)
        close(info->uart.fd);
    info->uart.fd = -1;
    free(info->uart.device);
}

/* ---------------------------------------------------------------------- */
/* CRC helpers copied from reference/ESP-Miner                            */
/* ---------------------------------------------------------------------- */

static uint8_t crc5(uint8_t *data, uint8_t len)
{
    uint8_t i, j, k, index = 0;
    uint8_t crc = 0x1F;
    uint8_t crcin[5] = {1,1,1,1,1};
    uint8_t crcout[5] = {1,1,1,1,1};
    uint8_t din = 0;

    len *= 8;
    for (j = 0x80, k = 0, i = 0; i < len; i++) {
        din = (data[index] & j) != 0;
        crcout[0] = crcin[4] ^ din;
        crcout[1] = crcin[0];
        crcout[2] = crcin[1] ^ crcin[4] ^ din;
        crcout[3] = crcin[2];
        crcout[4] = crcin[3];
        j >>= 1; k++;
        if (k == 8) { j = 0x80; k = 0; index++; }
        memcpy(crcin, crcout, 5);
    }
    crc = 0;
    if (crcin[4]) crc |= 0x10;
    if (crcin[3]) crc |= 0x08;
    if (crcin[2]) crc |= 0x04;
    if (crcin[1]) crc |= 0x02;
    if (crcin[0]) crc |= 0x01;
    return crc;
}

static const uint16_t crc16_table[256] = {
#include "crc16.c"
};

static uint16_t crc16_false(uint8_t *buffer, uint16_t len)
{
    uint16_t crc = 0xffff;
    while (len-- > 0)
        crc = crc16_table[((crc >> 8) ^ (*buffer++)) & 0xFF] ^ (crc << 8);
    return crc;
}

/* ---------------------------------------------------------------------- */
/* Protocol helpers                                                       */
/* ---------------------------------------------------------------------- */

#define TYPE_JOB       0x20
#define TYPE_CMD       0x40
#define GROUP_SINGLE   0x00
#define GROUP_ALL      0x10
#define CMD_WRITE      0x01
#define CMD_READ       0x02
#define CMD_SETADDRESS 0x00
#define CMD_INACTIVE   0x03

static const uint8_t cmd_version_mask[] = { 0x00, 0xA4, 0x90, 0x00, 0x00, 0x00 };
static const uint8_t cmd_chain_inactive[] = { 0x00, 0x00 };

static int bm1370_send_packet(struct bm1370_info *info, uint8_t header,
                              const uint8_t *data, uint8_t data_len)
{
    bool job = header & TYPE_JOB;
    uint8_t total = job ? data_len + 6 : data_len + 5;
    uint8_t buf[64];

    buf[0] = 0x55;
    buf[1] = 0xAA;
    buf[2] = header;
    buf[3] = job ? (data_len + 4) : (data_len + 3);
    memcpy(buf + 4, data, data_len);

    if (job) {
        uint16_t crc = crc16_false(buf + 2, data_len + 2);
        buf[4 + data_len] = (crc >> 8) & 0xFF;
        buf[5 + data_len] = crc & 0xFF;
    } else {
        buf[4 + data_len] = crc5(buf + 2, data_len + 2);
    }

    if (write(info->uart.fd, buf, total) != total) {
        applog(LOG_ERR, "BM1370: failed to send packet: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/* basic init based on reference implementation */
static bool bm1370_hw_init(struct bm1370_info *info)
{
    /* set version mask to 0 (default) */
    if (bm1370_send_packet(info, TYPE_CMD | GROUP_ALL | CMD_WRITE,
                           cmd_version_mask, sizeof(cmd_version_mask)) < 0)
        return false;

    /* put chain inactive */
    if (bm1370_send_packet(info, TYPE_CMD | GROUP_ALL | CMD_INACTIVE,
                           cmd_chain_inactive, sizeof(cmd_chain_inactive)) < 0)
        return false;

    return true;
}

/* ---------------------------------------------------------------------- */
/* cgminer driver hooks                                                   */
/* ---------------------------------------------------------------------- */

static struct cgpu_info *bm1370_detect_one(const char *path)
{
    struct cgpu_info *cgpu = usb_alloc_cgpu(&bm1370_drv, 1);
    struct bm1370_info *info;

    if (!cgpu)
        return NULL;

    info = cgcalloc(1, sizeof(*info));
    cgpu->device_data = info;
    cgpu->device_path = strdup(path);

    if (!bm1370_uart_open(info, path))
        goto fail;

    if (!bm1370_hw_init(info))
        goto fail;

    if (!add_cgpu(cgpu))
        goto fail;

    applog(LOG_INFO, "BM1370: detected at %s", path);
    return cgpu;

fail:
    bm1370_uart_close(info);
    free(info);
    cgpu = usb_free_cgpu(cgpu);
    return NULL;
}

static void bm1370_detect(__maybe_unused bool hotplug)
{
    bm1370_detect_one(opt_bm1370_dev);
}

struct device_drv bm1370_drv = {
    .drv_id    = DRIVER_bm1370,
    .dname     = "BM1370",
    .name      = "BM1370",
    .drv_detect = bm1370_detect,
    .hash_work  = hash_driver_work,
    .get_statline_before = blank_get_statline_before,
};

