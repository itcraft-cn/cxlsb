#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_api.xlsb";

static cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    (void)user_data;
    return cxlsb_cell_number(row + col);
}

static cxlsb_cell_data_t mixed_supplier(int row, int col, void* user_data) {
    (void)user_data;
    switch (col % 4) {
        case 0: return cxlsb_cell_text("RowX");
        case 1: return cxlsb_cell_number(row * 1.5);
        case 2: return cxlsb_cell_number(cxlsb_date_from_timestamp(1700000000000));
        case 3: return cxlsb_cell_bool(row % 2 == 0);
        default: return cxlsb_cell_blank();
    }
}

TEST(writer_create) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(writer);
    cxlsb_writer_destroy(writer);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(writer_write_batch) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(writer);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 10, 5);
    ASSERT_EQ(err, CXLSB_OK);
    
    ASSERT_EQ(cxlsb_writer_get_sheet_count(writer), 1);
    
    cxlsb_writer_destroy(writer);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(writer_multiple_sheets) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(writer);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 5, 3);
    ASSERT_EQ(err, CXLSB_OK);
    
    err = cxlsb_writer_write_batch(writer, "Sheet2", simple_supplier, NULL, 5, 3);
    ASSERT_EQ(err, CXLSB_OK);
    
    ASSERT_EQ(cxlsb_writer_get_sheet_count(writer), 2);
    
    cxlsb_writer_destroy(writer);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(writer_mixed_cell_types) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(writer);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "MixedTypes", mixed_supplier, NULL, 5, 4);
    ASSERT_EQ(err, CXLSB_OK);
    
    cxlsb_writer_destroy(writer);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(reader_create) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 10, 5);
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(reader);
    cxlsb_reader_destroy(reader);
    
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(reader_get_sheet_count) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 10, 5);
    cxlsb_writer_write_batch(writer, "Sheet2", simple_supplier, NULL, 5, 3);
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(reader);
    ASSERT_EQ(cxlsb_reader_get_sheet_count(reader), 2);
    cxlsb_reader_destroy(reader);
    
    unlink(TEST_FILE);
    return TEST_PASS;
}

typedef struct {
    int row_count;
    int max_row;
    int max_col;
    double first_value;
    double last_value;
} read_test_context;

static void read_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    read_test_context* ctx = (read_test_context*)user_data;
    ctx->row_count++;
    if (row > ctx->max_row) ctx->max_row = row;
    if (col_count > ctx->max_col) ctx->max_col = col_count;
    if (ctx->row_count == 1) ctx->first_value = cells[0].value.number;
    if (ctx->row_count == 10) ctx->last_value = cells[col_count-1].value.number;
}

TEST(reader_read_rows) {
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 10, 5);
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(reader);
    
    read_test_context ctx = {0};
    cxlsb_error_t err = cxlsb_reader_for_each_row(reader, 0, read_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx.row_count, 10);
    ASSERT_EQ(ctx.max_row, 9);
    ASSERT_EQ(ctx.max_col, 5);
    ASSERT_EQ(ctx.first_value, 0.0);
    ASSERT_EQ(ctx.last_value, 13.0);
    
    cxlsb_reader_destroy(reader);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== API Layer Tests ===\n\n");
    
    RUN_TEST(writer_create);
    RUN_TEST(writer_write_batch);
    RUN_TEST(writer_multiple_sheets);
    RUN_TEST(writer_mixed_cell_types);
    RUN_TEST(reader_create);
    RUN_TEST(reader_get_sheet_count);
    RUN_TEST(reader_read_rows);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}