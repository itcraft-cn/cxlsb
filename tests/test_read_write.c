#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>
#include <math.h>

static const char* TEST_FILE = "/tmp/test_read.xlsb";

static cxlsb_cell_data_t number_supplier(int row, int col, void* user_data) {
    (void)user_data;
    return cxlsb_cell_number(row * 10 + col);
}

static cxlsb_cell_data_t text_supplier(int row, int col, void* user_data) {
    (void)user_data;
    if (col == 0) return cxlsb_cell_number(row);
    return cxlsb_cell_text("T-Row");
}

typedef struct {
    int row_count;
    double first_value;
    double last_value;
    int first_row;
    int last_row;
} read_context;

static void number_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    read_context* ctx = (read_context*)user_data;
    ctx->row_count++;
    if (ctx->row_count == 1) {
        ctx->first_row = row;
        ctx->first_value = cells[0].value.number;
    }
    ctx->last_row = row;
    ctx->last_value = cells[col_count - 1].value.number;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Read/Write Roundtrip Tests ===\n\n");
    
    // Test 1: Write numbers, read back
    printf("Test 1: Write numbers (100 rows, 3 cols), read back...\n");
    {
        cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
        cxlsb_writer_write_batch(writer, "Sheet1", number_supplier, NULL, 100, 3);
        cxlsb_writer_destroy(writer);
        
        cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
        if (reader == NULL) {
            printf("  FAILED: reader is NULL\n");
            failed++;
        } else {
            read_context ctx = {0};
            cxlsb_error_t err = cxlsb_reader_for_each_row(reader, 0, number_handler, &ctx);
            if (err != CXLSB_OK) {
                printf("  FAILED: error %d\n", err);
                failed++;
            } else if (ctx.row_count != 100) {
                printf("  FAILED: row_count=%d, expected 100\n", ctx.row_count);
                failed++;
            } else if (ctx.first_value != 0.0) {
                printf("  FAILED: first_value=%f, expected 0\n", ctx.first_value);
                failed++;
            } else if (ctx.last_value != 992.0) {
                printf("  FAILED: last_value=%f, expected 992 (row=99, col=2)\n", ctx.last_value);
                failed++;
            } else {
                printf("  PASSED: 100 rows, first=%f, last=%f\n", ctx.first_value, ctx.last_value);
                passed++;
            }
            cxlsb_reader_destroy(reader);
        }
        unlink(TEST_FILE);
    }
    
    // Test 2: Write text, read back
    printf("Test 2: Write text (50 rows, 3 cols), read back...\n");
    {
        cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
        cxlsb_writer_write_batch(writer, "Sheet1", text_supplier, NULL, 50, 3);
        cxlsb_writer_destroy(writer);
        
        cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
        if (reader == NULL) {
            printf("  FAILED: reader is NULL\n");
            failed++;
        } else {
            read_context ctx = {0};
            cxlsb_error_t err = cxlsb_reader_for_each_row(reader, 0, number_handler, &ctx);
            if (err != CXLSB_OK) {
                printf("  FAILED: error %d\n", err);
                failed++;
            } else if (ctx.row_count != 50) {
                printf("  FAILED: row_count=%d, expected 50\n", ctx.row_count);
                failed++;
            } else {
                printf("  PASSED: 50 rows\n");
                passed++;
            }
            cxlsb_reader_destroy(reader);
        }
        unlink(TEST_FILE);
    }
    
    // Test 3: Multiple sheets
    printf("Test 3: Multiple sheets...\n");
    {
        cxlsb_writer_t* writer = cxlsb_writer_create(TEST_FILE);
        cxlsb_writer_write_batch(writer, "Sheet1", number_supplier, NULL, 10, 3);
        cxlsb_writer_write_batch(writer, "Sheet2", number_supplier, NULL, 20, 3);
        cxlsb_writer_destroy(writer);
        
        cxlsb_reader_t* reader = cxlsb_reader_create(TEST_FILE);
        if (reader == NULL) {
            printf("  FAILED: reader is NULL\n");
            failed++;
        } else {
            int count = cxlsb_reader_get_sheet_count(reader);
            if (count != 2) {
                printf("  FAILED: sheet_count=%d, expected 2\n", count);
                failed++;
            } else {
                printf("  PASSED: 2 sheets\n");
                passed++;
            }
            cxlsb_reader_destroy(reader);
        }
        unlink(TEST_FILE);
    }
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}