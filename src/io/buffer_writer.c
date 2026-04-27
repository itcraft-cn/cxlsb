#include "buffer_writer.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_INITIAL_CAPACITY 4096
#define LARGE_FILE_BUFFER_SIZE (16 * 1024 * 1024)

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create(size_t initial_capacity) {
    cxlsb_buffer_writer_t* writer = (cxlsb_buffer_writer_t*)malloc(sizeof(cxlsb_buffer_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_INITIAL_CAPACITY;
    }
    
    writer->buffer = (uint8_t*)malloc(initial_capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->capacity = initial_capacity;
    writer->initial_capacity = initial_capacity;
    writer->position = 0;
    writer->fixed_mode = false;
    
    return writer;
}

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create_fixed(size_t capacity) {
    cxlsb_buffer_writer_t* writer = (cxlsb_buffer_writer_t*)malloc(sizeof(cxlsb_buffer_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    if (capacity == 0) {
        capacity = LARGE_FILE_BUFFER_SIZE;
    }
    
    writer->buffer = (uint8_t*)malloc(capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->capacity = capacity;
    writer->initial_capacity = capacity;
    writer->position = 0;
    writer->fixed_mode = true;
    
    return writer;
}

void cxlsb_buffer_writer_destroy(cxlsb_buffer_writer_t* writer) {
    if (writer != NULL) {
        if (writer->buffer != NULL) {
            free(writer->buffer);
        }
        free(writer);
    }
}

cxlsb_error_t cxlsb_buffer_writer_ensure_capacity(cxlsb_buffer_writer_t* writer, 
                                                   size_t required) {
    size_t needed = writer->position + required;
    
    if (needed <= writer->capacity) {
        return CXLSB_OK;
    }
    
    if (writer->fixed_mode) {
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    size_t new_capacity = writer->capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    
    uint8_t* new_buffer = (uint8_t*)realloc(writer->buffer, new_capacity);
    if (new_buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    writer->buffer = new_buffer;
    writer->capacity = new_capacity;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_byte(cxlsb_buffer_writer_t* writer, uint8_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 1);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = value;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_short(cxlsb_buffer_writer_t* writer, uint16_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 2);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = (uint8_t)(value & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 8) & 0xFF);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_int(cxlsb_buffer_writer_t* writer, uint32_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 4);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = (uint8_t)(value & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 8) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 16) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 24) & 0xFF);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_long(cxlsb_buffer_writer_t* writer, uint64_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 8);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = (uint8_t)(value & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 8) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 16) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 24) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 32) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 40) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 48) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 56) & 0xFF);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_double(cxlsb_buffer_writer_t* writer, double value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 8);
    if (err != CXLSB_OK) {
        return err;
    }
    
    uint64_t bits;
    memcpy(&bits, &value, sizeof(double));
    
    return cxlsb_buffer_writer_write_long(writer, bits);
}

cxlsb_error_t cxlsb_buffer_writer_write_bytes(cxlsb_buffer_writer_t* writer, 
                                               const uint8_t* data, 
                                               size_t length) {
    if (data == NULL || length == 0) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, length);
    if (err != CXLSB_OK) {
        return err;
    }
    
    memcpy(writer->buffer + writer->position, data, length);
    writer->position += length;
    
    return CXLSB_OK;
}

size_t cxlsb_buffer_writer_get_position(cxlsb_buffer_writer_t* writer) {
    return writer->position;
}

uint8_t* cxlsb_buffer_writer_get_buffer(cxlsb_buffer_writer_t* writer) {
    return writer->buffer;
}

size_t cxlsb_buffer_writer_get_length(cxlsb_buffer_writer_t* writer) {
    return writer->position;
}

void cxlsb_buffer_writer_reset(cxlsb_buffer_writer_t* writer) {
    writer->position = 0;
}