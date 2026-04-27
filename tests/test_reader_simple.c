#include <stdio.h>
#include <stdlib.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_reader.xlsb";

static cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    (void)user_data;
    return cxlsb_cell_number(row * 10 + col);
}

int main(void) {
    printf("=== Simple Reader Test ===\n");
    
    printf("Step 1: Write file...\n");
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 5, 3);
    cxlsb_writer_destroy(writer);
    printf("  OK\n");
    
    printf("Step 2: Create reader...\n");
    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    if (reader == NULL) {
        printf("  FAILED: reader is NULL\n");
        unlink(TEST_FILE);
        return 1;
    }
    printf("  OK\n");
    
    printf("Step 3: Get sheet count...\n");
    int count = cxlsb_reader_get_sheet_count(reader);
    printf("  sheet_count = %d\n", count);
    cxlsb_reader_destroy(reader);
    printf("  OK\n");
    
    unlink(TEST_FILE);
    printf("\n=== Test Passed ===\n");
    return 0;
}