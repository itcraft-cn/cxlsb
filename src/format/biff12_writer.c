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

static void put_cell_header(uint8_t* dst, uint8_t record_type, uint8_t record_size,
                            uint32_t col, uint32_t xf_id) {
    dst[0] = record_type;
    dst[1] = record_size;
    dst[2] = (uint8_t)(col & 0xFF);
    dst[3] = (uint8_t)((col >> 8) & 0xFF);
    dst[4] = (uint8_t)((col >> 16) & 0xFF);
    dst[5] = (uint8_t)((col >> 24) & 0xFF);
    dst[6] = (uint8_t)(xf_id & 0xFF);
    dst[7] = (uint8_t)((xf_id >> 8) & 0xFF);
    dst[8] = (uint8_t)((xf_id >> 16) & 0xFF);
    dst[9] = 0; /* flags */
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_real(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   double value, uint32_t xf_id) {
    (void)row;
    uint8_t b[18];
    put_cell_header(b, BIFF12_BRT_CELL_REAL, 16, col, xf_id);
    memcpy(b + 10, &value, 8);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, b, sizeof(b));
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_rk(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 double value, uint32_t xf_id) {
    (void)row;
    uint8_t b[14];
    put_cell_header(b, BIFF12_BRT_CELL_RK, 12, col, xf_id);

    /* 标准 RK 整数形式：num(30 位有符号)<<2 | fInt(bit1)=1。 */
    int32_t iv = (int32_t)value;
    uint32_t rk = ((uint32_t)iv << 2) | 0x02;
    b[10] = (uint8_t)(rk & 0xFF);
    b[11] = (uint8_t)((rk >> 8) & 0xFF);
    b[12] = (uint8_t)((rk >> 16) & 0xFF);
    b[13] = (uint8_t)((rk >> 24) & 0xFF);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, b, sizeof(b));
}

/* |n| < 2^29 的整数可用 RK 存储（RK = f64 高 32 位）。 */
static bool can_use_rk(double value) {
    if (value < -536870912.0 || value > 536870911.0) return false;
    return value == (double)(int32_t)value;
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_number(cxlsb_biff12_writer_t* writer,
                                                     uint32_t row, uint32_t col,
                                                     double value, uint32_t xf_id) {
    if (can_use_rk(value)) {
        return cxlsb_biff12_writer_write_cell_rk(writer, row, col, value, xf_id);
    }
    return cxlsb_biff12_writer_write_cell_real(writer, row, col, value, xf_id);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_st(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 const char* str, uint32_t xf_id) {
    (void)row;
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    uint32_t record_size = 8 + 4 + (uint32_t)utf16_len;

    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_CELL_ST, record_size);
    if (err != CXLSB_OK) return err;

    uint8_t b[10];
    put_cell_header(b, 0, 0, col, xf_id); /* 仅复用前 10 字节的布局，头部已单独写出 */
    err = cxlsb_buffer_writer_write_bytes(writer->buffer, b + 2, 8);
    if (err != CXLSB_OK) return err;
    return cxlsb_biff12_writer_write_wide_string(writer, str);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_isst(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   uint32_t sst_index, uint32_t xf_id) {
    (void)row;
    uint8_t b[14];
    put_cell_header(b, BIFF12_BRT_CELL_ISST, 12, col, xf_id);
    b[10] = (uint8_t)(sst_index & 0xFF);
    b[11] = (uint8_t)((sst_index >> 8) & 0xFF);
    b[12] = (uint8_t)((sst_index >> 16) & 0xFF);
    b[13] = (uint8_t)((sst_index >> 24) & 0xFF);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, b, sizeof(b));
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_bool(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   bool value, uint32_t xf_id) {
    (void)row;
    uint8_t b[11];
    put_cell_header(b, BIFF12_BRT_CELL_BOOL, 9, col, xf_id);
    b[10] = value ? 1 : 0;
    return cxlsb_buffer_writer_write_bytes(writer->buffer, b, sizeof(b));
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_blank(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, uint32_t col,
                                                    uint32_t xf_id) {
    (void)row;
    uint8_t b[10];
    put_cell_header(b, BIFF12_BRT_CELL_BLANK, 8, col, xf_id);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, b, sizeof(b));
}

cxlsb_error_t cxlsb_biff12_writer_write_row_header(cxlsb_biff12_writer_t* writer,
                                                     uint32_t row, 
                                                     uint32_t col_count) {
    uint32_t num_spans = col_count > 0 ? (col_count - 1) / 1024 + 1 : 0;

    /* 常见路径：单 span（列数 <= 1024），固定 27 字节整块写出。 */
    if (num_spans == 1) {
        uint8_t b[27];
        memset(b, 0, sizeof(b));
        b[0] = BIFF12_BRT_ROW_HDR; /* 1 字节 varint */
        b[1] = 25;                 /* 记录长度 */
        b[2] = (uint8_t)(row & 0xFF);
        b[3] = (uint8_t)((row >> 8) & 0xFF);
        b[4] = (uint8_t)((row >> 16) & 0xFF);
        b[5] = (uint8_t)((row >> 24) & 0xFF);
        /* b[6..9] ixfe = 0 */
        b[10] = 0x0E; /* miyRw = 270 */
        b[11] = 0x01;
        /* b[12..14] flags = 0 */
        b[15] = 1; /* ccolspan = 1 */
        /* span: colMic = 0, colLast = col_count - 1 */
        b[23] = (uint8_t)((col_count - 1) & 0xFF);
        b[24] = (uint8_t)(((col_count - 1) >> 8) & 0xFF);
        b[25] = (uint8_t)(((col_count - 1) >> 16) & 0xFF);
        b[26] = (uint8_t)(((col_count - 1) >> 24) & 0xFF);
        return cxlsb_buffer_writer_write_bytes(writer->buffer, b, sizeof(b));
    }

    uint32_t record_size = 4 + 4 + 2 + 3 + 4 + num_spans * 8;
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_ROW_HDR, record_size);
    if (err != CXLSB_OK) return err;

    cxlsb_buffer_writer_write_int(writer->buffer, row);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    cxlsb_buffer_writer_write_short(writer->buffer, 270);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    cxlsb_buffer_writer_write_int(writer->buffer, num_spans);

    for (uint32_t seg = 0; seg < num_spans; seg++) {
        uint32_t seg_start = seg * 1024;
        uint32_t seg_end = (seg + 1) * 1024 - 1;
        if (seg_end >= col_count) seg_end = col_count - 1;
        cxlsb_buffer_writer_write_int(writer->buffer, seg_start);
        cxlsb_buffer_writer_write_int(writer->buffer, seg_end);
    }
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_writer_write_ws_dim(cxlsb_biff12_writer_t* writer,
                                                uint32_t first_row,
                                                uint32_t last_row,
                                                uint32_t first_col,
                                                uint32_t last_col) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_WS_DIM, 16);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, first_row);
    cxlsb_buffer_writer_write_int(writer->buffer, last_row);
    cxlsb_buffer_writer_write_int(writer->buffer, first_col);
    return cxlsb_buffer_writer_write_int(writer->buffer, last_col);
}

cxlsb_error_t cxlsb_biff12_writer_write_wide_string(cxlsb_biff12_writer_t* writer, const char* str) {
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    size_t char_count = utf16_len / 2;
    
    cxlsb_buffer_writer_write_int(writer->buffer, char_count);
    
    static uint8_t utf16_buf[2048];
    if (utf16_len > sizeof(utf16_buf)) {
        uint8_t* dynamic_buf = (uint8_t*)malloc(utf16_len);
        if (dynamic_buf == NULL) return CXLSB_ERR_NO_MEMORY;
        cxlsb_utf8_to_utf16le(str, dynamic_buf, &utf16_len);
        cxlsb_error_t err = cxlsb_buffer_writer_write_bytes(writer->buffer, dynamic_buf, utf16_len);
        free(dynamic_buf);
        return err;
    }
    
    cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, utf16_buf, utf16_len);
}

void cxlsb_biff12_writer_reset(cxlsb_biff12_writer_t* writer) {
    if (writer != NULL && writer->buffer != NULL) {
        cxlsb_buffer_writer_reset(writer->buffer);
    }
}

size_t cxlsb_biff12_writer_get_length(cxlsb_biff12_writer_t* writer) {
    if (writer == NULL || writer->buffer == NULL) return 0;
    return cxlsb_buffer_writer_get_length(writer->buffer);
}

const uint8_t* cxlsb_biff12_writer_get_buffer(cxlsb_biff12_writer_t* writer) {
    if (writer == NULL || writer->buffer == NULL) return NULL;
    return cxlsb_buffer_writer_get_buffer(writer->buffer);
}