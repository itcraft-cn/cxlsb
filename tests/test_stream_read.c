#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_stream_read.xlsb";

static cxlsb_cell_data_t number_supplier(int row, int col, void* ud) {
    (void)ud;
    return cxlsb_cell_number(row * 100 + col);
}

typedef struct {
    int rows[1000];
    int count;
    int total_cols;
    double values[1000];
} stream_ctx;

static void stream_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    stream_ctx* ctx = (stream_ctx*)ud;
    ctx->rows[ctx->count] = row;
    ctx->total_cols = cols;
    ctx->values[ctx->count] = cells[0].value.number;
    ctx->count++;
}

TEST(stream_read_single_batch) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 3);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    cxlsb_error_t err = cxlsb_reader_start_streaming(r, 0);
    ASSERT_EQ(err, CXLSB_OK);
    
    stream_ctx ctx = {0};
    err = cxlsb_reader_read_batch(r, 100, stream_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx.count, 100);
    ASSERT_EQ(ctx.rows[0], 0);
    ASSERT_EQ(ctx.rows[99], 99);
    ASSERT_EQ(ctx.values[0], 0.0);
    ASSERT_EQ(ctx.values[99], 9900.0);
    
    err = cxlsb_reader_end_streaming(r);
    ASSERT_EQ(err, CXLSB_OK);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(stream_read_multiple_batches) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 100, 3);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    cxlsb_error_t err = cxlsb_reader_start_streaming(r, 0);
    ASSERT_EQ(err, CXLSB_OK);
    
    stream_ctx ctx1 = {0};
    err = cxlsb_reader_read_batch(r, 30, stream_handler, &ctx1);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx1.count, 30);
    ASSERT_EQ(ctx1.rows[0], 0);
    ASSERT_EQ(ctx1.rows[29], 29);
    
    ASSERT_EQ(cxlsb_reader_get_current_row(r), 30);
    ASSERT_TRUE(cxlsb_reader_has_more_rows(r));
    
    stream_ctx ctx2 = {0};
    err = cxlsb_reader_read_batch(r, 40, stream_handler, &ctx2);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx2.count, 40);
    ASSERT_EQ(ctx2.rows[0], 30);
    ASSERT_EQ(ctx2.rows[39], 69);
    
    stream_ctx ctx3 = {0};
    err = cxlsb_reader_read_batch(r, 50, stream_handler, &ctx3);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_EQ(ctx3.count, 30);
    ASSERT_EQ(ctx3.rows[0], 70);
    ASSERT_EQ(ctx3.rows[29], 99);
    
    ASSERT_TRUE(!cxlsb_reader_has_more_rows(r));
    
    err = cxlsb_reader_end_streaming(r);
    ASSERT_EQ(err, CXLSB_OK);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(stream_read_large_file) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 500, 5);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    cxlsb_error_t err = cxlsb_reader_start_streaming(r, 0);
    ASSERT_EQ(err, CXLSB_OK);
    
    int total_rows = 0;
    int batch_num = 0;
    
    while (cxlsb_reader_has_more_rows(r)) {
        stream_ctx ctx = {0};
        err = cxlsb_reader_read_batch(r, 50, stream_handler, &ctx);
        ASSERT_EQ(err, CXLSB_OK);
        total_rows += ctx.count;
        batch_num++;
        
        if (batch_num == 1) {
            ASSERT_EQ(ctx.rows[0], 0);
            ASSERT_EQ(ctx.rows[49], 49);
        }
    }
    
    ASSERT_EQ(total_rows, 500);
    ASSERT_EQ(batch_num, 10);
    
    err = cxlsb_reader_end_streaming(r);
    ASSERT_EQ(err, CXLSB_OK);
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(stream_read_state_tracking) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    cxlsb_writer_write_batch(w, "Data", number_supplier, NULL, 200, 3);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    cxlsb_error_t err = cxlsb_reader_start_streaming(r, 0);
    ASSERT_EQ(err, CXLSB_OK);
    
    ASSERT_TRUE(cxlsb_reader_has_more_rows(r));
    ASSERT_EQ(cxlsb_reader_get_current_row(r), 0);
    
    stream_ctx ctx1 = {0};
    cxlsb_reader_read_batch(r, 50, stream_handler, &ctx1);
    ASSERT_EQ(cxlsb_reader_get_current_row(r), 50);
    ASSERT_TRUE(cxlsb_reader_has_more_rows(r));
    
    stream_ctx ctx2 = {0};
    cxlsb_reader_read_batch(r, 50, stream_handler, &ctx2);
    ASSERT_EQ(cxlsb_reader_get_current_row(r), 100);
    ASSERT_TRUE(cxlsb_reader_has_more_rows(r));
    
    stream_ctx ctx3 = {0};
    cxlsb_reader_read_batch(r, 100, stream_handler, &ctx3);
    ASSERT_EQ(cxlsb_reader_get_current_row(r), 199);
    ASSERT_TRUE(!cxlsb_reader_has_more_rows(r));
    
    cxlsb_reader_end_streaming(r);
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Stream Read Tests ===\n\n");
    
    RUN_TEST(stream_read_single_batch);
    RUN_TEST(stream_read_multiple_batches);
    RUN_TEST(stream_read_large_file);
    RUN_TEST(stream_read_state_tracking);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}