#ifndef CXLSB_BUFFER_WRITER_H
#define CXLSB_BUFFER_WRITER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

typedef struct cxlsb_buffer_writer {
    uint8_t* buffer;
    size_t capacity;
    size_t position;
    size_t initial_capacity;
    bool fixed_mode;
} cxlsb_buffer_writer_t;

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create(size_t initial_capacity);

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create_fixed(size_t capacity);

void cxlsb_buffer_writer_destroy(cxlsb_buffer_writer_t* writer);

cxlsb_error_t cxlsb_buffer_writer_write_byte(cxlsb_buffer_writer_t* writer, uint8_t value);

cxlsb_error_t cxlsb_buffer_writer_write_short(cxlsb_buffer_writer_t* writer, uint16_t value);

cxlsb_error_t cxlsb_buffer_writer_write_int(cxlsb_buffer_writer_t* writer, uint32_t value);

cxlsb_error_t cxlsb_buffer_writer_write_long(cxlsb_buffer_writer_t* writer, uint64_t value);

cxlsb_error_t cxlsb_buffer_writer_write_double(cxlsb_buffer_writer_t* writer, double value);

cxlsb_error_t cxlsb_buffer_writer_write_bytes(cxlsb_buffer_writer_t* writer, 
                                               const uint8_t* data, 
                                               size_t length);

size_t cxlsb_buffer_writer_get_position(cxlsb_buffer_writer_t* writer);

uint8_t* cxlsb_buffer_writer_get_buffer(cxlsb_buffer_writer_t* writer);

size_t cxlsb_buffer_writer_get_length(cxlsb_buffer_writer_t* writer);

void cxlsb_buffer_writer_reset(cxlsb_buffer_writer_t* writer);

cxlsb_error_t cxlsb_buffer_writer_ensure_capacity(cxlsb_buffer_writer_t* writer, 
                                                   size_t required);

#endif /* CXLSB_BUFFER_WRITER_H */