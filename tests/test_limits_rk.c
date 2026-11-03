#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "cxlsb/xlsb_writer.h"
#include "cxlsb/xlsb_reader.h"
#include "cxlsb/cell_data.h"
#include "cxlsb/limits.h"
#include "format/limits.h"

static const char* TEST_FILE = "/tmp/test_limits_rk.xlsb";

static int passed = 0;
static int failed = 0;

static void check(int ok, const char* message) {
    if (ok) {
        passed++;
        printf("  PASSED: %s\n", message);
    } else {
        failed++;
        printf("  FAILED: %s\n", message);
    }
}

static cxlsb_cell_data_t rk_supplier(int row, int col, void* user_data) {
    (void)user_data;
    static const double values[3][3] = {
        { 0.0, -536870912.0, 536870911.0 },
        { 1.5, 123456789.0, -42.0 },
        { 0.25, 536870912.0, 3.141592653589793 }
    };
    if (row < 3 && col < 3) return cxlsb_cell_number(values[row][col]);
    return cxlsb_cell_blank();
}

typedef struct {
    int row_count;
    double values[3][3];
} rk_context;

static void rk_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    rk_context* ctx = (rk_context*)user_data;
    int r = ctx->row_count;
    for (int c = 0; c < col_count && c < 3; c++) {
        if (r < 3 && cells[c].type == CXLSB_CELL_NUMBER) {
            ctx->values[r][c] = cells[c].value.number;
        }
    }
    ctx->row_count++;
}

static int approx(double a, double b) {
    return fabs(a - b) < 1e-9;
}

static cxlsb_cell_data_t wide_supplier(int row, int col, void* user_data) {
    (void)row;
    (void)user_data;
    return cxlsb_cell_number((double)col);
}

typedef struct {
    int row_count;
    int last_col_count;
} wide_context;

static void wide_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    (void)row;
    (void)cells;
    wide_context* ctx = (wide_context*)user_data;
    ctx->row_count++;
    ctx->last_col_count = col_count;
}

int main(void) {
    printf("\n=== Limits / RK Sync Tests ===\n\n");

    /* 1. RK 编码回环 */
    cxlsb_set_overflow(CXLSB_OVERFLOW_ERROR, CXLSB_OVERFLOW_ERROR);
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "RK", rk_supplier, NULL, 3, 3);
    cxlsb_writer_destroy(writer);
    check(err == CXLSB_OK, "write RK/Real mixed values");

    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    check(reader != NULL, "open RK workbook");
    if (reader != NULL) {
        rk_context ctx;
        memset(&ctx, 0, sizeof(ctx));
        err = cxlsb_reader_for_each_row(reader, 0, rk_handler, &ctx);
        check(err == CXLSB_OK && ctx.row_count == 3, "read back 3 rows");
        check(approx(ctx.values[0][0], 0.0) && approx(ctx.values[0][1], -536870912.0)
              && approx(ctx.values[0][2], 536870911.0), "RK boundary values");
        check(approx(ctx.values[1][0], 1.5) && approx(ctx.values[1][1], 123456789.0)
              && approx(ctx.values[1][2], -42.0), "Real / RK mid values");
        check(approx(ctx.values[2][0], 0.25) && approx(ctx.values[2][1], 536870912.0)
              && approx(ctx.values[2][2], 3.141592653589793), "Real high precision");
        cxlsb_reader_destroy(reader);
    }
    unlink(TEST_FILE);

    /* 2. 列上限：Cut 截断、Error 报错 */
    cxlsb_set_overflow(CXLSB_OVERFLOW_CUT, CXLSB_OVERFLOW_CUT);
    writer = cxlsb_writer_create(TEST_FILE);
    err = cxlsb_writer_write_batch(writer, "Wide", wide_supplier, NULL, 1, 17000);
    check(err == CXLSB_OK, "Cut: 17000 columns accepted and truncated");
    cxlsb_writer_destroy(writer);

    reader = cxlsb_reader_create(TEST_FILE);
    if (reader != NULL) {
        wide_context wctx;
        memset(&wctx, 0, sizeof(wctx));
        err = cxlsb_reader_for_each_row(reader, 0, wide_handler, &wctx);
        check(err == CXLSB_OK && wctx.last_col_count == CXLSB_MAX_COLUMNS,
              "Cut: read back 16384 columns");
        cxlsb_reader_destroy(reader);
    }
    unlink(TEST_FILE);

    cxlsb_set_overflow(CXLSB_OVERFLOW_ERROR, CXLSB_OVERFLOW_ERROR);
    writer = cxlsb_writer_create(TEST_FILE);
    err = cxlsb_writer_write_batch(writer, "TooWide", wide_supplier, NULL, 1, 17000);
    check(err == CXLSB_ERR_LIMIT_EXCEEDED, "Error: over-limit columns reported");
    cxlsb_writer_destroy(writer);
    unlink(TEST_FILE);

    /* 3. 行上限策略快照 */
    cxlsb_sheet_limits_t limits;
    cxlsb_set_overflow(CXLSB_OVERFLOW_ERROR, CXLSB_OVERFLOW_ERROR);
    check(cxlsb_sheet_limits_begin(10, &limits) == CXLSB_OK, "row limits begin");
    int rows = 0;
    check(cxlsb_sheet_limits_rows(&limits, CXLSB_MAX_ROWS - 5, 10, &rows) == CXLSB_ERR_LIMIT_EXCEEDED,
          "Error: over-limit rows reported");
    cxlsb_set_row_overflow(CXLSB_OVERFLOW_CUT);
    rows = 0;
    check(cxlsb_sheet_limits_rows(&limits, CXLSB_MAX_ROWS - 5, 10, &rows) == CXLSB_ERR_LIMIT_EXCEEDED,
          "snapshot: later global change does not affect this operation");
    cxlsb_sheet_limits_t new_limits;
    cxlsb_sheet_limits_begin(10, &new_limits);
    rows = 0;
    check(cxlsb_sheet_limits_rows(&new_limits, CXLSB_MAX_ROWS - 5, 10, &rows) == CXLSB_OK && rows == 5,
          "new operation picks up current Cut policy");

    printf("\n=== Summary: %d passed, %d failed ===\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
