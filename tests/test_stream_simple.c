#include <stdio.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>

static cxlsb_cell_data_t simple(int row, int col, void* ud) {
    (void)ud;
    return cxlsb_cell_number(row * 10 + col);
}

static int row_count = 0;
static double first_val = 0;
static double last_val = 0;

static void handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    (void)ud;
    row_count++;
    if (row_count == 1) {
        first_val = cells[0].value.number;
    }
    last_val = cells[cols-1].value.number;
}

int main(void) {
    printf("=== Streaming Write + Read Test ===\n\n");
    
    printf("Step 1: Create writer\n");
    cxlsb_writer_t* w = cxlsb_writer_create("/tmp/stream_simple.xlsb");
    if (!w) { printf("  FAILED\n"); return 1; }
    printf("  OK\n");
    
    printf("Step 2: Start sheet (3 cols)\n");
    cxlsb_error_t err = cxlsb_writer_start_sheet(w, "Test", 3);
    if (err != CXLSB_OK) { printf("  FAILED: %d\n", err); return 1; }
    printf("  OK\n");
    
    printf("Step 3: Write batch1 (5 rows at row 0)\n");
    err = cxlsb_writer_write_rows(w, 0, simple, NULL, 5);
    if (err != CXLSB_OK) { printf("  FAILED: %d\n", err); return 1; }
    printf("  OK\n");
    
    printf("Step 4: Write batch2 (5 rows at row 5)\n");
    err = cxlsb_writer_write_rows(w, 5, simple, NULL, 5);
    if (err != CXLSB_OK) { printf("  FAILED: %d\n", err); return 1; }
    printf("  OK\n");
    
    printf("Step 5: End sheet\n");
    err = cxlsb_writer_end_sheet(w);
    if (err != CXLSB_OK) { printf("  FAILED: %d\n", err); return 1; }
    printf("  OK\n");
    
    printf("Step 6: Destroy writer\n");
    cxlsb_writer_destroy(w);
    printf("  OK\n");
    
    printf("\nStep 7: Read back\n");
    cxlsb_reader_t* r = cxlsb_reader_create("/tmp/stream_simple.xlsb");
    if (!r) { printf("  FAILED: reader null\n"); return 1; }
    
    err = cxlsb_reader_for_each_row(r, 0, handler, NULL);
    if (err != CXLSB_OK) { printf("  FAILED: read error %d\n", err); return 1; }
    
    printf("  row_count=%d, first=%f, last=%f\n", row_count, first_val, last_val);
    
    if (row_count != 10) {
        printf("  FAILED: expected 10 rows\n");
        cxlsb_reader_destroy(r);
        return 1;
    }
    if (first_val != 0.0) {
        printf("  FAILED: expected first=0\n");
        cxlsb_reader_destroy(r);
        return 1;
    }
    if (last_val != 92.0) {  // row 9 * 10 + col 2
        printf("  FAILED: expected last=92\n");
        cxlsb_reader_destroy(r);
        return 1;
    }
    
    cxlsb_reader_destroy(r);
    printf("  OK - All 10 rows read correctly!\n");
    
    unlink("/tmp/stream_simple.xlsb");
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}