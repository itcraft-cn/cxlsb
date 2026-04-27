#include "sheet_writer.h"
#include "biff12_record_types.h"
#include "../io/buffer_writer.h"
#include <stdlib.h>
#include <string.h>

cxlsb_sheet_writer_t* cxlsb_sheet_writer_create(cxlsb_sst_table_t* sst, cxlsb_styles_registry_t* styles) {
    cxlsb_sheet_writer_t* writer = (cxlsb_sheet_writer_t*)malloc(sizeof(cxlsb_sheet_writer_t));
    if (writer == NULL) return NULL;
    
    writer->biff12 = cxlsb_biff12_writer_create(64 * 1024);
    if (writer->biff12 == NULL) { free(writer); return NULL; }
    
    writer->sst = sst;
    writer->styles = styles;
    writer->col_count = 0;
    writer->max_row = 0;
    writer->max_col = 0;
    writer->streaming_mode = false;
    return writer;
}

void cxlsb_sheet_writer_destroy(cxlsb_sheet_writer_t* writer) {
    if (writer != NULL) {
        if (writer->biff12 != NULL) cxlsb_biff12_writer_destroy(writer->biff12);
        free(writer);
    }
}

static cxlsb_error_t write_sheet_data(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int start_row, int row_count, int col_count) {
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET_DATA);
    
    for (int row = start_row; row < start_row + row_count; row++) {
        int first_col = -1, last_col = -1;
        for (int col = 0; col < col_count; col++) {
            cxlsb_cell_data_t cell = supplier(row, col, user_data);
            if (cell.type != CXLSB_CELL_BLANK) {
                if (first_col < 0) first_col = col;
                last_col = col;
            }
        }
        
        if (first_col >= 0) {
            cxlsb_biff12_writer_write_row_header(writer->biff12, row, first_col, last_col);
            for (int col = first_col; col <= last_col; col++) {
                cxlsb_cell_data_t cell = supplier(row, col, user_data);
                uint32_t xf_id = 0;
                
                switch (cell.type) {
                    case CXLSB_CELL_NUMBER:
                        cxlsb_biff12_writer_write_cell_real(writer->biff12, row, col, cell.value.number, xf_id);
                        break;
                    case CXLSB_CELL_TEXT:
                        if (writer->sst != NULL && strlen(cell.value.text) > 3) {
                            uint32_t sst_idx = cxlsb_sst_table_add_string(writer->sst, cell.value.text);
                            cxlsb_biff12_writer_write_cell_isst(writer->biff12, row, col, sst_idx, xf_id);
                        } else {
                            cxlsb_biff12_writer_write_cell_st(writer->biff12, row, col, cell.value.text, xf_id);
                        }
                        break;
                    case CXLSB_CELL_BOOL:
                        cxlsb_biff12_writer_write_cell_bool(writer->biff12, row, col, cell.value.boolean, xf_id);
                        break;
                    case CXLSB_CELL_DATE:
                        cxlsb_biff12_writer_write_cell_real(writer->biff12, row, col, cell.value.number, xf_id);
                        break;
                    default:
                        cxlsb_biff12_writer_write_cell_blank(writer->biff12, row, col, xf_id);
                        break;
                }
            }
        }
        if (row > writer->max_row) writer->max_row = row;
        if (col_count > writer->max_col) writer->max_col = col_count - 1;
    }
    return cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET_DATA);
}

cxlsb_error_t cxlsb_sheet_writer_write_sheet(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count, int col_count, uint8_t** out_data, size_t* out_length) {
    cxlsb_biff12_writer_reset(writer->biff12);
    
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET);
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_WS_PROP);
    
    write_sheet_data(writer, supplier, user_data, 0, row_count, col_count);
    
    cxlsb_biff12_writer_write_ws_dim(writer->biff12, 0, 0, row_count > 0 ? row_count - 1 : 0, col_count > 0 ? col_count - 1 : 0);
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET);
    
    size_t len = cxlsb_biff12_writer_get_length(writer->biff12);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) return CXLSB_ERR_NO_MEMORY;
    memcpy(data, cxlsb_biff12_writer_get_buffer(writer->biff12), len);
    
    *out_data = data;
    *out_length = len;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_writer_start_streaming(cxlsb_sheet_writer_t* writer, int col_count) {
    writer->streaming_mode = true;
    writer->col_count = col_count;
    writer->max_row = 0;
    writer->max_col = 0;
    cxlsb_biff12_writer_reset(writer->biff12);
    
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET);
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_WS_PROP);
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_writer_append_rows(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int start_row, int row_count) {
    if (!writer->streaming_mode) return CXLSB_ERR_INVALID_ARG;
    return write_sheet_data(writer, supplier, user_data, start_row, row_count, writer->col_count);
}

cxlsb_error_t cxlsb_sheet_writer_finalize_streaming(cxlsb_sheet_writer_t* writer, int row_count, int col_count, uint8_t** out_data, size_t* out_length) {
    if (!writer->streaming_mode) return CXLSB_ERR_INVALID_ARG;
    
    cxlsb_biff12_writer_write_ws_dim(writer->biff12, 0, 0, row_count > 0 ? row_count - 1 : 0, col_count > 0 ? col_count - 1 : 0);
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET);
    writer->streaming_mode = false;
    
    size_t len = cxlsb_biff12_writer_get_length(writer->biff12);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) return CXLSB_ERR_NO_MEMORY;
    memcpy(data, cxlsb_biff12_writer_get_buffer(writer->biff12), len);
    
    *out_data = data;
    *out_length = len;
    return CXLSB_OK;
}