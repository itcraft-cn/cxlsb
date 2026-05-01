#include "test_framework.h"
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include "../src/format/styles_registry.h"
#include <unistd.h>

static const char* TEST_FILE = "/tmp/test_styles.xlsb";

static cxlsb_styles_registry_t* registry = NULL;

static cxlsb_cell_data_t styled_supplier(int row, int col, void* ud) {
    (void)ud;
    
    switch (col) {
        case 0:
            return cxlsb_cell_number(row);
        case 1:
            return cxlsb_cell_number(row * 1.5);
        case 2:
            return cxlsb_cell_number(row / 7.0);
        default:
            return cxlsb_cell_blank();
    }
}

TEST(styles_registry_create) {
    cxlsb_styles_registry_t* r = cxlsb_styles_registry_create();
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(r->style_count, 1);  // Default style
    cxlsb_styles_registry_destroy(r);
    return TEST_PASS;
}

TEST(styles_registry_add_format) {
    cxlsb_styles_registry_t* r = cxlsb_styles_registry_create();
    ASSERT_NOT_NULL(r);
    
    int fmt1 = cxlsb_styles_registry_add_format(r, "#,##0.00");
    ASSERT_TRUE(fmt1 >= 164);
    
    int fmt2 = cxlsb_styles_registry_add_format(r, "0.00%");
    ASSERT_TRUE(fmt2 > fmt1);
    
    int fmt3 = cxlsb_styles_registry_add_format(r, "#,##0.00");
    ASSERT_EQ(fmt3, fmt1);  // Duplicate returns same ID
    
    cxlsb_styles_registry_destroy(r);
    return TEST_PASS;
}

TEST(styles_registry_add_style) {
    cxlsb_styles_registry_t* r = cxlsb_styles_registry_create();
    ASSERT_NOT_NULL(r);
    
    int fmt1 = cxlsb_styles_registry_add_format(r, "#,##0.00");
    int style1 = cxlsb_styles_registry_add_style(r, fmt1);
    ASSERT_EQ(style1, 1);  // First custom style (index 1)
    
    int fmt2 = cxlsb_styles_registry_add_format(r, "0.00%");
    int style2 = cxlsb_styles_registry_add_style(r, fmt2);
    ASSERT_EQ(style2, 2);  // Second custom style (index 2)
    
    int style3 = cxlsb_styles_registry_add_style(r, fmt1);
    ASSERT_EQ(style3, 1);  // Duplicate returns same index
    
    cxlsb_styles_registry_destroy(r);
    return TEST_PASS;
}

TEST(styles_registry_serialize) {
    cxlsb_styles_registry_t* r = cxlsb_styles_registry_create();
    ASSERT_NOT_NULL(r);
    
    cxlsb_styles_registry_add_format(r, "#,##0.00");
    cxlsb_styles_registry_add_style(r, 164);
    
    uint8_t* data = NULL;
    size_t len = 0;
    cxlsb_error_t err = cxlsb_styles_registry_to_bytes(r, &data, &len);
    ASSERT_EQ(err, CXLSB_OK);
    ASSERT_NOT_NULL(data);
    ASSERT_TRUE(len > 0);
    
    free(data);
    cxlsb_styles_registry_destroy(r);
    return TEST_PASS;
}

static int row_count = 0;

static void row_counter(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    (void)row; (void)cols; (void)cells; (void)ud;
    row_count++;
}

TEST(styles_integration) {
    registry = cxlsb_styles_registry_create();
    ASSERT_NOT_NULL(registry);
    
    int fmt1 = cxlsb_styles_registry_add_format(registry, "#,##0.00");
    int fmt2 = cxlsb_styles_registry_add_format(registry, "0.00%");
    int fmt3 = cxlsb_styles_registry_add_format(registry, "yyyy-mm-dd");
    
    cxlsb_styles_registry_add_style(registry, fmt1);
    cxlsb_styles_registry_add_style(registry, fmt2);
    cxlsb_styles_registry_add_style(registry, fmt3);
    
    cxlsb_writer_t* w = cxlsb_writer_create(TEST_FILE);
    ASSERT_NOT_NULL(w);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(w, "Data", styled_supplier, NULL, 100, 3);
    ASSERT_EQ(err, CXLSB_OK);
    
    cxlsb_writer_destroy(w);
    
    cxlsb_reader_t* r = cxlsb_reader_create(TEST_FILE);
    ASSERT_NOT_NULL(r);
    
    row_count = 0;
    cxlsb_reader_for_each_row(r, 0, row_counter, NULL);
    
    ASSERT_EQ(row_count, 100);
    
    cxlsb_reader_destroy(r);
    cxlsb_styles_registry_destroy(registry);
    registry = NULL;
    
    unlink(TEST_FILE);
    return TEST_PASS;
}

int main(void) {
    int passed = 0, failed = 0;
    
    printf("\n=== Styles Registry Tests ===\n\n");
    
    RUN_TEST(styles_registry_create);
    RUN_TEST(styles_registry_add_format);
    RUN_TEST(styles_registry_add_style);
    RUN_TEST(styles_registry_serialize);
    RUN_TEST(styles_integration);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}