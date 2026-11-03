#ifndef CXLSB_TEMPLATE_FILLER_H
#define CXLSB_TEMPLATE_FILLER_H

#include "types.h"
#include "error_codes.h"

typedef struct cxlsb_fill_result cxlsb_fill_result_t;

struct cxlsb_fill_result {
    int start_row;
    int start_col;
    int row_count;
    int col_count;
    int* sst_indices;    // SST indices for text cells [row_count * col_count]
    int* xf_indices;     // Style indices for filled cells
    int* cell_types;     // cxlsb_cell_type_t for each filled cell
    double* numbers;     // 数值/日期/布尔值（布尔用 0/1）
};

typedef struct cxlsb_template_filler cxlsb_template_filler_t;

cxlsb_template_filler_t* cxlsb_template_filler_create(const char* template_path);

void cxlsb_template_filler_destroy(cxlsb_template_filler_t* filler);

cxlsb_error_t cxlsb_template_filler_fill_index(cxlsb_template_filler_t* filler,
                                                int sheet_index,
                                                int start_row,
                                                int start_col,
                                                cxlsb_cell_supplier_fn supplier,
                                                void* user_data,
                                                int row_count,
                                                int col_count);

cxlsb_error_t cxlsb_template_filler_fill_marker(cxlsb_template_filler_t* filler,
                                                 int sheet_index,
                                                 const char* marker,
                                                 cxlsb_cell_supplier_fn supplier,
                                                 void* user_data,
                                                 int row_count,
                                                 int col_count);

cxlsb_error_t cxlsb_template_filler_save(cxlsb_template_filler_t* filler,
                                          const char* output_path);

int cxlsb_template_filler_get_sheet_count(cxlsb_template_filler_t* filler);

const char* cxlsb_template_filler_get_sheet_name(cxlsb_template_filler_t* filler, int sheet_index);

int cxlsb_template_filler_find_marker_row(cxlsb_template_filler_t* filler,
                                           int sheet_index,
                                           const char* marker);

int cxlsb_template_filler_find_marker_col(cxlsb_template_filler_t* filler,
                                           int sheet_index,
                                           const char* marker);

#endif /* CXLSB_TEMPLATE_FILLER_H */