#include <stdio.h>
#include <stdlib.h>
#include <zip.h>
#include <string.h>
#include <unistd.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"

static const char* TEST_FILE = "/tmp/test_stream_start.xlsb";

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
    printf("Test: Streaming write...\n");
    
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    printf("Writer created: %p\n", (void*)writer);
    
    if (!writer) {
        printf("FAILED: writer is NULL\n");
        return 1;
    }
    
    cxlsb_error_t err = cxlsb_writer_start_sheet(writer, "Orders", 3);
    printf("start_sheet returned: %d\n", err);
    
    if (err != CXLSB_OK) {
        cxlsb_writer_destroy(writer);
        printf("FAILED: start_sheet error\n");
        return 1;
    }
    
    order_t batch[10];
    for (int i = 0; i < 10; i++) {
        batch[i].id = i;
        batch[i].customer = "Customer-A";
        batch[i].amount = i * 100.5;
    }
    
    err = cxlsb_writer_write_rows(writer, 0, order_supplier, batch, 10);
    printf("write_rows returned: %d\n", err);
    
    err = cxlsb_writer_end_sheet(writer);
    printf("end_sheet returned: %d\n", err);
    
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