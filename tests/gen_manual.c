#include <stdio.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/cell_data.h"

static cxlsb_cell_data_t supplier(int row, int col, void* user_data) {
    (void)user_data;
    return cxlsb_cell_number(row * 10 + col);
}

int main(void) {
    cxlsb_writer_t* writer = cxlsb_writer_create("/tmp/manual_test.xlsb");
    cxlsb_writer_write_batch(writer, "Sheet1", supplier, NULL, 5, 5);
    cxlsb_writer_destroy(writer);
    printf("Created: /tmp/manual_test.xlsb\n");
    return 0;
}