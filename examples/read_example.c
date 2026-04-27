#include <cxlsb/xlsb_reader.h>
#include <cxlsb/types.h>
#include <stdio.h>

static void row_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    int* total_rows = (int*)user_data;
    (*total_rows)++;
    
    printf("Row %d: ", row);
    for (int i = 0; i < col_count; i++) {
        if (i > 0) printf(" | ");
        switch (cells[i].type) {
            case CXLSB_CELL_BLANK:
                printf("[blank]");
                break;
            case CXLSB_CELL_NUMBER:
                printf("%.2f", cells[i].value.number);
                break;
            case CXLSB_CELL_TEXT:
                printf("%s", cells[i].value.text ? cells[i].value.text : "(null)");
                break;
            case CXLSB_CELL_BOOL:
                printf("%s", cells[i].value.boolean ? "true" : "false");
                break;
        }
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : "output.xlsb";
    
    printf("Reading XLSB file: %s\n", filename);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(filename);
    if (reader == NULL) {
        fprintf(stderr, "Failed to create reader\n");
        return 1;
    }
    
    int sheet_count = cxlsb_reader_get_sheet_count(reader);
    printf("Found %d sheets\n", sheet_count);
    
    if (sheet_count > 0) {
        int total_rows = 0;
        cxlsb_error_t err = cxlsb_reader_for_each_row(reader, 0, row_handler, &total_rows);
        if (err != CXLSB_OK) {
            fprintf(stderr, "Failed to read sheet: %d\n", err);
        } else {
            printf("Total rows read: %d\n", total_rows);
        }
    }
    
    cxlsb_reader_destroy(reader);
    return 0;
}