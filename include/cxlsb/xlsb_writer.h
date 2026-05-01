#ifndef CXLSB_XLSB_WRITER_H
#define CXLSB_XLSB_WRITER_H

#include "types.h"
#include "error_codes.h"

typedef struct cxlsb_writer cxlsb_writer_t;

cxlsb_writer_t* cxlsb_writer_create(const char* path);
void cxlsb_writer_destroy(cxlsb_writer_t* writer);

cxlsb_error_t cxlsb_writer_write_batch(cxlsb_writer_t* writer, const char* sheet_name, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count, int col_count);

cxlsb_error_t cxlsb_writer_start_sheet(cxlsb_writer_t* writer, const char* sheet_name, int col_count);
cxlsb_error_t cxlsb_writer_write_rows(cxlsb_writer_t* writer, int start_row, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count);
cxlsb_error_t cxlsb_writer_end_sheet(cxlsb_writer_t* writer);

int cxlsb_writer_get_sheet_count(cxlsb_writer_t* writer);
cxlsb_error_t cxlsb_writer_close(cxlsb_writer_t* writer);

int cxlsb_writer_add_format(cxlsb_writer_t* writer, const char* format_string);
int cxlsb_writer_add_style(cxlsb_writer_t* writer, int num_fmt_id);

typedef struct cxlsb_sheet_data_entry {
    uint8_t* data;
    size_t length;
    char* name;
} cxlsb_sheet_data_entry_t;

#endif /* CXLSB_XLSB_WRITER_H */