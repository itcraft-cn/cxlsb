#ifndef CXLSB_BUFFER_READER_H
#define CXLSB_BUFFER_READER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

typedef struct cxlsb_buffer_reader {
    const uint8_t* buffer;
    size_t length;
    size_t position;
} cxlsb_buffer_reader_t;

cxlsb_buffer_reader_t* cxlsb_buffer_reader_create(const uint8_t* buffer, size_t length);

void cxlsb_buffer_reader_destroy(cxlsb_buffer_reader_t* reader);

uint8_t cxlsb_buffer_reader_read_byte(cxlsb_buffer_reader_t* reader);

uint16_t cxlsb_buffer_reader_read_short(cxlsb_buffer_reader_t* reader);

uint32_t cxlsb_buffer_reader_read_int(cxlsb_buffer_reader_t* reader);

uint64_t cxlsb_buffer_reader_read_long(cxlsb_buffer_reader_t* reader);

double cxlsb_buffer_reader_read_double(cxlsb_buffer_reader_t* reader);

cxlsb_error_t cxlsb_buffer_reader_read_bytes(cxlsb_buffer_reader_t* reader, 
                                              uint8_t* out, 
                                              size_t length);

size_t cxlsb_buffer_reader_get_position(cxlsb_buffer_reader_t* reader);

size_t cxlsb_buffer_reader_get_remaining(cxlsb_buffer_reader_t* reader);

bool cxlsb_buffer_reader_has_remaining(cxlsb_buffer_reader_t* reader, size_t required);

void cxlsb_buffer_reader_set_position(cxlsb_buffer_reader_t* reader, size_t position);

bool cxlsb_buffer_reader_is_eof(cxlsb_buffer_reader_t* reader);

const uint8_t* cxlsb_buffer_reader_get_buffer(cxlsb_buffer_reader_t* reader);

#endif /* CXLSB_BUFFER_READER_H */