#ifndef CXLSB_XLSB_READER_H
#define CXLSB_XLSB_READER_H

#include "types.h"
#include "error_codes.h"

typedef struct cxlsb_reader cxlsb_reader_t;

cxlsb_reader_t* cxlsb_reader_create(const char* path);
void cxlsb_reader_destroy(cxlsb_reader_t* reader);
int cxlsb_reader_get_sheet_count(cxlsb_reader_t* reader);
cxlsb_error_t cxlsb_reader_for_each_row(cxlsb_reader_t* reader, int sheet_index, cxlsb_row_handler_fn handler, void* user_data);

#endif /* CXLSB_XLSB_READER_H */