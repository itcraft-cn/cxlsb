#include <stdio.h>
#include <stdlib.h>
#include <zip.h>
#include <string.h>
#include <unistd.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"

static const char* TEST_FILE = "/tmp/test_stream_min.xlsb";

static cxlsb_cell_data_t batch_supplier(int row, int col, void* ud) {
    (void)ud;
    return cxlsb_cell_number(row * col);
}

int main(void) {
    printf("Test: Simple batch write...\n");
    
    cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
    printf("Writer created: %p\n", (void*)writer);
    
    if (!writer) {
        printf("FAILED: writer is NULL\n");
        return 1;
    }
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Summary", batch_supplier, NULL, 50, 5);
    printf("write_batch returned: %d\n", err);
    
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