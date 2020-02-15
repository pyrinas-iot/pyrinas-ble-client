#ifndef FS_H
#define FS_H

#include "lfs.h"
#include "lfs_util.h"

void fs_init();
void fs_write(const char *filename, const void *data, size_t size);
void fs_read(const char *filename, void *data, size_t size);
void fs_delete(const char *filename);

#endif