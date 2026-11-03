#ifndef CXLSB_XLSB_CONTAINER_H
#define CXLSB_XLSB_CONTAINER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

typedef struct cxlsb_container {
    void* zip_handle;
    char* path;
    bool is_open;
    int compression_level;
} cxlsb_container_t;

#define CXLSB_DEFAULT_COMPRESSION_LEVEL 1

cxlsb_container_t* cxlsb_container_create(const char* path);

/** 设置 ZIP 压缩级别（0=store，1-9=deflate；默认 1，速度优先）。 */
void cxlsb_container_set_compression_level(cxlsb_container_t* container, int level);

void cxlsb_container_destroy(cxlsb_container_t* container);

cxlsb_error_t cxlsb_container_add_entry(cxlsb_container_t* container,
                                          const char* name,
                                          const uint8_t* data,
                                          size_t length);

cxlsb_error_t cxlsb_container_close(cxlsb_container_t* container);

#endif /* CXLSB_XLSB_CONTAINER_H */