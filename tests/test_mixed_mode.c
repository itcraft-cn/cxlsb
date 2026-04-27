#include <stdio.h>
#include <stdlib.h>
#include <zip.h>
#include <string.h>
#include <unistd.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"

static const char* TEST_FILE = "/tmp/test_mixed.xlsb";

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

static cxlsb_cell_data_t batch_supplier(int row, int col, void* ud) {
    (void)ud;
    return cxlsb_cell_number(row * col);
}

static cxlsb_cell_data_t footer_supplier(int row, int col, void* ud) {
    (void)ud;
    (void)row;
    (void)col;
    return cxlsb_cell_text("End");
}

int main(void) {
    printf("Test: Mixed mode (batch + streaming)...\n");
    
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    printf("Writer created: %p\n", (void*)writer);
    
    if (!writer) {
        printf("FAILED: writer is NULL\n");
        return 1;
    }
    
    // Sheet1: batch mode
    printf("Writing Sheet1 (batch)...\n");
    cxlsb_writer_write_batch(writer, "Summary", batch_supplier, NULL, 50, 5);
    
    // Sheet2: streaming mode
    printf("Writing Sheet2 (streaming)...\n");
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
    printf("Writing Sheet3 (batch)...\n");
    cxlsb_writer_write_batch(writer, "Footer", footer_supplier, NULL, 10, 2);
    
    cxlsb_writer_destroy(writer);
    printf("Writer destroyed\n");
    
    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    printf("Reader created: %p\n", (void*)reader);
    
    if (reader) {
        int count = cxlsb_reader_get_sheet_count(reader);
        printf("Sheet count: %d\n", count);
        cxlsb_reader_destroy(reader);
    }
    
    unlink(TEST_FILE);
    printf("PASSED\n");
    return 0;
}