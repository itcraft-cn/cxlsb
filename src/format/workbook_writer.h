#ifndef CXLSB_WORKBOOK_WRITER_H
#define CXLSB_WORKBOOK_WRITER_H

#include <stdint.h>
#include "biff12_writer.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_workbook_writer {
    char** sheet_names;
    uint32_t sheet_count;
    uint32_t capacity;
} cxlsb_workbook_writer_t;

cxlsb_workbook_writer_t* cxlsb_workbook_writer_create(void);
void cxlsb_workbook_writer_destroy(cxlsb_workbook_writer_t* writer);
cxlsb_error_t cxlsb_workbook_writer_add_sheet(cxlsb_workbook_writer_t* writer, const char* name);
uint32_t cxlsb_workbook_writer_get_count(cxlsb_workbook_writer_t* writer);
cxlsb_error_t cxlsb_workbook_writer_to_bytes(cxlsb_workbook_writer_t* writer, uint8_t** out_data, size_t* out_length);

#endif /* CXLSB_WORKBOOK_WRITER_H */