#include "cxlsb/xlsb_reader.h"
#include <stdio.h>
#include <stdlib.h>

static void print_row(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    printf("Row %d: ", row);
    for (int i = 0; i < cols; i++) {
        if (cells[i].type == CXLSB_CELL_TEXT) {
            printf("[%s] ", cells[i].value.text);
        } else if (cells[i].type == CXLSB_CELL_NUMBER) {
            printf("[%g] ", cells[i].value.number);
        } else if (cells[i].type == CXLSB_CELL_BOOL) {
            printf("[%d] ", cells[i].value.boolean);
        } else {
            printf("[_] ");
        }
    }
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <xlsb_file>\n", argv[0]);
        return 1;
    }
    
    printf("Reading: %s\n\n", argv[1]);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(argv[1]);
    if (!reader) {
        printf("Failed to open file\n");
        return 1;
    }
    
    int sheets = cxlsb_reader_get_sheet_count(reader);
    printf("Sheet count: %d\n", sheets);
    
    for (int s = 0; s < sheets; s++) {
        printf("\nSheet %d:\n", s);
        cxlsb_reader_for_each_row(reader, s, print_row, NULL);
    }
    
    cxlsb_reader_destroy(reader);
    return 0;
}