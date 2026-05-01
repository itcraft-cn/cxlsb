#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/cell_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cxlsb_writer_t* writer = NULL;
static int style_indices[10];

static void init_formats(void) {
    int fmt1 = cxlsb_writer_add_format(writer, "#,##0.00");
    style_indices[1] = cxlsb_writer_add_style(writer, fmt1);
    
    int fmt2 = cxlsb_writer_add_format(writer, "0.00%");
    style_indices[2] = cxlsb_writer_add_style(writer, fmt2);
    
    int fmt3 = cxlsb_writer_add_format(writer, "#,##0.00;[Red]-#,##0.00");
    style_indices[3] = cxlsb_writer_add_style(writer, fmt3);
    
    int fmt4 = cxlsb_writer_add_format(writer, "￥#,##0.00");
    style_indices[4] = cxlsb_writer_add_style(writer, fmt4);
    
    int fmt5 = cxlsb_writer_add_format(writer, "$#,##0.00");
    style_indices[5] = cxlsb_writer_add_style(writer, fmt5);
    
    style_indices[6] = cxlsb_writer_add_style(writer, 22);
    style_indices[7] = cxlsb_writer_add_style(writer, 21);
    
    int fmt8 = cxlsb_writer_add_format(writer, "yyyy-mm-dd");
    style_indices[8] = cxlsb_writer_add_style(writer, fmt8);
    
    printf("Registered styles:\n");
    printf("  styles[%d]: #,##0.00 (千分位)\n", style_indices[1]);
    printf("  styles[%d]: 0.00%% (百分比)\n", style_indices[2]);
    printf("  styles[%d]: 负数红色\n", style_indices[3]);
    printf("  styles[%d]: ￥货币\n", style_indices[4]);
    printf("  styles[%d]: $货币\n", style_indices[5]);
    printf("  styles[%d]: m/d/yy h:mm (日期时间)\n", style_indices[6]);
    printf("  styles[%d]: h:mm:ss (时间)\n", style_indices[7]);
    printf("  styles[%d]: yyyy-mm-dd (日期)\n", style_indices[8]);
}

static cxlsb_cell_data_t format_supplier(int row, int col, void* ud) {
    (void)ud;
    double value = row * 123.456 - 500.0;
    
    switch (col) {
        case 0:
            char buf[32];
            sprintf(buf, "Row-%d", row);
            return cxlsb_cell_text_with_style(strdup(buf), 0);
        case 1:
            return cxlsb_cell_number_with_style(value, style_indices[1]);
        case 2:
            return cxlsb_cell_number_with_style(value / 100.0, style_indices[2]);
        case 3:
            return cxlsb_cell_number_with_style(value, style_indices[3]);
        case 4:
            return cxlsb_cell_number_with_style(value, style_indices[4]);
        case 5:
            return cxlsb_cell_number_with_style(value, style_indices[5]);
        case 6:
            return cxlsb_cell_number_with_style(46142.31823571759, style_indices[6]);
        case 7:
            return cxlsb_cell_number_with_style(46142.31823571759, style_indices[7]);
        case 8:
            return cxlsb_cell_date_with_style(cxlsb_date_from_timestamp(1714560000 + row * 86400), style_indices[8]);
        default:
            return cxlsb_cell_blank();
    }
}

int main(void) {
    printf("Generating format_test.xlsb...\n");
    
    writer = cxlsb_writer_create("/tmp/format_test.xlsb");
    if (!writer) {
        printf("Failed to create writer\n");
        return 1;
    }
    
    init_formats();
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Formats", format_supplier, NULL, 20, 9);
    if (err != CXLSB_OK) {
        printf("Write error: %s\n", cxlsb_error_string(err));
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    cxlsb_writer_destroy(writer);
    
    printf("\nGenerated /tmp/format_test.xlsb\n");
    printf("\n验证:\n");
    printf("  python3 ~/code/rust/rxlsb/tools/verify_style_index.py /tmp/format_test.xlsb\n");
    printf("\n预期:\n");
    printf("  XF总数=10, 各列显示正确格式\n");
    
    return 0;
}