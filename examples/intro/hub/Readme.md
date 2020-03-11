# Pyrinas OS App

## Getting Started

First you will have to download and bootstrap the OS directory.
It's best to clone it in to the same parent directory as this project.

```
git clone https://github.com/pyrinas-iot/pyrinas-os.git
```

Then you can set up a symbolic link to the OS

```
ln -s ../pyrinas-os/ .
```

Using the provided `Makefile`, you can now flash, debug and more.

## Starting fresh

If you're starting fresh with a new piece of hardware, here's what you need to do:

1. `make erase` your device.
1. `make flash_softdevice`
1. `make flash`

As long as things are plugged in, you should get some successful prompts.

Now, you can open up the debug console using `make debug`.

```
$ make debug

Building app in Peripheral mode for Xenon.

Debug using JLinkExe
JLinkExe -device NRF52 -speed 4000 -if SWD -autoconnect 1 -SelectEmuBySN 1234567 -RTTTelnetPort 19021
SEGGER J-Link Commander V6.62a (Compiled Jan 31 2020 12:59:22)
DLL version V6.62a, compiled Jan 31 2020 12:59:05

RTT Telnet Port set to 19021
Connecting to J-Link via USB...O.K.
Firmware: J-Link OB-SAM3U128-V2-NordicSemi compiled Jan 21 2020 17:30:48
Hardware version: V1.00
S/N: 682978319
License(s): RDI, FlashBP, FlashDL, JFlash, GDB
VTref=3.300V
Device "NRF52" selected.
...

```

Then, in a separate window, open `make rtt`

```
$ make rtt

Building app in Peripheral mode for Xenon.

jlinkrttclient -RTTTelnetPort 19021
###RTT Client: ************************************************************
###RTT Client: *               SEGGER Microcontroller GmbH                *
###RTT Client: *   Solutions for real time microcontroller applications   *
###RTT Client: ************************************************************
###RTT Client: *                                                          *
###RTT Client: *       (c) 2012 - 2016  SEGGER Microcontroller GmbH       *
###RTT Client: *                                                          *
###RTT Client: *     www.segger.com     Support: support@segger.com       *
###RTT Client: *                                                          *
###RTT Client: ************************************************************
###RTT Client: *                                                          *
###RTT Client: * SEGGER J-Link RTT Client   Compiled Jan 31 2020 13:00:24 *
###RTT Client: *                                                          *
###RTT Client: ************************************************************

###RTT Client: -----------------------------------------------
###RTT Client: Connecting to J-Link RTT Server via localhost:19021 .....
```

## Commands

`make build` - builds your app.

`make clean` - cleans all remnants of your app.

`make debug` - opens up the `jlinkexe` debugger console.

`make erase` - erases the chip attached to your programmer.

`make flash` - flashes the current app to your connected device.

`make flash_softdevice` - flashes the soft_device

`make rtt` - opens up the debug console.

## License

**Make it yours!**

This repository is a **template**. Meaning it provides the scaffolding for you
to create awesome things. Set your license here.

**pyrinas-os** is under the BSD 3 license.