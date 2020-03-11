# Start: Your configuration!

# Set this to the directory of pyrinas-os
# If you used a symbolic link this points to
# the `pyrinas-os` folder in this repository
OS_DIR := ../../../

# This should be the serial number of your Jlink programmer.
# To find simply run `jlinkexe`
PROG_SERIAL=123456789

# This is your debugger port for Jlink's RTT. If you
# have mulitple, you will have to change this on each app
# your're using
PROG_PORT=19021

# This is where you set your board type. Here are the supported boards:
# xenon - Particle Xenon
BOARD=xenon

# This is where you can name your app something. Make it specific
APP_FILENAME=pyrinas-sensor

# This determines whether or not you're using debug mode
# Comment this out or change to false
DEBUG=true

# End: Your Configuration

EXPORT_ARGS += \
	APP_DIR=$(PWD) \
	PROG_SERIAL=$(PROG_SERIAL) \
	PROG_PORT=$(PROG_PORT) \
	BOARD=$(BOARD) \
	APP_FILENAME=$(APP_FILENAME) \
	DEBUG=$(DEBUG)

.PHONY: build clean debug erase flash flash_softdevice ota rtt

default: build

build:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) build

clean:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) clean

debug:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) debug

erase:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) erase

flash:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) flash

flash_softdevice:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) flash_softdevice

ota:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) ota
	@mkdir -p _ota
	@cp $(OS_DIR)/_ota/$(APP_FILENAME)* _ota/

rtt:
	@export $(EXPORT_ARGS) && make -C $(OS_DIR) rtt