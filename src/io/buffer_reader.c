#include "buffer_reader.h"
#include <stdlib.h>
#include <string.h>

cxlsb_buffer_reader_t* cxlsb_buffer_reader_create(const uint8_t* buffer, size_t length) {
    if (buffer == NULL) {
        return NULL;
    }
    
    cxlsb_buffer_reader_t* reader = (cxlsb_buffer_reader_t*)malloc(sizeof(cxlsb_buffer_reader_t));
    if (reader == NULL) {
        return NULL;
    }
    
    reader->buffer = buffer;
    reader->length = length;
    reader->position = 0;
    
    return reader;
}

void cxlsb_buffer_reader_destroy(cxlsb_buffer_reader_t* reader) {
    if (reader != NULL) {
        free(reader);
    }
}

uint8_t cxlsb_buffer_reader_read_byte(cxlsb_buffer_reader_t* reader) {
    if (reader->position >= reader->length) {
        return 0;
    }
    return reader->buffer[reader->position++];
}

uint16_t cxlsb_buffer_reader_read_short(cxlsb_buffer_reader_t* reader) {
    if (reader->position + 2 > reader->length) {
        reader->position = reader->length;
        return 0;
    }
    
    uint16_t value = (uint16_t)reader->buffer[reader->position];
    value |= (uint16_t)reader->buffer[reader->position + 1] << 8;
    reader->position += 2;
    
    return value;
}

uint32_t cxlsb_buffer_reader_read_int(cxlsb_buffer_reader_t* reader) {
    if (reader->position + 4 > reader->length) {
        reader->position = reader->length;
        return 0;
    }
    
    uint32_t value = (uint32_t)reader->buffer[reader->position];
    value |= (uint32_t)reader->buffer[reader->position + 1] << 8;
    value |= (uint32_t)reader->buffer[reader->position + 2] << 16;
    value |= (uint32_t)reader->buffer[reader->position + 3] << 24;
    reader->position += 4;
    
    return value;
}

uint64_t cxlsb_buffer_reader_read_long(cxlsb_buffer_reader_t* reader) {
    if (reader->position + 8 > reader->length) {
        reader->position = reader->length;
        return 0;
    }
    
    uint64_t value = (uint64_t)reader->buffer[reader->position];
    value |= (uint64_t)reader->buffer[reader->position + 1] << 8;
    value |= (uint64_t)reader->buffer[reader->position + 2] << 16;
    value |= (uint64_t)reader->buffer[reader->position + 3] << 24;
    value |= (uint64_t)reader->buffer[reader->position + 4] << 32;
    value |= (uint64_t)reader->buffer[reader->position + 5] << 40;
    value |= (uint64_t)reader->buffer[reader->position + 6] << 48;
    value |= (uint64_t)reader->buffer[reader->position + 7] << 56;
    reader->position += 8;
    
    return value;
}

double cxlsb_buffer_reader_read_double(cxlsb_buffer_reader_t* reader) {
    uint64_t bits = cxlsb_buffer_reader_read_long(reader);
    double value;
    memcpy(&value, &bits, sizeof(double));
    return value;
}

cxlsb_error_t cxlsb_buffer_reader_read_bytes(cxlsb_buffer_reader_t* reader, 
                                              uint8_t* out, 
                                              size_t length) {
    if (out == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (reader->position + length > reader->length) {
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    memcpy(out, reader->buffer + reader->position, length);
    reader->position += length;
    
    return CXLSB_OK;
}

size_t cxlsb_buffer_reader_get_position(cxlsb_buffer_reader_t* reader) {
    return reader->position;
}

size_t cxlsb_buffer_reader_get_remaining(cxlsb_buffer_reader_t* reader) {
    return reader->length - reader->position;
}

bool cxlsb_buffer_reader_has_remaining(cxlsb_buffer_reader_t* reader, size_t required) {
    return reader->position + required <= reader->length;
}

void cxlsb_buffer_reader_set_position(cxlsb_buffer_reader_t* reader, size_t position) {
    if (position <= reader->length) {
        reader->position = position;
    } else {
        reader->position = reader->length;
    }
}

bool cxlsb_buffer_reader_is_eof(cxlsb_buffer_reader_t* reader) {
    return reader->position >= reader->length;
}