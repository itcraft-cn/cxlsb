#include "cxlsb/cell_data.h"

cxlsb_cell_data_t cxlsb_cell_blank(void) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_BLANK;
    cell.value.number = 0.0;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_number(double value) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_NUMBER;
    cell.value.number = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_text(const char* text) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_TEXT;
    cell.value.text = text;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_bool(bool value) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_BOOL;
    cell.value.boolean = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_date(double excel_date) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_DATE;
    cell.value.number = excel_date;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_error(int error_code) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_ERROR;
    cell.value.error_code = error_code;
    return cell;
}

double cxlsb_date_from_timestamp(uint64_t timestamp) {
    return (double)(timestamp / 86400000.0) + 25569.0;
}

uint64_t cxlsb_date_to_timestamp(double excel_date) {
    return (uint64_t)((excel_date - 25569.0) * 86400000.0);
}