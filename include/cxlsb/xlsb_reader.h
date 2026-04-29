#ifndef CXLSB_XLSB_READER_H
#define CXLSB_XLSB_READER_H

#include "types.h"
#include "error_codes.h"

typedef struct cxlsb_reader cxlsb_reader_t;

cxlsb_reader_t* cxlsb_reader_create(const char* path);
void cxlsb_reader_destroy(cxlsb_reader_t* reader);
int cxlsb_reader_get_sheet_count(cxlsb_reader_t* reader);

cxlsb_error_t cxlsb_reader_for_each_row(cxlsb_reader_t* reader, int sheet_index, 
                                         cxlsb_row_handler_fn handler, void* user_data);

cxlsb_error_t cxlsb_reader_read_page(cxlsb_reader_t* reader, int sheet_index,
                                      int start_row, int row_count,
                                      cxlsb_row_handler_fn handler, void* user_data);

cxlsb_error_t cxlsb_reader_start_streaming(cxlsb_reader_t* reader, int sheet_index);
cxlsb_error_t cxlsb_reader_read_batch(cxlsb_reader_t* reader, int batch_size,
                                       cxlsb_row_handler_fn handler, void* user_data);
cxlsb_error_t cxlsb_reader_end_streaming(cxlsb_reader_t* reader);
int cxlsb_reader_get_current_row(cxlsb_reader_t* reader);
bool cxlsb_reader_has_more_rows(cxlsb_reader_t* reader);

#endif /* CXLSB_XLSB_READER_H */