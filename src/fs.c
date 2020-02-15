/**
 *
 * Copyright (c) 2020, Jared Wolff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "fs.h"
#include "flash.h"

#include "nrf_log_ctrl.h"

#include "app_error.h"

// #define NRF_LOG_MODULE_NAME fs
// #include "nrf_log.h"
// #include "nrf_log_ctrl.h"
// NRF_LOG_MODULE_REGISTER();

#define FS_BUFFER_SIZE 256
#define FS_LOOK_AHEAD_SIZE 128

// Buffers used by FS
static uint8_t file_buffer[FS_BUFFER_SIZE];
static uint8_t read_buffer[FS_BUFFER_SIZE];
static uint8_t prog_buffer[FS_BUFFER_SIZE];
static uint8_t lookahead_buffer[FS_LOOK_AHEAD_SIZE];

// Variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

// Configuration for file
const struct lfs_file_config file_cfg = {
    .buffer = file_buffer,
};

// Configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    // TODO: set driver endpoints here
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,

    // block device configuration
    // TODO: tenative size is 256
    .read_size = FS_BUFFER_SIZE,
    .prog_size = FS_BUFFER_SIZE,
    .block_size = 4096,
    .block_count = 512, /* 2 Mbyte external flash */
    .lookahead_size = FS_LOOK_AHEAD_SIZE,
    .cache_size = FS_BUFFER_SIZE,
    .block_cycles = 500,

    // Buffers!
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

__STATIC_INLINE void fs_check_error(int error)
{
    // App error if < 0
    if (error < 0)
    {
        NRF_LOG_ERROR("fs error");
        APP_ERROR_CHECK(NRF_ERROR_INTERNAL);
    }
}

static void fs_boot_count()
{
    // read current count
    uint32_t boot_count = 0;

    lfs_file_opencfg(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT, &file_cfg);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    NRF_LOG_INFO("Boot count: %d", boot_count);

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);
}

void fs_init()
{

    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        NRF_LOG_ERROR("Unable to mount fs.");

        err = lfs_format(&lfs, &cfg);
        if (err)
        {
            APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE);
        }

        err = lfs_mount(&lfs, &cfg);
        // Borks if there's still an error
        if (err)
            APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE);
    }

    fs_boot_count();
}
void fs_write(const char *filename, const void *data, size_t size)
{
    int err = 0;

    // Mount the fs
    err = lfs_mount(&lfs, &cfg);
    if (err)
        fs_check_error(err);

    // Open the file for writing
    err = lfs_file_opencfg(&lfs, &file, filename, LFS_O_RDWR | LFS_O_CREAT, &file_cfg);
    if (err)
        fs_check_error(err);

    // Write the data
    err = lfs_file_write(&lfs, &file, data, size);
    if (err)
        fs_check_error(err);

    // remember the storage is not updated until the file is closed successfully
    err = lfs_file_close(&lfs, &file);
    if (err)
        fs_check_error(err);

    // release any resources we were using
    err = lfs_unmount(&lfs);
    if (err)
        fs_check_error(err);
}

void fs_read(const char *filename, void *data, size_t size)
{
    // Mount the fs
    int err = lfs_mount(&lfs, &cfg);
    if (err)
        fs_check_error(err);

    // Open the file for writing
    err = lfs_file_opencfg(&lfs, &file, filename, LFS_O_RDONLY, &file_cfg);
    if (err)
        fs_check_error(err);

    // Write the data
    err = lfs_file_read(&lfs, &file, data, size);
    if (err)
        fs_check_error(err);

    // remember the storage is not updated until the file is closed successfully
    err = lfs_file_close(&lfs, &file);
    if (err)
        fs_check_error(err);

    // release any resources we were using
    err = lfs_unmount(&lfs);
    if (err)
        fs_check_error(err);
}

void fs_delete(const char *filename)
{
    // Mount the fs
    int err = lfs_mount(&lfs, &cfg);
    if (err)
        fs_check_error(err);

    // Delete the file
    err = lfs_remove(&lfs, filename);
    if (err)
        fs_check_error(err);

    // release any resources we were using
    err = lfs_unmount(&lfs);
    if (err)
        fs_check_error(err);
    fs_check_error(err);
}