#include "sheet_writer.h"
#include "biff12_record_types.h"
#include "limits.h"
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
    writer->dim_last_row_offset = 0;
    writer->streaming_limits.columns = 0;
    writer->streaming_limits.row_behavior = CXLSB_OVERFLOW_ERROR;
    return writer;
}

void cxlsb_sheet_writer_destroy(cxlsb_sheet_writer_t* writer) {
    if (writer != NULL) {
        if (writer->biff12 != NULL) cxlsb_biff12_writer_destroy(writer->biff12);
        free(writer);
    }
}

static cxlsb_error_t write_ws_views(cxlsb_biff12_writer_t* biff12) {
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_WS_VIEWS);
    
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BEGIN_WS_VIEW, 30);
    static const uint8_t ws_view_data[] = {
        0xDC, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    cxlsb_buffer_writer_write_bytes(biff12->buffer, ws_view_data, sizeof(ws_view_data));
    
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_SEL, 36);
    static const uint8_t sel_data[] = {
        0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00
    };
    cxlsb_buffer_writer_write_bytes(biff12->buffer, sel_data, sizeof(sel_data));
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_WS_VIEW);
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_WS_VIEWS);
    return CXLSB_OK;
}

static cxlsb_error_t write_ws_fmt_info(cxlsb_biff12_writer_t* biff12) {
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_WS_FMT_INFO, 12);
    static const uint8_t fmt_info_data[] = {
        0x00, 0x09, 0x00, 0x00, 0x08, 0x00, 0x0E, 0x01,
        0x00, 0x00, 0x00, 0x00
    };
    cxlsb_buffer_writer_write_bytes(biff12->buffer, fmt_info_data, sizeof(fmt_info_data));
    return CXLSB_OK;
}

static cxlsb_error_t write_ws_prop(cxlsb_biff12_writer_t* biff12) {
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_WS_PROP, 23);
    static const uint8_t ws_prop_data[] = {
        0xC9, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
    };
    return cxlsb_buffer_writer_write_bytes(biff12->buffer, ws_prop_data, sizeof(ws_prop_data));
}

/* 写入单元格；Blank 不生成记录（与 jxlsb/rxlsb 一致）。 */
static cxlsb_error_t write_cell(cxlsb_sheet_writer_t* writer, int row, int col,
                                const cxlsb_cell_data_t* cell) {
    uint32_t xf_id = cell->style_index;

    switch (cell->type) {
        case CXLSB_CELL_NUMBER:
        case CXLSB_CELL_DATE:
            return cxlsb_biff12_writer_write_cell_number(writer->biff12, (uint32_t)row,
                                                         (uint32_t)col, cell->value.number, xf_id);
        case CXLSB_CELL_TEXT:
            if (writer->sst != NULL) {
                uint32_t sst_idx = cxlsb_sst_table_add_string(writer->sst, cell->value.text);
                return cxlsb_biff12_writer_write_cell_isst(writer->biff12, (uint32_t)row,
                                                           (uint32_t)col, sst_idx, xf_id);
            }
            return cxlsb_biff12_writer_write_cell_st(writer->biff12, (uint32_t)row,
                                                     (uint32_t)col, cell->value.text, xf_id);
        case CXLSB_CELL_BOOL:
            return cxlsb_biff12_writer_write_cell_bool(writer->biff12, (uint32_t)row,
                                                       (uint32_t)col, cell->value.boolean, xf_id);
        default:
            /* CXLSB_CELL_BLANK / 未知类型：跳过，不写 BrtCellBlank。 */
            return CXLSB_OK;
    }
}

static cxlsb_error_t write_rows(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier,
                                void* user_data, int start_row, int row_count, int col_count) {
    if (row_count <= 0 || col_count <= 0) return CXLSB_OK;

    cxlsb_cell_data_t* row_cache = (cxlsb_cell_data_t*)malloc((size_t)col_count * sizeof(cxlsb_cell_data_t));
    if (row_cache == NULL) return CXLSB_ERR_NO_MEMORY;

    for (int row = start_row; row < start_row + row_count; row++) {
        for (int col = 0; col < col_count; col++) {
            row_cache[col] = supplier(row, col, user_data);
        }

        cxlsb_error_t err = cxlsb_biff12_writer_write_row_header(writer->biff12,
                                                                 (uint32_t)row,
                                                                 (uint32_t)col_count);
        if (err != CXLSB_OK) { free(row_cache); return err; }

        for (int col = 0; col < col_count; col++) {
            err = write_cell(writer, row, col, &row_cache[col]);
            if (err != CXLSB_OK) { free(row_cache); return err; }
        }

        if (row > writer->max_row) writer->max_row = row;
    }
    if (col_count - 1 > writer->max_col) writer->max_col = col_count - 1;

    free(row_cache);
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_writer_write_sheet(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count, int col_count, uint8_t** out_data, size_t* out_length) {
    if (writer == NULL || supplier == NULL || out_data == NULL || out_length == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }

    cxlsb_sheet_limits_t limits;
    cxlsb_error_t err = cxlsb_sheet_limits_begin(col_count, &limits);
    if (err != CXLSB_OK) return err;
    int actual_cols = cxlsb_sheet_limits_columns(&limits);

    int actual_rows = 0;
    err = cxlsb_sheet_limits_rows(&limits, 0, row_count, &actual_rows);
    if (err != CXLSB_OK) return err;

    cxlsb_biff12_writer_reset(writer->biff12);
    writer->max_row = 0;
    writer->max_col = 0;

    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET);
    write_ws_prop(writer->biff12);

    uint32_t last_row = actual_rows > 0 ? (uint32_t)(actual_rows - 1) : 0;
    uint32_t last_col = actual_cols > 0 ? (uint32_t)(actual_cols - 1) : 0;
    cxlsb_biff12_writer_write_ws_dim(writer->biff12, 0, last_row, 0, last_col);

    write_ws_views(writer->biff12);
    write_ws_fmt_info(writer->biff12);

    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET_DATA);
    err = write_rows(writer, supplier, user_data, 0, actual_rows, actual_cols);
    if (err != CXLSB_OK) return err;
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET_DATA);

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
    if (writer == NULL) return CXLSB_ERR_NULL_PTR;

    cxlsb_sheet_limits_t limits;
    cxlsb_error_t err = cxlsb_sheet_limits_begin(col_count, &limits);
    if (err != CXLSB_OK) return err;
    col_count = cxlsb_sheet_limits_columns(&limits);

    writer->streaming_mode = true;
    writer->col_count = col_count;
    writer->streaming_limits = limits;
    writer->max_row = 0;
    writer->max_col = col_count > 0 ? col_count - 1 : 0;
    cxlsb_biff12_writer_reset(writer->biff12);
    
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET);
    write_ws_prop(writer->biff12);

    /*
     * 前导先写维度，last_row 先置 0；结束时按偏移回填，
     * 避免结束阶段整块拷贝行数据。
     * BrtWsDim 偏移 = type varint(2) + varsize(1) + first_row(4)。
     */
    size_t dim_offset = cxlsb_biff12_writer_get_length(writer->biff12);
    cxlsb_biff12_writer_write_ws_dim(writer->biff12, 0, 0, 0,
                                     col_count > 0 ? (uint32_t)(col_count - 1) : 0);
    writer->dim_last_row_offset = dim_offset + 2 + 1 + 4;

    write_ws_views(writer->biff12);
    write_ws_fmt_info(writer->biff12);
    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_BEGIN_SHEET_DATA);
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_writer_append_rows(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int start_row, int row_count) {
    if (writer == NULL || supplier == NULL) return CXLSB_ERR_NULL_PTR;
    if (!writer->streaming_mode) return CXLSB_ERR_INVALID_ARG;

    int actual_rows = 0;
    cxlsb_error_t err = cxlsb_sheet_limits_rows(&writer->streaming_limits, start_row,
                                                row_count, &actual_rows);
    if (err != CXLSB_OK) return err;
    if (actual_rows == 0) return CXLSB_OK;

    return write_rows(writer, supplier, user_data, start_row, actual_rows, writer->col_count);
}

cxlsb_error_t cxlsb_sheet_writer_finalize_streaming(cxlsb_sheet_writer_t* writer, int row_count, int col_count, uint8_t** out_data, size_t* out_length) {
    if (writer == NULL || out_data == NULL || out_length == NULL) return CXLSB_ERR_NULL_PTR;
    if (!writer->streaming_mode) return CXLSB_ERR_INVALID_ARG;

    (void)col_count;

    /* 回填 last_row（无行时保持 0）。 */
    uint32_t last_row = 0;
    if (row_count > 0) {
        last_row = (uint32_t)(row_count - 1);
    } else if (writer->max_row > 0) {
        last_row = (uint32_t)writer->max_row;
    }
    cxlsb_buffer_writer_set_u32_at(writer->biff12->buffer,
                                   writer->dim_last_row_offset,
                                   last_row);

    cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET_DATA);
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
