#ifndef CXLSB_TYPES_H
#define CXLSB_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum cxlsb_cell_type {
    CXLSB_CELL_BLANK = 0,
    CXLSB_CELL_NUMBER = 1,
    CXLSB_CELL_TEXT = 2,
    CXLSB_CELL_BOOL = 3,
    CXLSB_CELL_DATE = 4,
    CXLSB_CELL_ERROR = 5
} cxlsb_cell_type_t;

typedef struct cxlsb_cell_data {
    cxlsb_cell_type_t type;
    uint32_t style_index;
    union {
        double number;
        const char* text;
        bool boolean;
        int error_code;
    } value;
} cxlsb_cell_data_t;

typedef cxlsb_cell_data_t (*cxlsb_cell_supplier_fn)(int row, int col, void* user_data);

typedef void (*cxlsb_row_handler_fn)(int row, int col_count, 
                                      cxlsb_cell_data_t* cells, void* user_data);

typedef void (*cxlsb_cell_handler_fn)(int row, int col, 
                                       cxlsb_cell_data_t* cell, void* user_data);

typedef struct cxlsb_sheet_info {
    int index;
    char* name;
    int row_count;
    int col_count;
} cxlsb_sheet_info_t;

typedef struct cxlsb_cell_style {
    int font_id;
    int fill_id;
    int border_id;
    int num_format_id;
    uint8_t horizontal_align;
    uint8_t vertical_align;
} cxlsb_cell_style_t;

typedef struct cxlsb_merge_cell {
    uint32_t first_row;
    uint32_t first_col;
    uint32_t last_row;
    uint32_t last_col;
} cxlsb_merge_cell_t;

#endif /* CXLSB_TYPES_H */