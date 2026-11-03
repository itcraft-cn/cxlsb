#ifndef CXLSB_SHEET_WRITER_H
#define CXLSB_SHEET_WRITER_H

#include "sst_table.h"
#include "styles_registry.h"
#include "biff12_writer.h"
#include "limits.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_sheet_writer {
    cxlsb_biff12_writer_t* biff12;
    cxlsb_sst_table_t* sst;
    cxlsb_styles_registry_t* styles;
    int col_count;
    int max_row;
    int max_col;
    bool streaming_mode;
    size_t dim_last_row_offset;
    cxlsb_sheet_limits_t streaming_limits;
} cxlsb_sheet_writer_t;

cxlsb_sheet_writer_t* cxlsb_sheet_writer_create(cxlsb_sst_table_t* sst, cxlsb_styles_registry_t* styles);
void cxlsb_sheet_writer_destroy(cxlsb_sheet_writer_t* writer);
cxlsb_error_t cxlsb_sheet_writer_write_sheet(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count, int col_count, uint8_t** out_data, size_t* out_length);
cxlsb_error_t cxlsb_sheet_writer_start_streaming(cxlsb_sheet_writer_t* writer, int col_count);
cxlsb_error_t cxlsb_sheet_writer_append_rows(cxlsb_sheet_writer_t* writer, cxlsb_cell_supplier_fn supplier, void* user_data, int start_row, int row_count);
cxlsb_error_t cxlsb_sheet_writer_finalize_streaming(cxlsb_sheet_writer_t* writer, int row_count, int col_count, uint8_t** out_data, size_t* out_length);

#endif /* CXLSB_SHEET_WRITER_H */