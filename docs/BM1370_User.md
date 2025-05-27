# BM1370 Driver Usage

The BM1370 driver is compiled optionally.  Configure cgminer with the following
flag:

```sh
./configure --enable-bm1370
```

Once built, run cgminer as normal.  The driver will attempt to open
`/dev/ttyUSB0` on startup unless overridden with `--bm1370-dev`. If the device is present you will see a log message
similar to:

```
BM1370: detected at /dev/ttyUSB0
```

At this stage only basic initialisation commands are sent to the ASIC.  Work can
be dispatched but nonce reporting has not yet been implemented.  The driver is
therefore suitable for development and experimentation only.

Use `--bm1370-dev=/path/to/device` to select a different UART node.
To extend the protocol refer to the source files `driver-bm1370.c` and
`driver-btm-soc.c` for guidance.
