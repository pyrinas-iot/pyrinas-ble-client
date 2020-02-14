#ifndef FLASH_H
#define FLASH_H

#include "lfs.h"

#define QSPI_STD_CMD_WRSR 0x01
#define QSPI_STD_CMD_RSTEN 0x66
#define QSPI_STD_CMD_RST 0x99
#define QSPI_MX25_CMD_RDP 0xab

void flash_init(void);
int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int flash_erase(const struct lfs_config *c, lfs_block_t block);
int flash_sync(const struct lfs_config *c);

#endif