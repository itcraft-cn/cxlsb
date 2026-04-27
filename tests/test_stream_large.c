#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>
#include <time.h>

static const char* TEST_FILE = "/tmp/test_stream_large.xlsb";

typedef struct {
    long id;
    double amount;
} order_t;

static cxlsb_cell_data_t batch_supplier(int row, int col, void* ud) {
    order_t* orders = (order_t*)ud;
    int idx = row;
    if (idx < 0) return cxlsb_cell_blank();
    switch (col) {
        case 0: return cxlsb_cell_number(orders[idx].id);
        case 1: return cxlsb_cell_text("Customer");
        case 2: return cxlsb_cell_number(orders[idx].amount);
        case 3: return cxlsb_cell_number(cxlsb_date_from_timestamp(time(NULL)*1000));
        default: return cxlsb_cell_blank();
    }
}

typedef struct {
    int row_count;
    long first_id;
    long last_id;
} read_ctx;

static void order_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    read_ctx* ctx = (read_ctx*)ud;
    ctx->row_count++;
    if (ctx->row_count == 1) {
        ctx->first_id = (long)cells[0].value.number;
    }
    ctx->last_id = (long)cells[0].value.number;
}

TEST(streaming_1000_rows_10_batches) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    cxlsb_error_t err = cxlsb_writer_start_sheet(w, "Orders", 4);
    ASSERT_EQ(err, CXLSB_OK);
    
    // Create 1000 orders total
    order_t* orders = malloc(1000 * sizeof(order_t));
    for (int i = 0; i < 1000; i++) {
        orders[i].id = i;
        orders[i].amount = i * 100.5;
    }
    
    // Write all 1000 rows using streaming
    err = cxlsb_writer_write_rows(w, 0, batch_supplier, orders, 1000);
    ASSERT_EQ(err, CXLSB_OK);
    
    err = cxlsb_writer_end_sheet(w);
    ASSERT_EQ(err, CXLSB_OK);
    cxlsb_writer_destroy(w);
    
    // Read back and verify
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    read_ctx ctx = {0};
    err = cxlsb_reader_for_each_row(r, 0, order_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    
    ASSERT_EQ(ctx.row_count, 1000);
    ASSERT_EQ(ctx.first_id, 0);
    ASSERT_EQ(ctx.last_id, 999);
    
    free(orders);
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

TEST(streaming_500_rows_multiple_batches) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    cxlsb_error_t err = cxlsb_writer_start_sheet(w, "Products", 3);
    ASSERT_EQ(err, CXLSB_OK);
    
    // Create 500 orders
    order_t* orders = malloc(500 * sizeof(order_t));
    for (int i = 0; i < 500; i++) {
        orders[i].id = i;
        orders[i].amount = i * 50.0;
    }
    
    // Write all 500 rows
    err = cxlsb_writer_write_rows(w, 0, batch_supplier, orders, 500);
    ASSERT_EQ(err, CXLSB_OK);
    
    err = cxlsb_writer_end_sheet(w);
    ASSERT_EQ(err, CXLSB_OK);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    read_ctx ctx = {0};
    err = cxlsb_reader_for_each_row(r, 0, order_handler, &ctx);
    ASSERT_EQ(err, CXLSB_OK);
    
    ASSERT_EQ(ctx.row_count, 500);
    
    free(orders);
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Large Streaming Tests ===\n\n");
    
    RUN_TEST(streaming_1000_rows_10_batches);
    RUN_TEST(streaming_500_rows_multiple_batches);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}