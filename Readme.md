# Scaffolding Project for nRF52840 Based Projects on SDK v16

## Install (Mac OSX Only)

1. Run `make sdk`. This will download and extract a copy of the SDK to `_sdk`
1. Run `make toolchain`. This will download and extract a copy of the toolchain.
1. For a first time startup, you'll have to run `make gen_key`. This key is used for secure DFU.

## Other setup

You should update two variables in the main `Makefile`:

`PROG_SERIAL` - change this to the serial of your Jlink programmer.

To get your serial number, run `jlinkexe` and you should see the output:

```
$ jlinkexe
SEGGER J-Link Commander V6.60c (Compiled Dec 23 2019 16:16:43)
DLL version V6.60c, compiled Dec 23 2019 16:16:28

Connecting to J-Link via USB...O.K.
Firmware: J-Link OB-SAM3U128-V2-NordicSemi compiled Jan  7 2019 14:07:15
Hardware version: V1.00
S/N: <YOUR SERIAL HERE>
License(s): RDI, FlashBP, FlashDL, JFlash, GDB
VTref=3.300V


Type "connect" to establish a target connection, '?' for help
```

Look for `S/N:` and you should see your serial!


1. `APP_FILENAME` - should match the name of your app. Make it something memorable!

## Building

`make build` will build your code
`make merge` will merge your code as one hex file. This includes the Softdevice
`make flash_softdevice` will flash the softdevice.
`make flash` will flash your app, bootloader and settings.

**Note:** on a fresh board, you should run `make erase`, `make flash_softdevice` then `make flash`

## Uninstalling Toolchain or SDK

1. Run `make sdk_clean` for the SDK
1. Run `make toolchain_clean` for the Toolchain

Both options will remove the respective directories.