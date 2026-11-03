#include "limits.h"
#include <stddef.h>

static cxlsb_overflow_behavior_t g_column_behavior = CXLSB_OVERFLOW_ERROR;
static cxlsb_overflow_behavior_t g_row_behavior = CXLSB_OVERFLOW_ERROR;

void cxlsb_set_column_overflow(cxlsb_overflow_behavior_t behavior) {
    g_column_behavior = (behavior == CXLSB_OVERFLOW_CUT)
        ? CXLSB_OVERFLOW_CUT : CXLSB_OVERFLOW_ERROR;
}

void cxlsb_set_row_overflow(cxlsb_overflow_behavior_t behavior) {
    g_row_behavior = (behavior == CXLSB_OVERFLOW_CUT)
        ? CXLSB_OVERFLOW_CUT : CXLSB_OVERFLOW_ERROR;
}

void cxlsb_set_overflow(cxlsb_overflow_behavior_t columns, cxlsb_overflow_behavior_t rows) {
    cxlsb_set_column_overflow(columns);
    cxlsb_set_row_overflow(rows);
}

cxlsb_overflow_behavior_t cxlsb_get_column_overflow(void) {
    return g_column_behavior;
}

cxlsb_overflow_behavior_t cxlsb_get_row_overflow(void) {
    return g_row_behavior;
}

cxlsb_error_t cxlsb_sheet_limits_begin(int requested_columns, cxlsb_sheet_limits_t* out) {
    if (out == NULL) return CXLSB_ERR_NULL_PTR;
    if (requested_columns < 0) return CXLSB_ERR_INVALID_ARG;

    if (requested_columns > CXLSB_MAX_COLUMNS) {
        if (g_column_behavior == CXLSB_OVERFLOW_CUT) {
            requested_columns = CXLSB_MAX_COLUMNS;
        } else {
            return CXLSB_ERR_LIMIT_EXCEEDED;
        }
    }

    out->columns = requested_columns;
    out->row_behavior = g_row_behavior;
    return CXLSB_OK;
}

int cxlsb_sheet_limits_columns(const cxlsb_sheet_limits_t* limits) {
    return limits != NULL ? limits->columns : 0;
}

cxlsb_error_t cxlsb_sheet_limits_rows(const cxlsb_sheet_limits_t* limits,
                                      int start_row,
                                      int requested_rows,
                                      int* out_rows) {
    if (limits == NULL || out_rows == NULL) return CXLSB_ERR_NULL_PTR;
    if (start_row < 0 || requested_rows < 0) return CXLSB_ERR_INVALID_ARG;

    if (start_row >= CXLSB_MAX_ROWS) {
        if (limits->row_behavior == CXLSB_OVERFLOW_CUT) {
            *out_rows = 0;
            return CXLSB_OK;
        }
        return CXLSB_ERR_LIMIT_EXCEEDED;
    }

    if (start_row + requested_rows > CXLSB_MAX_ROWS) {
        if (limits->row_behavior == CXLSB_OVERFLOW_CUT) {
            *out_rows = CXLSB_MAX_ROWS - start_row;
            return CXLSB_OK;
        }
        return CXLSB_ERR_LIMIT_EXCEEDED;
    }

    *out_rows = requested_rows;
    return CXLSB_OK;
}
