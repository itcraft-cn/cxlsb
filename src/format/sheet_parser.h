#ifndef CXLSB_SHEET_PARSER_H
#define CXLSB_SHEET_PARSER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int row;
    int col;
    int type;  // 0=blank, 1=text, 2=number, 3=bool
    int style_index;
    union {
        char* text;
        double number;
        int bool_val;
    } data;
} cxlsb_cell_info;

typedef struct {
    int row_first;
    int row_last;
    int col_first;
    int col_last;
} cxlsb_merge_cell;

typedef struct {
    cxlsb_cell_info* cells;
    int cell_count;
    int cell_capacity;
    cxlsb_merge_cell* merges;
    int merge_count;
    int merge_capacity;
    int max_row;
    int max_col;
} cxlsb_sheet_parser;

cxlsb_sheet_parser* cxlsb_sheet_parser_create(const uint8_t* data, size_t len, const char** sst_strings, int sst_count);
void cxlsb_sheet_parser_destroy(cxlsb_sheet_parser* parser);

#endif