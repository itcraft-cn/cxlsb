#include <cxlsb/xlsb_writer.h>
#include <cxlsb/types.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int current_row;
    int total_rows;
    int cols;
} simple_data_t;

static cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    simple_data_t* data = (simple_data_t*)user_data;
    if (row >= data->total_rows || col >= data->cols) {
        return cxlsb_cell_blank();
    }
    if (col == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Row%d", row);
        return cxlsb_cell_text(buf);
    } else if (col == 1) {
        return cxlsb_cell_number(row * 1.5);
    } else {
        return cxlsb_cell_bool((row % 2) == 0);
    }
}

int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : "output.xlsb";
    
    printf("Creating XLSB file: %s\n", filename);
    
    cxlsb_writer_t* writer = cxlsb_writer_create(filename);
    if (writer == NULL) {
        fprintf(stderr, "Failed to create writer\n");
        return 1;
    }
    
    simple_data_t data = {0, 100, 3};
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1", simple_supplier, &data, data.total_rows, data.cols);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Failed to write batch: %d\n", err);
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    err = cxlsb_writer_close(writer);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Failed to close writer: %d\n", err);
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    printf("Successfully created XLSB file with %d sheets\n", cxlsb_writer_get_sheet_count(writer));
    cxlsb_writer_destroy(writer);
    return 0;
}