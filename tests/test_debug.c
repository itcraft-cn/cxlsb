#include <stdio.h>
#include <stdlib.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_debug.xlsb";

static cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    (void)user_data;
    double val = row * 10 + col;
    printf("  supplier(%d, %d) = %f\n", row, col, val);
    return cxlsb_cell_number(val);
}

static void debug_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    int* count = (int*)user_data;
    (*count)++;
    printf("Row %d: col_count=%d\n", row, col_count);
    for (int i = 0; i < col_count; i++) {
        printf("  cells[%d]: type=%d, value=%f\n", i, cells[i].type, 
               cells[i].type == CXLSB_CELL_NUMBER ? cells[i].value.number : 0.0);
    }
}

int main(void) {
    printf("=== Debug Test (5 rows, 3 cols) ===\n");
    
    printf("\n--- Writing ---\n");
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 5, 3);
    cxlsb_writer_destroy(writer);
    
    printf("\n--- Reading ---\n");
    cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
    int row_count = 0;
    cxlsb_reader_for_each_row(reader, 0, debug_handler, &row_count);
    cxlsb_reader_destroy(reader);
    
    printf("\nTotal rows read: %d\n", row_count);
    unlink(TEST_FILE);
    return 0;
}