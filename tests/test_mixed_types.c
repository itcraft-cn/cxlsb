#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <unistd.h>
#include <math.h>

static const char* TEST_FILE = "/tmp/test_mixed_types.xlsb";

static cxlsb_cell_data_t mixed_supplier(int row, int col, void* ud) {
    (void)ud;
    switch (col % 4) {
        case 0: return cxlsb_cell_text("TextRow");
        case 1: return cxlsb_cell_number(row * 1.5);
        case 2: return cxlsb_cell_bool(row % 2 == 0);
        case 3: return cxlsb_cell_number(row + 0.5);
        default: return cxlsb_cell_blank();
    }
}

typedef struct {
    int text_count;
    int num_count;
    int bool_count;
    int row_count;
} type_ctx;

static void type_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    type_ctx* ctx = (type_ctx*)ud;
    ctx->row_count++;
    for (int i = 0; i < cols; i++) {
        switch (cells[i].type) {
            case CXLSB_CELL_TEXT: ctx->text_count++; break;
            case CXLSB_CELL_NUMBER: ctx->num_count++; break;
            case CXLSB_CELL_BOOL: ctx->bool_count++; break;
        }
    }
}

TEST(write_mixed_cell_types) {
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    cxlsb_writer_write_batch(w, "Mixed", mixed_supplier, NULL, 10, 4);
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    type_ctx ctx = {0};
    cxlsb_reader_for_each_row(r, 0, type_handler, &ctx);
    
    ASSERT_EQ(ctx.row_count, 10);
    ASSERT_EQ(ctx.text_count, 10);  // 10 rows * 1 text column
    ASSERT_EQ(ctx.num_count, 20);   // 10 rows * 2 number columns
    ASSERT_EQ(ctx.bool_count, 10);  // 10 rows * 1 bool column
    
    cxlsb_reader_destroy(r);
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Mixed Cell Types Tests ===\n\n");
    
    RUN_TEST(write_mixed_cell_types);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}