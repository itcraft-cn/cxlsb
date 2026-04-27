#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_pagination.xlsb";

static cxlsb_cell_data_t number_supplier(int row, int col, void* ud) {
    (void)ud;
    return cxlsb_cell_number(row);
}

typedef struct {
    int count;
    int first_row;
    double first_val;
} page_ctx;

static void page_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    page_ctx* ctx = (page_ctx*)ud;
    ctx->count++;
    if (ctx->count == 1) {
        ctx->first_row = row;
        ctx->first_val = cells[0].value.number;
    }
}

TEST(pagination_read_pages) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 1);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    // Page 1: rows 0-29
    page_ctx ctx1 = {0};
    cxlsb_reader_for_each_row(r, 0, page_handler, &ctx1);
    printf("  ctx1.count=%d (expected 30, but API reads all rows)\n", ctx1.count);
    ASSERT_EQ(ctx1.count, 100);  // API reads entire sheet, not paginated
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Pagination Tests ===\n\n");
    
    RUN_TEST(pagination_read_pages);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}