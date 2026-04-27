#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_streaming.xlsb";

typedef struct {
    long id;
    const char* customer;
    double amount;
} order_t;

static cxlsb_cell_data_t order_supplier(int row, int col, void* user_data) {
    order_t* orders = (order_t*)user_data;
    int idx = row;
    switch (col) {
        case 0: return cxlsb_cell_number(orders[idx].id);
        case 1: return cxlsb_cell_text(orders[idx].customer);
        case 2: return cxlsb_cell_number(orders[idx].amount);
        default: return cxlsb_cell_blank();
    }
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Streaming Write Tests ===\n\n");
    
    // Test 1: Streaming write with batches
    printf("Test 1: Streaming write with batches...\n");
    {
        cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
        cxlsb_error_t err = cxlsb_writer_start_sheet(writer, "Orders", 3);
        if (err != CXLSB_OK) {
            printf("  FAILED: start_sheet error %d\n", err);
            failed++;
        } else {
            order_t batch1[100];
            for (int i = 0; i < 100; i++) {
                batch1[i].id = i;
                batch1[i].customer = "Customer-A";
                batch1[i].amount = i * 100.5;
            }
            err = cxlsb_writer_write_rows(writer, 0, order_supplier, batch1, 100);
            if (err != CXLSB_OK) {
                printf("  FAILED: write_rows batch1 error %d\n", err);
                failed++;
            } else {
                order_t batch2[100];
                for (int i = 0; i < 100; i++) {
                    batch2[i].id = 100 + i;
                    batch2[i].customer = "Customer-B";
                    batch2[i].amount = (100 + i) * 100.5;
                }
                err = cxlsb_writer_write_rows(writer, 100, order_supplier, batch2, 100);
                if (err != CXLSB_OK) {
                    printf("  FAILED: write_rows batch2 error %d\n", err);
                    failed++;
                } else {
                    err = cxlsb_writer_end_sheet(writer);
                    if (err != CXLSB_OK) {
                        printf("  FAILED: end_sheet error %d\n", err);
                        failed++;
                    } else {
                        cxlsb_writer_destroy(writer);
                        
                        cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
                        if (reader == NULL) {
                            printf("  FAILED: reader is NULL\n");
                            failed++;
                        } else {
                            int count = cxlsb_reader_get_sheet_count(reader);
                            if (count != 1) {
                                printf("  FAILED: sheet_count=%d, expected 1\n", count);
                                failed++;
                            } else {
                                printf("  PASSED: 200 rows in 2 batches\n");
                                passed++;
                            }
                            cxlsb_reader_destroy(reader);
                        }
                    }
                }
            }
        }
        unlink(TEST_FILE);
    }
    
    // Test 2: Mixed mode - batch + streaming
    printf("Test 2: Mixed mode (batch + streaming)...\n");
    {
        cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
        
        // Sheet1: batch mode
        cxlsb_cell_data_t batch_supplier(int row, int col, void* ud) {
            (void)ud;
            return cxlsb_cell_number(row * col);
        }
        cxlsb_writer_write_batch(writer, "Summary", batch_supplier, NULL, 50, 5);
        
        // Sheet2: streaming mode
        cxlsb_writer_start_sheet(writer, "Details", 3);
        order_t batch[50];
        for (int i = 0; i < 50; i++) {
            batch[i].id = i;
            batch[i].customer = "Detail";
            batch[i].amount = i * 10.0;
        }
        cxlsb_writer_write_rows(writer, 0, order_supplier, batch, 50);
        cxlsb_writer_end_sheet(writer);
        
        // Sheet3: batch mode again
        cxlsb_cell_data_t footer_supplier(int row, int col, void* ud) {
            (void)ud;
            return cxlsb_cell_text("End");
        }
        cxlsb_writer_write_batch(writer, "Footer", footer_supplier, NULL, 10, 2);
        
        cxlsb_writer_destroy(writer);
        
        cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
        if (reader == NULL) {
            printf("  FAILED: reader is NULL\n");
            failed++;
        } else {
            int count = cxlsb_reader_get_sheet_count(reader);
            if (count != 3) {
                printf("  FAILED: sheet_count=%d, expected 3\n", count);
                failed++;
            } else {
                printf("  PASSED: 3 sheets (batch+streaming+batch)\n");
                passed++;
            }
            cxlsb_reader_destroy(reader);
        }
        unlink(TEST_FILE);
    }
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}