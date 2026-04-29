#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_paged_read.xlsb";

static cxlsb_cell_data_t number_supplier(int row, int col, void* ud) {
    (void)ud;
    return cxlsb_cell_number(row * 10 + col);
}

typedef struct {
    int rows[100];
    int count;
    double first_val;
} page_ctx;

static void page_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    page_ctx* ctx = (page_ctx*)ud;
    ctx->rows[ctx->count++] = row;
    if (ctx->count == 1) ctx->first_val = cells[0].value.number;
}

TEST(paged_read_first_page) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 5);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    page_ctx ctx = {0};
    cxlsb_error_t err = cxlsb_reader_read_page(r, 0, 0, 30, page_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx.count, 30);
    ASSERT_EQ(ctx.rows[0], 0);
    ASSERT_EQ(ctx.rows[29], 29);
    ASSERT_EQ(ctx.first_val, 0.0);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(paged_read_middle_page) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 5);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    page_ctx ctx = {0};
    cxlsb_error_t err = cxlsb_reader_read_page(r, 0, 50, 20, page_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx.count, 20);
    ASSERT_EQ(ctx.rows[0], 50);
    ASSERT_EQ(ctx.rows[19], 69);
    ASSERT_EQ(ctx.first_val, 500.0);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(paged_read_last_page) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 5);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    page_ctx ctx = {0};
    cxlsb_error_t err = cxlsb_reader_read_page(r, 0, 80, 30, page_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx.count, 20);
    ASSERT_EQ(ctx.rows[0], 80);
    ASSERT_EQ(ctx.rows[19], 99);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(paged_read_multiple_pages) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 5);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    int total_rows = 0;
    
    page_ctx ctx1 = {0};
    cxlsb_reader_read_page(r, 0, 0, 25, page_handler, &ctx1);
    total_rows += ctx1.count;
    ASSERT_EQ(ctx1.count, 25);
    
    page_ctx ctx2 = {0};
    cxlsb_reader_read_page(r, 0, 25, 25, page_handler, &ctx2);
    total_rows += ctx2.count;
    ASSERT_EQ(ctx2.count, 25);
    ASSERT_EQ(ctx2.rows[0], 25);
    
    page_ctx ctx3 = {0};
    cxlsb_reader_read_page(r, 0, 50, 50, page_handler, &ctx3);
    total_rows += ctx3.count;
    ASSERT_EQ(ctx3.count, 50);
    ASSERT_EQ(ctx3.rows[0], 50);
    
    ASSERT_EQ(total_rows, 100);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Paged Read Tests ===\n\n");
    
    RUN_TEST(paged_read_first_page);
    RUN_TEST(paged_read_middle_page);
    RUN_TEST(paged_read_last_page);
    RUN_TEST(paged_read_multiple_pages);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}