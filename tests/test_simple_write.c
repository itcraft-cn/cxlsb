#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_simple.xlsb";

static cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    (void)user_data;
    return cxlsb_cell_number(row * 10 + col);
}

int main(void) {
    printf("=== Simple Write Test ===\n");
    
    printf("Step 1: Create writer...\n");
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    if (writer == NULL) {
        printf("  FAILED: writer is NULL\n");
        return 1;
    }
    printf("  OK\n");
    
    printf("Step 2: Write batch (10 rows, 5 cols)...\n");
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 10, 5);
    if (err != CXLSB_OK) {
        printf("  FAILED: error %d\n", err);
        cxlsb_writer_destroy(writer);
        return 1;
    }
    printf("  OK\n");
    
    printf("Step 3: Check sheet count...\n");
    int count = cxlsb_writer_get_sheet_count(writer);
    printf("  Sheet count: %d\n", count);
    if (count != 1) {
        printf("  FAILED: expected 1\n");
        cxlsb_writer_destroy(writer);
        return 1;
    }
    printf("  OK\n");
    
    printf("Step 4: Destroy writer (auto close)...\n");
    cxlsb_writer_destroy(writer);
    printf("  OK\n");
    
    printf("Step 5: Check file exists...\n");
    if (access(TEST_FILE, F_OK) != 0) {
        printf("  FAILED: file not found\n");
        return 1;
    }
    printf("  OK\n");
    
    unlink(TEST_FILE);
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}