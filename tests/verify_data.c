#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"
#include <stdio.h>
#include <string.h>

static char name_pool[100][32];
static char cat_pool[100][32];
static char r_pool[10][32];
static int pool_initialized = 0;

static void init_pool(void) {
    if (!pool_initialized) {
        for (int i = 0; i < 100; i++) sprintf(name_pool[i], "Name-%d", i);
        for (int i = 0; i < 100; i++) sprintf(cat_pool[i], "Cat-%d", i % 100);
        for (int i = 0; i < 10; i++) sprintf(r_pool[i], "R-%d", i % 10);
        pool_initialized = 1;
    }
}

static cxlsb_cell_data_t test_supplier(int row, int col, void* ud) {
    (void)ud;
    switch (col) {
        case 0:
            return cxlsb_cell_text(name_pool[row]);
        case 1:
            return cxlsb_cell_text(cat_pool[row % 100]);
        case 2:
            return cxlsb_cell_text(r_pool[row % 10]);
        case 3:
            return cxlsb_cell_number(row);
        case 4:
            return cxlsb_cell_number(row * 10);
        default:
            return cxlsb_cell_blank();
    }
}

typedef struct {
    int count;
    cxlsb_cell_data_t cells[10][5];
} read_data_t;

static void row_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    read_data_t* d = (read_data_t*)ud;
    if (row < 10) {
        for (int c = 0; c < cols; c++) {
            d->cells[row][c] = cells[c];
            if (cells[c].type == CXLSB_CELL_TEXT && cells[c].value.text != NULL) {
                d->cells[row][c].value.text = strdup(cells[c].value.text);
            }
        }
        d->count++;
    }
}

int main(void) {
    init_pool();
    
    cxlsb_writer_t* w = cxlsb_writer_create("/tmp/cxlsb_verify.xlsb");
    cxlsb_writer_write_batch(w, "Test", test_supplier, NULL, 10, 5);
    cxlsb_writer_destroy(w);
    
    // 读取验证
    cxlsb_reader_t* r = cxlsb_reader_create("/tmp/cxlsb_verify.xlsb");
    
    read_data_t data = {0};
    
    cxlsb_reader_for_each_row(r, 0, row_handler, &data);
    
    printf("前10行数据:\n");
    for (int i = 0; i < data.count && i < 10; i++) {
        printf("Row %d: ", i);
        for (int j = 0; j < 5; j++) {
            cxlsb_cell_data_t* cell = &data.cells[i][j];
            switch (cell->type) {
                case CXLSB_CELL_TEXT:
                    printf("\"%s\" ", cell->value.text);
                    break;
                case CXLSB_CELL_NUMBER:
                    printf("%.2f ", cell->value.number);
                    break;
                default:
                    printf("(blank) ");
            }
        }
        printf("\n");
    }
    
    cxlsb_reader_destroy(r);
    return 0;
}