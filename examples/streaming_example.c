#include <cxlsb/xlsb_writer.h>
#include <cxlsb/cell_data.h>
#include <stdio.h>
#include <stdlib.h>

static cxlsb_cell_data_t data_supplier(int row, int col, void* ctx) {
    (void)ctx;
    
    if (col == 0) return cxlsb_cell_number(row + 1);
    if (col == 1) return cxlsb_cell_number((row + 1) * 100.0);
    if (col == 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Item-%d", row + 1);
        return cxlsb_cell_text(buf);
    }
    return cxlsb_cell_blank();
}

int main(int argc, char** argv) {
    const char* output_path = argc > 1 ? argv[1] : "/tmp/streaming_demo.xlsb";
    int rows = argc > 2 ? atoi(argv[2]) : 10000;
    int cols = argc > 3 ? atoi(argv[3]) : 3;
    
    printf("Creating %s with %d rows x %d cols...\n", output_path, rows, cols);
    
    cxlsb_writer_t* writer = cxlsb_writer_create(output_path);
    if (!writer) {
        fprintf(stderr, "Failed to create writer\n");
        return 1;
    }
    
    cxlsb_error_t err = cxlsb_writer_start_sheet(writer, "Data", cols);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Failed to start sheet: %d\n", err);
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    int batch_size = 1000;
    for (int start = 0; start < rows; start += batch_size) {
        int count = (start + batch_size > rows) ? (rows - start) : batch_size;
        err = cxlsb_writer_write_rows(writer, start, data_supplier, NULL, count);
        if (err != CXLSB_OK) {
            fprintf(stderr, "Failed at row %d: %d\n", start, err);
            cxlsb_writer_destroy(writer);
            return 1;
        }
        printf("  Written rows %d-%d\n", start, start + count - 1);
    }
    
    err = cxlsb_writer_end_sheet(writer);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Failed to end streaming: %d\n", err);
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    err = cxlsb_writer_close(writer);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Failed to close: %d\n", err);
    }
    
    cxlsb_writer_destroy(writer);
    
    FILE* f = fopen(output_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        printf("Done! File size: %ld bytes\n", ftell(f));
        fclose(f);
    }
    
    return 0;
}