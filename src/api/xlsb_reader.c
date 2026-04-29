#include "cxlsb/xlsb_reader.h"
#include "cxlsb/cell_data.h"
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
    cxlsb_buffer_reader_t* stream;
    bool streaming_mode;
    bool end_of_data;
    uint32_t current_row;
    int col_count;
    uint32_t first_col;
    cxlsb_cell_data_t* pending_cells;
    bool cells_ready;
};

cxlsb_reader_t* cxlsb_reader_create(const char* path) {
    if (path == NULL) return NULL;
    
    cxlsb_reader_t* reader = (cxlsb_reader_t*)malloc(sizeof(cxlsb_reader_t));
    if (reader == NULL) return NULL;
    
    reader->container = cxlsb_container_reader_create(path);
    if (reader->container == NULL) { free(reader); return NULL; }
    
    reader->sst = NULL;
    reader->sheet_count = cxlsb_container_reader_get_sheet_count(reader->container);
    reader->stream = NULL;
    reader->streaming_mode = false;
    reader->current_row = 0;
    reader->col_count = 0;
    reader->first_col = 0;
    reader->pending_cells = NULL;
    reader->cells_ready = false;
    
    if (cxlsb_container_reader_has_sst(reader->container)) {
        cxlsb_buffer_reader_t* sst_stream = cxlsb_container_reader_get_sst_stream(reader->container);
        if (sst_stream != NULL) {
            reader->sst = cxlsb_sst_table_create(1024);
            if (reader->sst != NULL) {
                while (cxlsb_buffer_reader_has_remaining(sst_stream, 2)) {
                    uint32_t type, size;
                    size_t bytes_read;
                    const uint8_t* buf = cxlsb_buffer_reader_get_buffer(sst_stream);
                    size_t pos = cxlsb_buffer_reader_get_position(sst_stream);
                    cxlsb_varint_decode_at(buf + pos, &type, &bytes_read);
                    cxlsb_buffer_reader_set_position(sst_stream, pos + bytes_read);
                    pos = cxlsb_buffer_reader_get_position(sst_stream);
                    cxlsb_varint_decode_at(buf + pos, &size, &bytes_read);
                    cxlsb_buffer_reader_set_position(sst_stream, pos + bytes_read);
                    
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
            cxlsb_buffer_reader_destroy(sst_stream);
        }
    }
    return reader;
}

void cxlsb_reader_destroy(cxlsb_reader_t* reader) {
    if (reader != NULL) {
        if (reader->streaming_mode && reader->stream != NULL) {
            cxlsb_buffer_reader_destroy(reader->stream);
        }
        if (reader->pending_cells != NULL) {
            for (int i = 0; i < reader->col_count; i++) {
                if (reader->pending_cells[i].type == CXLSB_CELL_TEXT && 
                    reader->pending_cells[i].value.text != NULL) {
                    free((void*)reader->pending_cells[i].value.text);
                }
            }
            free(reader->pending_cells);
        }
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
    uint32_t first_col = 0;
    cxlsb_cell_data_t* cells = NULL;
    bool cells_ready = false;
    
    while (cxlsb_buffer_reader_has_remaining(sheet_stream, 2)) {
        uint32_t type, size;
        size_t bytes_read;
        const uint8_t* buf = cxlsb_buffer_reader_get_buffer(sheet_stream);
        size_t pos = cxlsb_buffer_reader_get_position(sheet_stream);
        cxlsb_varint_decode_at(buf + pos, &type, &bytes_read);
        cxlsb_buffer_reader_set_position(sheet_stream, pos + bytes_read);
        pos = cxlsb_buffer_reader_get_position(sheet_stream);
        cxlsb_varint_decode_at(buf + pos, &size, &bytes_read);
        cxlsb_buffer_reader_set_position(sheet_stream, pos + bytes_read);
        
        if (type == BIFF12_BRT_END_SHEET_DATA) {
            if (cells_ready && cells != NULL && col_count > 0) {
                handler(current_row, col_count, cells, user_data);
                for (int i = 0; i < col_count; i++) {
                    if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                        free((void*)cells[i].value.text);
                    }
                }
                free(cells);
                cells = NULL;
            }
            break;
        }
        if (type == BIFF12_BRT_BEGIN_SHEET_DATA) continue;
        
        if (type == BIFF12_BRT_ROW_HDR) {
            if (cells != NULL) {
                if (cells_ready && col_count > 0) {
                    handler(current_row, col_count, cells, user_data);
                }
                for (int i = 0; i < col_count; i++) {
                    if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                        free((void*)cells[i].value.text);
                    }
                }
                free(cells);
                cells = NULL;
            }
            cells_ready = false;
            
            current_row = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_int(sheet_stream);  // ixfe
            cxlsb_buffer_reader_read_short(sheet_stream); // miyRw
            cxlsb_buffer_reader_read_byte(sheet_stream);  // flags byte 1
            cxlsb_buffer_reader_read_byte(sheet_stream);  // flags byte 2
            cxlsb_buffer_reader_read_byte(sheet_stream);  // flags byte 3
            uint32_t ccolspan = cxlsb_buffer_reader_read_int(sheet_stream);
            first_col = 0;
            uint32_t last_col = 0;
            if (ccolspan > 0) {
                first_col = cxlsb_buffer_reader_read_int(sheet_stream);
                last_col = cxlsb_buffer_reader_read_int(sheet_stream);
            }
            col_count = last_col - first_col + 1;
            cells = (cxlsb_cell_data_t*)malloc(sizeof(cxlsb_cell_data_t) * col_count);
            if (!cells) {
                cxlsb_buffer_reader_destroy(sheet_stream);
                return CXLSB_ERR_NO_MEMORY;
            }
            for (int i = 0; i < col_count; i++) cells[i] = cxlsb_cell_blank();
        } else if (type == BIFF12_BRT_CELL_REAL && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            double val = cxlsb_buffer_reader_read_double(sheet_stream);
            if (col >= first_col && col - first_col < (uint32_t)col_count) 
                cells[col - first_col] = cxlsb_cell_number(val);
        } else if (type == BIFF12_BRT_CELL_ISST && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint32_t sst_idx = cxlsb_buffer_reader_read_int(sheet_stream);
            if (col >= first_col && col - first_col < (uint32_t)col_count && reader->sst != NULL) {
                const char* str = cxlsb_sst_table_get_string(reader->sst, sst_idx);
                if (str != NULL) {
                    char* copy = strdup(str);
                    cxlsb_cell_data_t cell = cxlsb_cell_text(copy);
                    cells[col - first_col] = cell;
                }
            }
        } else if (type == BIFF12_BRT_CELL_ST && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint32_t str_len = cxlsb_buffer_reader_read_int(sheet_stream);
            if (str_len > 0) {
                size_t utf16_len = str_len * 2;
                uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
                if (utf16_buf != NULL) {
                    cxlsb_buffer_reader_read_bytes(sheet_stream, utf16_buf, utf16_len);
                    char* str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
                    if (str != NULL && col >= first_col && col - first_col < (uint32_t)col_count) 
                        cells[col - first_col] = cxlsb_cell_text(str);
                    free(utf16_buf);
                }
            }
        } else if (type == BIFF12_BRT_CELL_BOOL && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint8_t val = cxlsb_buffer_reader_read_byte(sheet_stream);
            if (col >= first_col && col - first_col < (uint32_t)col_count) 
                cells[col - first_col] = cxlsb_cell_bool(val != 0);
        } else {
            cxlsb_buffer_reader_set_position(sheet_stream, cxlsb_buffer_reader_get_position(sheet_stream) + size);
        }
        row_count++;
    }
    
    cxlsb_buffer_reader_destroy(sheet_stream);
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_reader_read_page(cxlsb_reader_t* reader, int sheet_index,
                                      int start_row, int row_count,
                                      cxlsb_row_handler_fn handler, void* user_data) {
    if (reader == NULL || handler == NULL) return CXLSB_ERR_NULL_PTR;
    if (sheet_index < 0 || (uint32_t)sheet_index >= reader->sheet_count) return CXLSB_ERR_INVALID_SHEET_INDEX;
    if (start_row < 0 || row_count <= 0) return CXLSB_ERR_INVALID_ARG;
    
    cxlsb_buffer_reader_t* sheet_stream = cxlsb_container_reader_get_sheet_stream(reader->container, sheet_index);
    if (sheet_stream == NULL) return CXLSB_ERR_ZIP_ENTRY_NOT_FOUND;
    
    uint32_t current_row = 0;
    int col_count = 0;
    uint32_t first_col = 0;
    cxlsb_cell_data_t* cells = NULL;
    bool cells_ready = false;
    int rows_processed = 0;
    
    while (cxlsb_buffer_reader_has_remaining(sheet_stream, 2) && rows_processed < row_count) {
        uint32_t type, size;
        size_t bytes_read;
        const uint8_t* buf = cxlsb_buffer_reader_get_buffer(sheet_stream);
        size_t pos = cxlsb_buffer_reader_get_position(sheet_stream);
        cxlsb_varint_decode_at(buf + pos, &type, &bytes_read);
        cxlsb_buffer_reader_set_position(sheet_stream, pos + bytes_read);
        pos = cxlsb_buffer_reader_get_position(sheet_stream);
        cxlsb_varint_decode_at(buf + pos, &size, &bytes_read);
        cxlsb_buffer_reader_set_position(sheet_stream, pos + bytes_read);
        
        if (type == BIFF12_BRT_END_SHEET_DATA) {
            if (cells != NULL) {
                if (cells_ready && col_count > 0) {
                    if (current_row >= (uint32_t)start_row && current_row < (uint32_t)(start_row + row_count)) {
                        handler(current_row, col_count, cells, user_data);
                        rows_processed++;
                    }
                }
                for (int i = 0; i < col_count; i++) {
                    if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                        free((void*)cells[i].value.text);
                    }
                }
                free(cells);
                cells = NULL;
            }
            break;
        }
        if (type == BIFF12_BRT_BEGIN_SHEET_DATA) continue;
        
        if (type == BIFF12_BRT_ROW_HDR) {
            if (cells != NULL) {
                if (cells_ready && col_count > 0) {
                    if (current_row >= (uint32_t)start_row && current_row < (uint32_t)(start_row + row_count)) {
                        handler(current_row, col_count, cells, user_data);
                        rows_processed++;
                    } else {
                        for (int i = 0; i < col_count; i++) {
                            if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                                free((void*)cells[i].value.text);
                            }
                        }
                    }
                }
                free(cells);
                cells = NULL;
            }
            cells_ready = false;
            
            current_row = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_short(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint32_t ccolspan = cxlsb_buffer_reader_read_int(sheet_stream);
            first_col = 0;
            uint32_t last_col = 0;
            if (ccolspan > 0) {
                first_col = cxlsb_buffer_reader_read_int(sheet_stream);
                last_col = cxlsb_buffer_reader_read_int(sheet_stream);
            }
            col_count = last_col - first_col + 1;
            cells = (cxlsb_cell_data_t*)malloc(sizeof(cxlsb_cell_data_t) * col_count);
            if (!cells) {
                cxlsb_buffer_reader_destroy(sheet_stream);
                return CXLSB_ERR_NO_MEMORY;
            }
            for (int i = 0; i < col_count; i++) cells[i] = cxlsb_cell_blank();
        } else if (type == BIFF12_BRT_CELL_REAL && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            double val = cxlsb_buffer_reader_read_double(sheet_stream);
            if (col >= first_col && col - first_col < (uint32_t)col_count) 
                cells[col - first_col] = cxlsb_cell_number(val);
        } else if (type == BIFF12_BRT_CELL_ISST && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint32_t sst_idx = cxlsb_buffer_reader_read_int(sheet_stream);
            if (col >= first_col && col - first_col < (uint32_t)col_count && reader->sst != NULL) {
                const char* str = cxlsb_sst_table_get_string(reader->sst, sst_idx);
                if (str != NULL) {
                    char* copy = strdup(str);
                    cells[col - first_col] = cxlsb_cell_text(copy);
                }
            }
        } else if (type == BIFF12_BRT_CELL_ST && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint32_t str_len = cxlsb_buffer_reader_read_int(sheet_stream);
            if (str_len > 0) {
                size_t utf16_len = str_len * 2;
                uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
                if (utf16_buf != NULL) {
                    cxlsb_buffer_reader_read_bytes(sheet_stream, utf16_buf, utf16_len);
                    char* str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
                    if (str != NULL && col >= first_col && col - first_col < (uint32_t)col_count) 
                        cells[col - first_col] = cxlsb_cell_text(str);
                    free(utf16_buf);
                }
            }
        } else if (type == BIFF12_BRT_CELL_BOOL && cells != NULL) {
            cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            cxlsb_buffer_reader_read_byte(sheet_stream);
            uint8_t val = cxlsb_buffer_reader_read_byte(sheet_stream);
            if (col >= first_col && col - first_col < (uint32_t)col_count) 
                cells[col - first_col] = cxlsb_cell_bool(val != 0);
        } else {
            cxlsb_buffer_reader_set_position(sheet_stream, cxlsb_buffer_reader_get_position(sheet_stream) + size);
        }
    }
    
    if (cells != NULL) {
        for (int i = 0; i < col_count; i++) {
            if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                free((void*)cells[i].value.text);
            }
        }
        free(cells);
    }
    
    cxlsb_buffer_reader_destroy(sheet_stream);
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_reader_start_streaming(cxlsb_reader_t* reader, int sheet_index) {
    if (reader == NULL) return CXLSB_ERR_NULL_PTR;
    if (sheet_index < 0 || (uint32_t)sheet_index >= reader->sheet_count) return CXLSB_ERR_INVALID_SHEET_INDEX;
    if (reader->streaming_mode) return CXLSB_ERR_INVALID_ARG;
    
    reader->stream = cxlsb_container_reader_get_sheet_stream(reader->container, sheet_index);
    if (reader->stream == NULL) return CXLSB_ERR_ZIP_ENTRY_NOT_FOUND;
    
    reader->streaming_mode = true;
    reader->end_of_data = false;
    reader->current_row = 0;
    reader->col_count = 0;
    reader->first_col = 0;
    reader->pending_cells = NULL;
    reader->cells_ready = false;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_reader_read_batch(cxlsb_reader_t* reader, int batch_size,
                                       cxlsb_row_handler_fn handler, void* user_data) {
    if (reader == NULL || handler == NULL) return CXLSB_ERR_NULL_PTR;
    if (!reader->streaming_mode || reader->stream == NULL) return CXLSB_ERR_INVALID_ARG;
    if (batch_size <= 0) return CXLSB_ERR_INVALID_ARG;
    
    int rows_processed = 0;
    
    while (cxlsb_buffer_reader_has_remaining(reader->stream, 2) && rows_processed < batch_size) {
        uint32_t type, size;
        size_t bytes_read;
        const uint8_t* buf = cxlsb_buffer_reader_get_buffer(reader->stream);
        size_t pos = cxlsb_buffer_reader_get_position(reader->stream);
        cxlsb_varint_decode_at(buf + pos, &type, &bytes_read);
        cxlsb_buffer_reader_set_position(reader->stream, pos + bytes_read);
        pos = cxlsb_buffer_reader_get_position(reader->stream);
        cxlsb_varint_decode_at(buf + pos, &size, &bytes_read);
        cxlsb_buffer_reader_set_position(reader->stream, pos + bytes_read);
        
        if (type == BIFF12_BRT_END_SHEET_DATA) {
            if (reader->pending_cells != NULL) {
                if (reader->cells_ready && reader->col_count > 0) {
                    handler(reader->current_row, reader->col_count, reader->pending_cells, user_data);
                    rows_processed++;
                    for (int i = 0; i < reader->col_count; i++) {
                        if (reader->pending_cells[i].type == CXLSB_CELL_TEXT && 
                            reader->pending_cells[i].value.text != NULL) {
                            free((void*)reader->pending_cells[i].value.text);
                        }
                    }
                }
                free(reader->pending_cells);
                reader->pending_cells = NULL;
            }
            reader->cells_ready = false;
            reader->end_of_data = true;
            break;
        }
        if (type == BIFF12_BRT_BEGIN_SHEET_DATA) continue;
        
        if (type == BIFF12_BRT_ROW_HDR) {
            if (reader->pending_cells != NULL) {
                if (reader->cells_ready && reader->col_count > 0) {
                    handler(reader->current_row, reader->col_count, reader->pending_cells, user_data);
                    rows_processed++;
                }
                for (int i = 0; i < reader->col_count; i++) {
                    if (reader->pending_cells[i].type == CXLSB_CELL_TEXT && 
                        reader->pending_cells[i].value.text != NULL) {
                        free((void*)reader->pending_cells[i].value.text);
                    }
                }
                free(reader->pending_cells);
                reader->pending_cells = NULL;
            }
            reader->cells_ready = false;
            
            reader->current_row = cxlsb_buffer_reader_read_int(reader->stream);
            cxlsb_buffer_reader_read_int(reader->stream);
            cxlsb_buffer_reader_read_short(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            uint32_t ccolspan = cxlsb_buffer_reader_read_int(reader->stream);
            reader->first_col = 0;
            uint32_t last_col = 0;
            if (ccolspan > 0) {
                reader->first_col = cxlsb_buffer_reader_read_int(reader->stream);
                last_col = cxlsb_buffer_reader_read_int(reader->stream);
            }
            reader->col_count = last_col - reader->first_col + 1;
            reader->pending_cells = (cxlsb_cell_data_t*)malloc(sizeof(cxlsb_cell_data_t) * reader->col_count);
            if (!reader->pending_cells) return CXLSB_ERR_NO_MEMORY;
            for (int i = 0; i < reader->col_count; i++) reader->pending_cells[i] = cxlsb_cell_blank();
        } else if (type == BIFF12_BRT_CELL_REAL && reader->pending_cells != NULL) {
            reader->cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            double val = cxlsb_buffer_reader_read_double(reader->stream);
            if (col >= reader->first_col && col - reader->first_col < (uint32_t)reader->col_count) 
                reader->pending_cells[col - reader->first_col] = cxlsb_cell_number(val);
        } else if (type == BIFF12_BRT_CELL_ISST && reader->pending_cells != NULL) {
            reader->cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            uint32_t sst_idx = cxlsb_buffer_reader_read_int(reader->stream);
            if (col >= reader->first_col && col - reader->first_col < (uint32_t)reader->col_count && reader->sst != NULL) {
                const char* str = cxlsb_sst_table_get_string(reader->sst, sst_idx);
                if (str != NULL) {
                    char* copy = strdup(str);
                    reader->pending_cells[col - reader->first_col] = cxlsb_cell_text(copy);
                }
            }
        } else if (type == BIFF12_BRT_CELL_ST && reader->pending_cells != NULL) {
            reader->cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            uint32_t str_len = cxlsb_buffer_reader_read_int(reader->stream);
            if (str_len > 0) {
                size_t utf16_len = str_len * 2;
                uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
                if (utf16_buf != NULL) {
                    cxlsb_buffer_reader_read_bytes(reader->stream, utf16_buf, utf16_len);
                    char* str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
                    if (str != NULL && col >= reader->first_col && col - reader->first_col < (uint32_t)reader->col_count) 
                        reader->pending_cells[col - reader->first_col] = cxlsb_cell_text(str);
                    free(utf16_buf);
                }
            }
        } else if (type == BIFF12_BRT_CELL_BOOL && reader->pending_cells != NULL) {
            reader->cells_ready = true;
            uint32_t col = cxlsb_buffer_reader_read_int(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            cxlsb_buffer_reader_read_byte(reader->stream);
            uint8_t val = cxlsb_buffer_reader_read_byte(reader->stream);
            if (col >= reader->first_col && col - reader->first_col < (uint32_t)reader->col_count) 
                reader->pending_cells[col - reader->first_col] = cxlsb_cell_bool(val != 0);
        } else {
            cxlsb_buffer_reader_set_position(reader->stream, cxlsb_buffer_reader_get_position(reader->stream) + size);
        }
    }
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_reader_end_streaming(cxlsb_reader_t* reader) {
    if (reader == NULL) return CXLSB_ERR_NULL_PTR;
    if (!reader->streaming_mode) return CXLSB_ERR_INVALID_ARG;
    
    if (reader->pending_cells != NULL) {
        for (int i = 0; i < reader->col_count; i++) {
            if (reader->pending_cells[i].type == CXLSB_CELL_TEXT && 
                reader->pending_cells[i].value.text != NULL) {
                free((void*)reader->pending_cells[i].value.text);
            }
        }
        free(reader->pending_cells);
        reader->pending_cells = NULL;
    }
    
    if (reader->stream != NULL) {
        cxlsb_buffer_reader_destroy(reader->stream);
        reader->stream = NULL;
    }
    
    reader->streaming_mode = false;
    return CXLSB_OK;
}

int cxlsb_reader_get_current_row(cxlsb_reader_t* reader) {
    if (reader == NULL || !reader->streaming_mode) return -1;
    return (int)reader->current_row;
}

bool cxlsb_reader_has_more_rows(cxlsb_reader_t* reader) {
    if (reader == NULL || !reader->streaming_mode || reader->stream == NULL || reader->end_of_data) return false;
    return cxlsb_buffer_reader_has_remaining(reader->stream, 2);
}