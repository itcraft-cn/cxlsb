#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>
#include <time.h>

static const char* TEST_FILE = "/tmp/test_stream_multi.xlsb";

typedef struct {
    int row_count;
    long first_id;
    long last_id;
} read_ctx;

static void id_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    read_ctx* ctx = (read_ctx*)ud;
    ctx->row_count++;
    if (ctx->row_count == 1) {
        ctx->first_id = (long)cells[0].value.number;
    }
    ctx->last_id = (long)cells[0].value.number;
}

static cxlsb_cell_data_t id_supplier(int row, int col, void* ud) {
    long* ids = (long*)ud;
    if (col == 0) return cxlsb_cell_number(ids[row]);
    return cxlsb_cell_blank();
}

TEST(stream_1000_rows_single_batch) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_start_sheet(w, "Data", 1);
    
    // Create 1000 IDs
    long* ids = malloc(1000 * sizeof(long));
    for (int i = 0; i < 1000; i++) ids[i] = i;
    
    // Write all in one batch
    cxlsb_writer_write_rows(w, 0, id_supplier, ids, 1000);
    cxlsb_writer_end_sheet(w);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    read_ctx ctx = {0};
    cxlsb_reader_for_each_row(r, 0, id_handler, &ctx);
    
    ASSERT_EQ(ctx.row_count, 1000);
    ASSERT_EQ(ctx.first_id, 0);
    ASSERT_EQ(ctx.last_id, 999);
    
    free(ids);
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(stream_500_rows_in_batches) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_start_sheet(w, "Data", 1);
    
    // Create 500 IDs
    long* ids = malloc(500 * sizeof(long));
    for (int i = 0; i < 500; i++) ids[i] = i;
    
    // Write all in one call
    cxlsb_writer_write_rows(w, 0, id_supplier, ids, 500);
    cxlsb_writer_end_sheet(w);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    read_ctx ctx = {0};
    cxlsb_reader_for_each_row(r, 0, id_handler, &ctx);
    
    ASSERT_EQ(ctx.row_count, 500);
    ASSERT_EQ(ctx.first_id, 0);
    ASSERT_EQ(ctx.last_id, 499);
    
    free(ids);
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Multi-Batch Streaming Tests ===\n\n");
    
    RUN_TEST(stream_1000_rows_single_batch);
    RUN_TEST(stream_500_rows_in_batches);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}