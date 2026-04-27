#include <cxlsb/types.h>
#include <cxlsb/error_codes.h>
#include <cxlsb/xlsb_writer.h>
#include <cxlsb/xlsb_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static cxlsb_cell_data_t test_supplier(int row, int col, void* user_data) {
    int* counter = (int*)user_data;
    (*counter)++;
    if (col == 0) return cxlsb_cell_text("Test");
    if (col == 1) return cxlsb_cell_number(123.45);
    return cxlsb_cell_bool(true);
}

static void test_cell_data(void) {
    cxlsb_cell_data_t blank = cxlsb_cell_blank();
    assert(blank.type == CXLSB_CELL_BLANK);
    
    cxlsb_cell_data_t num = cxlsb_cell_number(3.14);
    assert(num.type == CXLSB_CELL_NUMBER);
    assert(num.value.number > 3.13 && num.value.number < 3.15);
    
    cxlsb_cell_data_t text = cxlsb_cell_text("hello");
    assert(text.type == CXLSB_CELL_TEXT);
    assert(strcmp(text.value.text, "hello") == 0);
    free(text.value.text);
    
    cxlsb_cell_data_t bool_val = cxlsb_cell_bool(true);
    assert(bool_val.type == CXLSB_CELL_BOOL);
    assert(bool_val.value.boolean == true);
    
    printf("test_cell_data: PASSED\n");
}

static void test_write_read_cycle(void) {
    const char* test_file = "/tmp/test_cycle.xlsb";
    
    cxlsb_writer_t* writer = cxlsb_writer_create(test_file);
    assert(writer != NULL);
    
    int counter = 0;
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "TestSheet", test_supplier, &counter, 10, 3);
    assert(err == CXLSB_OK);
    assert(counter == 30);
    
    err = cxlsb_writer_close(writer);
    assert(err == CXLSB_OK);
    assert(cxlsb_writer_get_sheet_count(writer) == 1);
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(test_file);
    assert(reader != NULL);
    assert(cxlsb_reader_get_sheet_count(reader) == 1);
    cxlsb_reader_destroy(reader);
    
    printf("test_write_read_cycle: PASSED\n");
}

int main(void) {
    test_cell_data();
    test_write_read_cycle();
    printf("\nAll tests passed!\n");
    return 0;
}