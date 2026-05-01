#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/cell_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cxlsb_cell_data_t format_supplier(int row, int col, void* ud) {
    (void)ud;
    double value = row * 123.456 - 500.0;
    
    switch (col) {
        case 0:
            char buf[32];
            sprintf(buf, "Row-%d", row);
            return cxlsb_cell_text(strdup(buf));
        case 1:
            return cxlsb_cell_number(value);
        case 2:
            return cxlsb_cell_number(value / 100.0);
        case 3:
            return cxlsb_cell_number(value);
        case 4:
            return cxlsb_cell_number(value);
        case 5:
            return cxlsb_cell_number(value);
        case 6:
            return cxlsb_cell_number(46142.31823571759);
        case 7:
            return cxlsb_cell_number(46142.31823571759);
        case 8:
            return cxlsb_cell_date(cxlsb_date_from_timestamp(1714560000 + row * 86400));
        default:
            return cxlsb_cell_blank();
    }
}

int main(void) {
    printf("Generating format_test.xlsb...\n");
    
    cxlsb_writer_t* w = cxlsb_writer_create("/tmp/format_test.xlsb");
    if (!w) {
        printf("Failed to create writer\n");
        return 1;
    }
    
    cxlsb_error_t err = cxlsb_writer_write_batch(w, "Formats", format_supplier, NULL, 20, 9);
    if (err != CXLSB_OK) {
        printf("Write error: %s\n", cxlsb_error_string(err));
        cxlsb_writer_destroy(w);
        return 1;
    }
    
    cxlsb_writer_destroy(w);
    
    printf("Generated /tmp/format_test.xlsb\n");
    printf("Columns:\n");
    printf("  Col 0: Text (Row-N)\n");
    printf("  Col 1: Number (普通数值)\n");
    printf("  Col 2: Percentage value (百分比数值)\n");
    printf("  Col 3: Negative number (负数)\n");
    printf("  Col 4: Currency value (货币数值)\n");
    printf("  Col 5: USD currency (美元数值)\n");
    printf("  Col 6: Date+Time value (日期时间数值)\n");
    printf("  Col 7: Time value (时间数值)\n");
    printf("  Col 8: Date from timestamp (日期)\n");
    printf("\nNote: Style/Format not applied yet, xf_id=0 for all cells\n");
    printf("      WPS will show default format for all columns\n");
    printf("20 rows, 9 columns\n");
    
    return 0;
}