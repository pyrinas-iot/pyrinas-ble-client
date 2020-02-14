#ifndef FS_H
#define FS_H

#include "lfs.h"
#include "lfs_util.h"

void fs_init();
void fs_write_raw(const char *filename, const char *data, size_t size);
void fw_read_raw(const char *filename, char *data, size_t size);
void fs_delete(const char *filename);

#endif