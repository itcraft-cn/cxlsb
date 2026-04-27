#include "biff12_writer.h"
#include "biff12_record_types.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include "../io/buffer_writer.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create(size_t initial_capacity) {
    cxlsb_biff12_writer_t* writer = (cxlsb_biff12_writer_t*)malloc(sizeof(cxlsb_biff12_writer_t));
    if (writer == NULL) return NULL;
    
    writer->buffer = cxlsb_buffer_writer_create(initial_capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    return writer;
}

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create_fixed(size_t capacity) {
    cxlsb_biff12_writer_t* writer = (cxlsb_biff12_writer_t*)malloc(sizeof(cxlsb_biff12_writer_t));
    if (writer == NULL) return NULL;
    
    writer->buffer = cxlsb_buffer_writer_create_fixed(capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    return writer;
}

void cxlsb_biff12_writer_destroy(cxlsb_biff12_writer_t* writer) {
    if (writer != NULL) {
        if (writer->buffer != NULL) cxlsb_buffer_writer_destroy(writer->buffer);
        free(writer);
    }
}

cxlsb_error_t cxlsb_biff12_writer_write_record_header(cxlsb_biff12_writer_t* writer,
                                                       uint32_t record_type,
                                                       uint32_t record_size) {
    uint8_t type_buf[5], size_buf[5];
    int type_len = cxlsb_varint_encode(record_type, type_buf);
    int size_len = cxlsb_varint_encode(record_size, size_buf);
    
    cxlsb_error_t err = cxlsb_buffer_writer_write_bytes(writer->buffer, type_buf, type_len);
    if (err != CXLSB_OK) return err;
    return cxlsb_buffer_writer_write_bytes(writer->buffer, size_buf, size_len);
}

cxlsb_error_t cxlsb_biff12_writer_write_empty_record(cxlsb_biff12_writer_t* writer,
                                                      uint32_t record_type) {
    return cxlsb_biff12_writer_write_record_header(writer, record_type, 0);
}

cxlsb_error_t cxlsb_biff12_writer_write_varint(cxlsb_biff12_writer_t* writer, 
                                                uint32_t value) {
    uint8_t buf[5];
    int len = cxlsb_varint_encode(value, buf);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, buf, len);
}

cxlsb_error_t cxlsb_biff12_writer_write_wide_string(cxlsb_biff12_writer_t* writer,
                                                     const char* str) {
    if (str == NULL) return cxlsb_buffer_writer_write_int(writer->buffer, 0);
    
    size_t str_len = strlen(str);
    cxlsb_error_t err = cxlsb_buffer_writer_write_int(writer->buffer, (uint32_t)str_len);
    if (err != CXLSB_OK) return err;
    
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
    if (utf16_buf == NULL) return CXLSB_ERR_NO_MEMORY;
    
    err = cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
    if (err != CXLSB_OK) { free(utf16_buf); return err; }
    
    err = cxlsb_buffer_writer_write_bytes(writer->buffer, utf16_buf, utf16_len);
    free(utf16_buf);
    return err;
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_real(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   double value, uint32_t xf_id) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_CELL_REAL, 20);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, col);
    cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    return cxlsb_buffer_writer_write_double(writer->buffer, value);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_st(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 const char* str, uint32_t xf_id) {
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    uint32_t record_size = 16 + 1 + 4 + utf16_len;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_CELL_ST, record_size);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, col);
    cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    return cxlsb_biff12_writer_write_wide_string(writer, str);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_isst(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   uint32_t sst_index, uint32_t xf_id) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_CELL_ISST, 16);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, col);
    cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    return cxlsb_buffer_writer_write_int(writer->buffer, sst_index);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_bool(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   bool value, uint32_t xf_id) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_CELL_BOOL, 13);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, col);
    cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    return cxlsb_buffer_writer_write_byte(writer->buffer, value ? 1 : 0);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_blank(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, uint32_t col,
                                                    uint32_t xf_id) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_CELL_BLANK, 12);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, col);
    return cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
}

cxlsb_error_t cxlsb_biff12_writer_write_row_header(cxlsb_biff12_writer_t* writer,
                                                     uint32_t row, 
                                                     uint32_t first_col,
                                                     uint32_t last_col) {
    uint32_t record_size = 4 + 4 + 2 + 3 + 4 + 8;
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_ROW_HDR, record_size);
    if (err != CXLSB_OK) return err;
    
    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    cxlsb_buffer_writer_write_short(writer->buffer, 270);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    cxlsb_buffer_writer_write_int(writer->buffer, 1);
    cxlsb_buffer_writer_write_int(writer->buffer, first_col);
    return cxlsb_buffer_writer_write_int(writer->buffer, last_col);
}

cxlsb_error_t cxlsb_biff12_writer_write_ws_dim(cxlsb_biff12_writer_t* writer,
                                                uint32_t first_row,
                                                uint32_t first_col,
                                                uint32_t last_row,
                                                uint32_t last_col) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_WS_DIM, 16);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, first_row);
    cxlsb_buffer_writer_write_int(writer->buffer, last_row);
    cxlsb_buffer_writer_write_int(writer->buffer, first_col);
    return cxlsb_buffer_writer_write_int(writer->buffer, last_col);
}

uint8_t* cxlsb_biff12_writer_get_buffer(cxlsb_biff12_writer_t* writer) {
    return cxlsb_buffer_writer_get_buffer(writer->buffer);
}

size_t cxlsb_biff12_writer_get_length(cxlsb_biff12_writer_t* writer) {
    return cxlsb_buffer_writer_get_length(writer->buffer);
}

void cxlsb_biff12_writer_reset(cxlsb_biff12_writer_t* writer) {
    cxlsb_buffer_writer_reset(writer->buffer);
}