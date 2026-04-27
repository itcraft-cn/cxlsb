#ifndef CXLSB_CELL_DATA_H
#define CXLSB_CELL_DATA_H

#include "types.h"

cxlsb_cell_data_t cxlsb_cell_blank(void);
cxlsb_cell_data_t cxlsb_cell_number(double value);
cxlsb_cell_data_t cxlsb_cell_text(const char* text);
cxlsb_cell_data_t cxlsb_cell_bool(bool value);
cxlsb_cell_data_t cxlsb_cell_date(double excel_date);
cxlsb_cell_data_t cxlsb_cell_error(int error_code);

double cxlsb_date_from_timestamp(uint64_t timestamp);
uint64_t cxlsb_date_to_timestamp(double excel_date);

#endif /* CXLSB_CELL_DATA_H */