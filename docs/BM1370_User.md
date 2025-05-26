# BM1370 Driver Usage

The BM1370 driver is compiled optionally.  Configure cgminer with the following
flag:

```sh
./configure --enable-bm1370
```

Once built, run cgminer as normal.  By default the driver tries to open
`/dev/ttyUSB0`. You can override this with the `--bm1370-dev` option.
If the device is present you will see a log message
similar to:

```
BM1370: detected at /dev/ttyUSB0
```

At this stage only basic initialisation commands are sent to the ASIC.  Work can
be dispatched but nonce reporting has not yet been implemented.  The driver is
therefore suitable for development and experimentation only.

Refer to `driver-bm1370.c` and `driver-btm-soc.c` for protocol details and
further extension ideas.
