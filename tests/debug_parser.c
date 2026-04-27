#include "cxlsb/template_filler.h"
#include "cxlsb/cell_data.h"
#include "../src/format/sheet_parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("=== Debug Template Parser ===\n\n");
    
    cxlsb_template_filler_t* filler = cxlsb_template_filler_create("tests/resources/template/demo_template.xlsb");
    if (!filler) {
        printf("Failed to create filler\n");
        return 1;
    }
    
    cxlsb_sheet_parser* parser = filler->sheets[0];
    printf("Parser: cell_count=%d, max_row=%d, max_col=%d\n", 
           parser->cell_count, parser->max_row, parser->max_col);
    
    printf("\nTemplate cells:\n");
    for (int i = 0; i < parser->cell_count; i++) {
        cxlsb_cell_info* ci = &parser->cells[i];
        printf("  [%d] row=%d col=%d type=%d xf=%d ", i, ci->row, ci->col, ci->type, ci->style_index);
        if (ci->type == 1) {
            printf("text='%s'\n", ci->data.text);
        } else if (ci->type == 2) {
            printf("num=%g\n", ci->data.number);
        } else {
            printf("other\n");
        }
    }
    
    printf("\nSST strings (%d):\n", cxlsb_sst_table_get_count(filler->sst));
    for (uint32_t i = 0; i < cxlsb_sst_table_get_count(filler->sst); i++) {
        printf("  [%u] '%s'\n", i, cxlsb_sst_table_get_string(filler->sst, i));
    }
    
    cxlsb_template_filler_destroy(filler);
    return 0;
}