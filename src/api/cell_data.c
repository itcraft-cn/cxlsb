#include "cxlsb/cell_data.h"

cxlsb_cell_data_t cxlsb_cell_blank(void) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_BLANK;
    cell.style_index = 0;
    cell.value.number = 0.0;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_number(double value) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_NUMBER;
    cell.style_index = 0;
    cell.value.number = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_text(const char* text) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_TEXT;
    cell.style_index = 0;
    cell.value.text = text;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_bool(bool value) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_BOOL;
    cell.style_index = 0;
    cell.value.boolean = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_date(double excel_date) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_DATE;
    cell.style_index = 0;
    cell.value.number = excel_date;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_error(int error_code) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_ERROR;
    cell.style_index = 0;
    cell.value.error_code = error_code;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_number_with_style(double value, uint32_t style_index) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_NUMBER;
    cell.style_index = style_index;
    cell.value.number = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_text_with_style(const char* text, uint32_t style_index) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_TEXT;
    cell.style_index = style_index;
    cell.value.text = text;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_date_with_style(double excel_date, uint32_t style_index) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_DATE;
    cell.style_index = style_index;
    cell.value.number = excel_date;
    return cell;
}

double cxlsb_date_from_timestamp(uint64_t timestamp) {
    return (double)(timestamp / 86400000.0) + 25569.0;
}

uint64_t cxlsb_date_to_timestamp(double excel_date) {
    return (uint64_t)((excel_date - 25569.0) * 86400000.0);
}