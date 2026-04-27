#include <cxlsb/xlsb_writer.h>
#include <cxlsb/cell_data.h>
#include <cxlsb/types.h>
#include <stdio.h>
#include <stdlib.h>

static cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    if (col == 0) return cxlsb_cell_number(row + 1.0);
    return cxlsb_cell_number(row * 2.0);
}

int main(void) {
    printf("Creating writer...\n");
    cxlsb_writer_t* writer = cxlsb_writer_create("/tmp/simple_test.xlsb");
    if (writer == NULL) {
        printf("Failed to create writer\n");
        return 1;
    }
    printf("Writer created OK\n");
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, NULL, 10, 2);
    printf("Write batch result: %d\n", err);
    if (err != CXLSB_OK) {
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    err = cxlsb_writer_close(writer);
    printf("Close result: %d\n", err);
    
    cxlsb_writer_destroy(writer);
    printf("Done\n");
    return 0;
}
