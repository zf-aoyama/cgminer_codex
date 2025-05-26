# BM1370 Driver Design

This document outlines the high level design of the BM1370 driver included in
`cgminer`.  The implementation is heavily inspired by the `driver-btm-soc.c`
code and the reference implementation found in `reference/ESP-Miner`.

## Communication model

The BM1370 ASICs are connected over a UART interface.  Each packet begins with a
`0x55AA` preamble followed by a command byte and payload.  Command packets use a
CRC5 checksum while job packets use a CRC16 checksum.  The driver configures the
UART at 115200 baud and writes raw packets directly to the device node.

Only a minimal subset of the command set is implemented:

* Set version mask
* Mark the chain as inactive

The structures and CRC routines are copied from the reference code so the packet
format matches Bitmain's documentation.

## Device detection

Detection opens the path provided via the `--bm1370-dev` option
(default `/dev/ttyUSB0`). If successful, a new `cgpu_info` instance is created
and added to cgminer.  This keeps the driver self contained and avoids complex
USB enumeration code.

## Limitations

This driver is intentionally lightweight.  It does not yet implement frequency
configuration or nonce polling.  Error handling is minimal but UART setup and
packet transmission failures are reported.  It serves as a starting point for
further development when real hardware is
available.
