#include "cxlsb/template_filler.h"
#include "cxlsb/cell_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cxlsb_cell_data_t data_supplier(int row, int col, void* ud) {
    (void)ud;
    const char* names[] = {"赵六", "钱七", "孙八"};
    const char* cities[] = {"深圳", "杭州", "北京"};
    const char* ages[] = {"35", "22", "28"};
    
    switch (col) {
        case 0: return cxlsb_cell_text(names[row]);
        case 1: return cxlsb_cell_text(cities[row]);
        case 2: return cxlsb_cell_text(ages[row]);
        default: return cxlsb_cell_blank();
    }
}

int main(void) {
    printf("=== Template Filler API Tests ===\n\n");
    
    int passed = 0, failed = 0;
    
    // Test 1: Create filler
    printf("Test 1: Create filler from template...\n");
    cxlsb_template_filler_t* filler = cxlsb_template_filler_create("tests/resources/template/demo_template.xlsb");
    if (!filler) {
        printf("  FAILED: cannot create filler\n");
        failed++;
        return 1;
    }
    printf("  PASSED: filler created\n");
    passed++;
    
    // Test 2: Get sheet count
    printf("Test 2: Get sheet info...\n");
    int count = cxlsb_template_filler_get_sheet_count(filler);
    if (count != 1) {
        printf("  FAILED: sheet_count=%d, expected 1\n", count);
        failed++;
    } else {
        const char* name = cxlsb_template_filler_get_sheet_name(filler, 0);
        printf("  PASSED: 1 sheet, name='%s'\n", name);
        passed++;
    }
    
    // Test 3: Find marker
    printf("Test 3: Find ${data} marker...\n");
    int marker_row = cxlsb_template_filler_find_marker_row(filler, 0, "${data}");
    int marker_col = cxlsb_template_filler_find_marker_col(filler, 0, "${data}");
    if (marker_row < 0 || marker_col < 0) {
        printf("  FAILED: marker not found\n");
        failed++;
    } else {
        printf("  PASSED: marker at row=%d, col=%d\n", marker_row, marker_col);
        passed++;
    }
    
    // Test 4: Fill by marker
    printf("Test 4: Fill by marker...\n");
    cxlsb_error_t err = cxlsb_template_filler_fill_marker(filler, 0, "${data}",
                                                           data_supplier, NULL, 3, 4);
    if (err != CXLSB_OK) {
        printf("  FAILED: fill_marker error %d\n", err);
        failed++;
    } else {
        printf("  PASSED: fill_marker succeeded\n");
        passed++;
    }
    
    // Test 5: Save output
    printf("Test 5: Save to /tmp/fill_marker_api.xlsb...\n");
    err = cxlsb_template_filler_save(filler, "/tmp/fill_marker_api.xlsb");
    if (err != CXLSB_OK) {
        printf("  FAILED: save error %d\n", err);
        failed++;
    } else {
        printf("  PASSED: saved successfully\n");
        passed++;
    }
    
    cxlsb_template_filler_destroy(filler);
    
    // Test 6: Fill by index
    printf("\nTest 6: Fill by index (row=12, col=8)...\n");
    filler = cxlsb_template_filler_create("tests/resources/template/demo_template.xlsb");
    if (!filler) {
        printf("  FAILED: cannot create filler\n");
        failed++;
        return 1;
    }
    
    err = cxlsb_template_filler_fill_index(filler, 0, 12, 8,
                                            data_supplier, NULL, 2, 4);
    if (err != CXLSB_OK) {
        printf("  FAILED: fill_index error %d\n", err);
        failed++;
    } else {
        printf("  PASSED: fill_index succeeded\n");
        passed++;
    }
    
    err = cxlsb_template_filler_save(filler, "/tmp/fill_index_api.xlsb");
    if (err != CXLSB_OK) {
        printf("  FAILED: save error %d\n", err);
        failed++;
    } else {
        printf("  PASSED: saved to /tmp/fill_index_api.xlsb\n");
        passed++;
    }
    
    cxlsb_template_filler_destroy(filler);
    
    printf("\n=== Results ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}