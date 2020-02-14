#include "fs.h"
#include "flash.h"

#include "nrf_log_ctrl.h"

#include "app_error.h"

// #define NRF_LOG_MODULE_NAME fs
// #include "nrf_log.h"
// #include "nrf_log_ctrl.h"
// NRF_LOG_MODULE_REGISTER();

#define FS_MAX_READ_SIZE 128
#define FS_MAX_WRITE_SIZE 256
#define FS_MAX_LOOK_AHEAD_SIZE 256

// Buffers used by FS
uint8_t read_buffer[FS_MAX_READ_SIZE] __attribute__((aligned(4)));
uint8_t prog_buffer[FS_MAX_WRITE_SIZE] __attribute__((aligned(4)));
uint8_t lookahead_buffer[FS_MAX_LOOK_AHEAD_SIZE] __attribute__((aligned(4)));

// Variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

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
    .read_size = 256,
    .prog_size = 256,
    .block_size = 4096,
    .block_count = 512, /* 2 Mbyte external flash */
    .cache_size = 256,
    .lookahead_size = 256,
    .block_cycles = 500,

    // Buffers!
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

static void test()
{
    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

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
    NRF_LOG_INFO("fs_init");
    NRF_LOG_PROCESS();
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        lfs_format(&lfs, &cfg);
        err = lfs_mount(&lfs, &cfg);

        // Borks if there's still an error
        if (err) APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE);
    }

    UNUSED_VARIABLE(test);
}

void fs_write_raw(const char *filename, const char *data, size_t size)
{
}

void fw_read(const char *filename, const char *data, size_t size)
{
}

void fs_delete();