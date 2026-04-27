#ifndef CXLSB_XLSB_CONTAINER_READER_H
#define CXLSB_XLSB_CONTAINER_READER_H

#include <stdint.h>
#include <stdbool.h>
#include "../io/buffer_reader.h"
#include "cxlsb/error_codes.h"
#include "cxlsb/types.h"

typedef struct cxlsb_container_reader {
    void* zip_handle;
    char* path;
    bool is_open;
} cxlsb_container_reader_t;

cxlsb_container_reader_t* cxlsb_container_reader_create(const char* path);

void cxlsb_container_reader_destroy(cxlsb_container_reader_t* reader);

bool cxlsb_container_reader_has_entry(cxlsb_container_reader_t* reader, const char* name);

cxlsb_error_t cxlsb_container_reader_get_entry(cxlsb_container_reader_t* reader,
                                                const char* name,
                                                uint8_t** out_data,
                                                size_t* out_length);

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sheet_stream(
    cxlsb_container_reader_t* reader, int sheet_index);

bool cxlsb_container_reader_has_sst(cxlsb_container_reader_t* reader);

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sst_stream(cxlsb_container_reader_t* reader);

uint32_t cxlsb_container_reader_get_sheet_count(cxlsb_container_reader_t* reader);

#endif /* CXLSB_XLSB_CONTAINER_READER_H */