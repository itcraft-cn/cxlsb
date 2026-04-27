#include "cxlsb/xlsb_reader.h"
#include "../container/xlsb_container_reader.h"
#include "../format/biff12_record_types.h"
#include "../io/buffer_reader.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include "../format/sst_table.h"
#include <stdlib.h>
#include <string.h>

struct cxlsb_reader {
    cxlsb_container_reader_t* container;
    cxlsb_sst_table_t* sst;
    uint32_t sheet_count;
};

cxlsb_reader_t* cxlsb_reader_create(const char* path) {
    if (path == NULL) return NULL;
    
    cxlsb_reader_t* reader = (cxlsb_reader_t*)malloc(sizeof(cxlsb_reader_t));
    if (reader == NULL) return NULL;
    
    reader->container = cxlsb_container_reader_create(path);
    if (reader->container == NULL) { free(reader); return NULL; }
    
    reader->sst = NULL;
    reader->sheet_count = cxlsb_container_reader_get_sheet_count(reader->container);
    
    if (cxlsb_container_reader_has_sst(reader->container)) {
        cxlsb_buffer_reader_t* sst_stream = cxlsb_container_reader_get_sst_stream(reader->container);
        if (sst_stream != NULL) {
            reader->sst = cxlsb_sst_table_create(1024);
            if (reader->sst != NULL) {
                while (cxlsb_buffer_reader_has_remaining(sst_stream, 2)) {
                    uint32_t type, size;
                    size_t bytes_read;
                    cxlsb_varint_decode_at(cxlsb_buffer_reader_get_buffer(sst_stream) + cxlsb_buffer_reader_get_position(sst_stream), &type, &bytes_read);
                    cxlsb_buffer_reader_set_position(sst_stream, cxlsb_buffer_reader_get_position(sst_stream) + bytes_read);
                    cxlsb_varint_decode_at(cxlsb_buffer_reader_get_buffer(sst_stream) + cxlsb_buffer_reader_get_position(sst_stream), &size, &bytes_read);
                    cxlsb_buffer_reader_set_position(sst_stream, cxlsb_buffer_reader_get_position(sst_stream) + bytes_read);
                    
                    if (type == BIFF12_BRT_SST_ITEM) {
                        cxlsb_buffer_reader_read_byte(sst_stream);
                        uint32_t str_len = cxlsb_buffer_reader_read_int(sst_stream);
                        if (str_len > 0) {
                            size_t utf16_len = str_len * 2;
                            uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
                            if (utf16_buf != NULL) {
                                cxlsb_buffer_reader_read_bytes(sst_stream, utf16_buf, utf16_len);
                                char* str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
                                if (str != NULL) {
                                    cxlsb_sst_table_add_string(reader->sst, str);
                                    free(str);
                                }
                                free(utf16_buf);
                            }
                        }
                    } else {
                        cxlsb_buffer_reader_set_position(sst_stream, cxlsb_buffer_reader_get_position(sst_stream) + size);
                    }
                }
            }
            uint8_t* buf = cxlsb_buffer_reader_get_buffer(sst_stream);
            cxlsb_buffer_reader_destroy(sst_stream);
            free(buf);
        }
    }
    return reader;
}

void cxlsb_reader_destroy(cxlsb_reader_t* reader) {
    if (reader != NULL) {
        if (reader->sst) cxlsb_sst_table_destroy(reader->sst);
        if (reader->container) cxlsb_container_reader_destroy(reader->container);
        free(reader);
    }
}

int cxlsb_reader_get_sheet_count(cxlsb_reader_t* reader) {
    return reader == NULL ? -1 : (int)reader->sheet_count;
}

cxlsb_error_t cxlsb_reader_for_each_row(cxlsb_reader_t* reader, int sheet_index, cxlsb_row_handler_fn handler, void* user_data) {
    if (reader == NULL || handler == NULL) return CXLSB_ERR_NULL_PTR;
    if (sheet_index < 0 || (uint32_t)sheet_index >= reader->sheet_count) return CXLSB_ERR_INVALID_SHEET_INDEX;
    
    cxlsb_buffer_reader_t* sheet_stream = cxlsb_container_reader_get_sheet_stream(reader->container, sheet_index);
    if (sheet_stream == NULL) return CXLSB_ERR_ZIP_ENTRY_NOT_FOUND;
    
    int row_count = 0;
    uint32_t current_row = 0;
    int col_count = 0;
    cxlsb_cell_data_t* cells = NULL;
    
    while (cxlsb_buffer_reader_has_remaining(sheet_stream, 2)) {
        uint32_t type, size;
        size_t bytes_read;
        cxlsb_varint_decode_at(cxlsb_buffer_reader_get_buffer(sheet_stream) + cxlsb_buffer_reader_get_position(sheet_stream), &type, &bytes_read);
        cxlsb_buffer_reader_set_position(sheet_stream, cxlsb_buffer_reader_get_position(sheet_stream) + bytes_read);
        cxlsb_varint_decode_at(cxlsb_buffer_reader_get_buffer(sheet_stream) + cxlsb_buffer_reader_get_position(sheet_stream), &size, &bytes_read);
        cxlsb_buffer_reader_set_position(sheet_stream, cxlsb_buffer_reader_get_position(sheet_stream) + bytes_read);
        
        if (type == BIFF12_BRT_END_SHEET_DATA) break;
        if (type == BIFF12_BRT_BEGIN_SHEET_DATA) continue;
        if (type == BIFF12_BRT_ROW_HDR) {
            current_row = cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t first_col = cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t last_col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            col_count = last_col - first_col + 1;
            cells = (cxlsb_cell_data_t*)malloc(sizeof(cxlsb_cell_data_t) * col_count);
            for (int i = 0; i < col_count; i++) cells[i] = cxlsb_cell_blank();
        } else if (type == BIFF12_BRT_CELL_REAL && cells != NULL) {
            uint32_t row = cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_int(sheet_stream);
            double val = cxlsb_buffer_reader_read_double(sheet_stream);
            if (col < col_count) cells[col] = cxlsb_cell_number(val);
        } else if (type == BIFF12_BRT_CELL_ISST && cells != NULL) {
            uint32_t row = cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t sst_idx = cxlsb_buffer_reader_read_int(sheet_stream);
            if (col < col_count && reader->sst != NULL) {
                const char* str = cxlsb_sst_table_get_string(reader->sst, sst_idx);
                if (str != NULL) cells[col] = cxlsb_cell_text(str);
            }
        } else if (type == BIFF12_BRT_CELL_ST && cells != NULL) {
            uint32_t row = cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint32_t str_len = cxlsb_buffer_reader_read_int(sheet_stream);
            if (str_len > 0) {
                size_t utf16_len = str_len * 2;
                uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
                if (utf16_buf != NULL) {
                    cxlsb_buffer_reader_read_bytes(sheet_stream, utf16_buf, utf16_len);
                    char* str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
                    if (str != NULL && col < col_count) cells[col] = cxlsb_cell_text(str);
                    if (str != NULL) free(str);
                    free(utf16_buf);
                }
            }
        } else if (type == BIFF12_BRT_CELL_BOOL && cells != NULL) {
            uint32_t row = cxlsb_buffer_reader_read_int(sheet_stream);
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_int(sheet_stream);
            uint8_t val = cxlsb_buffer_reader_read_byte(sheet_stream);
            if (col < col_count) cells[col] = cxlsb_cell_bool(val != 0);
        } else {
            cxlsb_buffer_reader_set_position(sheet_stream, cxlsb_buffer_reader_get_position(sheet_stream) + size);
        }
        
        if (type == BIFF12_BRT_ROW_HDR || type == BIFF12_BRT_END_SHEET_DATA) {
            if (cells != NULL && col_count > 0) {
                handler(current_row, col_count, cells, user_data);
                for (int i = 0; i < col_count; i++) {
                    if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                        free(cells[i].value.text);
                    }
                }
                free(cells);
                cells = NULL;
            }
            row_count++;
        }
    }
    
    uint8_t* buf = cxlsb_buffer_reader_get_buffer(sheet_stream);
    cxlsb_buffer_reader_destroy(sheet_stream);
    free(buf);
    return CXLSB_OK;
}