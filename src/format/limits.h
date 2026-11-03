#ifndef CXLSB_LIMITS_INTERNAL_H
#define CXLSB_LIMITS_INTERNAL_H

#include "cxlsb/limits.h"
#include "cxlsb/error_codes.h"

/* 一次写入操作内的维度限制快照。 */
typedef struct cxlsb_sheet_limits {
    int columns;
    cxlsb_overflow_behavior_t row_behavior;
} cxlsb_sheet_limits_t;

cxlsb_error_t cxlsb_sheet_limits_begin(int requested_columns, cxlsb_sheet_limits_t* out);

int cxlsb_sheet_limits_columns(const cxlsb_sheet_limits_t* limits);

cxlsb_error_t cxlsb_sheet_limits_rows(const cxlsb_sheet_limits_t* limits,
                                      int start_row,
                                      int requested_rows,
                                      int* out_rows);

#endif /* CXLSB_LIMITS_INTERNAL_H */
