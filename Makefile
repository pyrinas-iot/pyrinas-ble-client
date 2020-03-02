PROJ_DIR				:= $(shell pwd)
SDK_ROOT        := ./_sdk
SDK_TEMP        := ./_sdk_temp
OTA_DIR         := ./_ota
BUILD_DIR       := ./_build
OUT_DIR         := ./_out
TOOLCHAIN_DIR   := ./_toolchain
BOOTLOADER_DIR  := ./bootloader
DFU_DIR         := ./dfu
SDK_CONFIG_DIR  := ./sdk_config
MAIN_DIR        := ./main
BIN_DIR         := ./bin
SOURCE_DIR      := ./src
INCLUDE_DIR     := ./include
EXTERNAL_DIR    := ./external
PROTO_DIR       := ./proto

SETTINGS        := settings
BL_SETTINGS     := bl_settings
BL_SETTINGS_SD  := bl_settings_sd

NANOPB_DIR      := $(EXTERNAL_DIR)/nanopb
NANOPB_GEN      := $(NANOPB_DIR)/generator/nanopb_generator.py

# Board definition and Git versioning
include Makefile.ver
include Makefile.bid

# Check for errors. These should be set in the app.
ifndef PROG_SERIAL
$(info PROG_SERIAL not set. Using default of 1234)
PROG_SERIAL := 1234
endif

ifndef PROG_PORT
$(info PROG_PORT not set. Using default of 19020)
PROG_PORT := 19020
endif

ifndef APP_FILENAME
$(info APP_FILENAME not set. Using default of 19020)
APP_FILENAME := pyrinas
endif

msg = Building app for $(BOARD_DESC).
$(info )
$(info $(msg))
$(info )

# Cert for DFU
DFU_CERT 				:= private.pem

# Commands
NRFJPROG				:= $(BIN_DIR)/nrfjprog/nrfjprog
MERGEHEX				:= $(BIN_DIR)/mergehex/mergehex
NRFUTIL					:= $(BIN_DIR)/nrfutil/nrfutil-mac

# Download and install SDK deps
SDK_ZIP     := .nrf_sdk.zip
NRF_SDK_FOLDER_NAME := nRF5_SDK_16.0.0_98a08e2.zip
NRF_SDK_URL := https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/nRF5_SDK_16.0.0_98a08e2.zip
NRF_SDK_MD5 := cc2ccb57d2c7159fd37f9e04fca6cc64
GCC_ARCHIVE := .gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2
GCC_OUTPUT_FOLDER := gcc-arm-none-eabi-6-2017-q2-update
GCC_URL := https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2
GCC_MD5 := d536d7fb167c04b24f7f0d40cd739cac

# Soft device info
SOFT_DEVICE := $(SDK_ROOT)/components/softdevice/$(BUILD_SD)/hex/$(BUILD_SD)_$(BUILD_SD_VER)_softdevice.hex

# Protocol Buffers
PROTO_SRC   := $(wildcard $(PROTO_DIR)/*.proto)
PROTO_PB    := $(PROTO_SRC:.proto=.pb)

.PHONY: sdk sdk_clean clean build debug merge merge_all erase flash flash_all flash_softdevice ota settings default gen_key toolchain toolchain_clean sdk sdk_clean

default: build

gen_key:
	@echo Generating pem key. You should only run this once!
	cd $(DFU_DIR) && $(NRFUTIL) keys generate private.pem
	cd $(DFU_DIR) && $(NRFUTIL) keys display --key pk --format code private.pem > dfu_public_key.c

settings:
	@echo Generating settings .hex file
	@mkdir -p $(BUILD_DIR)
	$(NRFUTIL) settings generate --family $(BOARD_FAM) --application $(MAIN_DIR)/_build/$(BUILD_IDENT).hex --application-version-string $(VER_STRING) --bootloader-version 1 --bl-settings-version 1 $(BUILD_DIR)/$(SETTINGS).hex

build:
	@export GCC_ARM_TOOLCHAIN=$(PROJ_DIR)/$(TOOLCHAIN_DIR) && make -C $(MAIN_DIR) clean_app
	@export GCC_ARM_TOOLCHAIN=$(PROJ_DIR)/$(TOOLCHAIN_DIR) && make -C $(BOOTLOADER_DIR) -j
	@export GCC_ARM_TOOLCHAIN=$(PROJ_DIR)/$(TOOLCHAIN_DIR) && make -C $(MAIN_DIR) -j

merge: build settings
	@mkdir -p $(BUILD_DIR)
	@echo Merging settings with bootloader
	@$(MERGEHEX) -m $(BOOTLOADER_DIR)/_build/$(BUILD_IDENT)_$(BUILD_SD).hex $(BUILD_DIR)/$(SETTINGS).hex -o $(BUILD_DIR)/$(BL_SETTINGS).hex
	@echo Merging app with bootloader + settings
	@$(MERGEHEX) -m $(BUILD_DIR)/$(BL_SETTINGS).hex $(MAIN_DIR)/_build/$(BUILD_IDENT).hex -o $(OUT_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).combined.hex

merge_all: merge
	@echo Merging all files
	@$(MERGEHEX) -m $(SOFT_DEVICE) $(OUT_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).combined.hex -o $(OUT_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).full.hex

flash_all: merge_all
	@echo Flashing all
	$(NRFJPROG) -f nrf52 --program $(OUT_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).full.hex --chiperase -s $(PROG_SERIAL)
	@$(NRFJPROG) -f nrf52 --reset -s $(PROG_SERIAL)

flash: merge
	@echo Flashing firmware
	$(NRFJPROG) -f nrf52 --program $(OUT_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).combined.hex --sectoranduicrerase -s $(PROG_SERIAL)
	@$(NRFJPROG) -f nrf52 --reset -s $(PROG_SERIAL)

flash_softdevice:
	@echo Flashing softdevice
	$(NRFJPROG) -f nrf52 --program $(SOFT_DEVICE) --sectorerase -s $(PROG_SERIAL)
	@$(NRFJPROG) -f nrf52 --reset -s $(PROG_SERIAL)

erase:
	@echo Erasing device
	$(NRFJPROG) -e -s $(PROG_SERIAL)

ota: build
	@echo Generating OTA package
	@mkdir -p $(OTA_DIR)
	$(NRFUTIL) pkg generate --sd-req $(NRFUTIL_SD_REQ) --hw-version 52 --key-file $(DFU_DIR)/$(DFU_CERT) \
		--application-version-string $(VER_STRING) --application $(BUILD_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).hex \
		$(OTA_DIR)/$(APP_FILENAME).app.$(VER_STRING_W_GITHASH).zip

debug:
	@echo Debug using JLinkExe
	JLinkExe -device NRF52 -speed 4000 -if SWD -autoconnect 1 -SelectEmuBySN $(PROG_SERIAL) -RTTTelnetPort $(PROG_PORT)

rtt:
	jlinkrttclient -RTTTelnetPort $(PROG_PORT)

sdk:
	@echo Installing NRF SDK
	@if [ ! -d $(SDK_ROOT) ]; then \
		if [ ! -f $(SDK_ZIP) ]; then \
			echo Downloading sdk deps...; \
			curl -o $(SDK_ZIP) $(NRF_SDK_URL); \
		fi; \
		if [ "`md5 -q $(SDK_ZIP)`" != "$(NRF_SDK_MD5)" ]; then \
			echo SDK archive MD5 does not match. Delete and reinstall.; \
			exit 1; \
		fi; \
		if [ ! -f $(SDK_ROOT) ]; then \
			unzip $(SDK_ZIP) -d $(SDK_ROOT); \
		fi; \
	fi;
	@echo Copyiing toolchain configuration file..
	@cp -f $(SDK_CONFIG_DIR)/Makefile.posix $(SDK_ROOT)/components/toolchain/gcc/
	@echo Building Microecc
	@cp -f $(SDK_CONFIG_DIR)/build_all.sh $(SDK_ROOT)/external/micro-ecc/
	@cd $(SDK_ROOT)/external/micro-ecc/ && sh build_all.sh
	@echo SDK deps download and install complete.
	@rm -rf $(SDK_ZIP) $(SDK_TEMP) $(GCC_ARCHIVE)

toolchain:
	@echo Installing OSX tools
	@if [ ! -d $(TOOLCHAIN_DIR) ]; then \
		if [ ! -f $(GCC_ARCHIVE) ]; then \
			echo Downloading gcc...; \
			curl -o $(GCC_ARCHIVE) $(GCC_URL); \
		fi; \
		if [ "`md5 -q $(GCC_ARCHIVE)`" != "$(GCC_MD5)" ]; then \
			echo GCC archive MD5 does not match. Delete and reinstall.; \
			exit 1; \
		fi; \
		if [ ! -d $(GCC_OUTPUT_FOLDER) ]; then \
			tar jxfkv $(GCC_ARCHIVE); \
		fi; \
		if [ -d $(GCC_OUTPUT_FOLDER) ]; then \
			mv $(GCC_OUTPUT_FOLDER) $(TOOLCHAIN_DIR); \
		fi; \
	fi;
	@echo Removing temporary files.
	@rm -rf $(GCC_ARCHIVE)

%.pb: %.proto
	protoc -I$(PROTO_DIR) --go_out=$(PROTO_DIR) $<
	protoc -I$(PROTO_DIR) -o$*.pb $<
	@$(NANOPB_GEN) -I$(PROTO_DIR) $@
	pbjs -t static-module -p$(PROTO_DIR) $*.proto > $@.js
	@mkdir -p $(SOURCE_DIR)/proto
	@mkdir -p $(INCLUDE_DIR)/proto
	@mv $*.pb.c $(SOURCE_DIR)/proto
	@mv $*.pb.h $(INCLUDE_DIR)/proto
	protoc -I$(PROTO_DIR) --cpp_out=$(PROTO_DIR) $<

protobuf: protoclean $(PROTO_PB)
	@echo building the protocol buffers $(PROTO_PB)

protoclean:
	@rm -fr $(PROTO_DIR)/*.pb

sdk_clean:
		@echo SDK Clean..
		@rm -rf $(SDK_ROOT)
		@rm -f $(SDK_ZIP)

toolchain_clean:
		@echo Toolchain Clean..
		@rm -rf $(TOOLCHAIN_DIR)
		@rm -f $(GCC_ARCHIVE)

clean:
		@echo Cleaning..
		@rm -rf $(BUILD_DIR)
		@make -C $(MAIN_DIR) clean