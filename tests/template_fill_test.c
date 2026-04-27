#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <stdint.h>
#include <cxlsb/xlsb_writer.h>
#include <cxlsb/cell_data.h>

#define TEMPLATE_PATH "tests/resources/template/demo_template.xlsb"
#define OUTPUT_PATH "/tmp/template_filled.xlsb"

static cxlsb_cell_data_t data_supplier(int row, int col, void* ctx) {
    (void)ctx;
    
    static const char* names[] = {"张三", "李四", "王五"};
    static const char* cities[] = {"北京", "上海", "广州"};
    static const int ages[] = {25, 30, 28};
    static const char* genders[] = {"男", "女", "男"};
    
    int data_row = row - 12;
    
    if (data_row < 0 || data_row >= 3) return cxlsb_cell_blank();
    
    switch (col) {
        case 8: return cxlsb_cell_text(names[data_row]);
        case 9: return cxlsb_cell_text(cities[data_row]);
        case 10: return cxlsb_cell_number((double)ages[data_row]);
        case 11: return cxlsb_cell_text(genders[data_row]);
        default: return cxlsb_cell_blank();
    }
}

int copy_zip_entry(zip_t* src, zip_t* dst, const char* name) {
    zip_file_t* file = zip_fopen(src, name, 0);
    if (!file) return -1;
    
    zip_stat_t stat;
    zip_stat(src, name, 0, &stat);
    
    uint8_t* data = malloc(stat.size);
    if (!data) {
        zip_fclose(file);
        return -1;
    }
    
    zip_fread(file, data, stat.size);
    zip_fclose(file);
    
    zip_source_t* source = zip_source_buffer(dst, data, stat.size, 1);
    if (!source) {
        free(data);
        return -1;
    }
    
    zip_file_add(dst, name, source, ZIP_FL_ENC_UTF_8);
    zip_set_file_compression(dst, zip_name_locate(dst, name, 0), ZIP_CM_STORE, 0);
    
    return 0;
}

int main(void) {
    printf("=== Template Fill Test ===\n");
    printf("Template: %s\n", TEMPLATE_PATH);
    printf("Output: %s\n\n", OUTPUT_PATH);
    
    int err = 0;
    zip_t* template_zip = zip_open(TEMPLATE_PATH, ZIP_RDONLY, &err);
    if (!template_zip) {
        fprintf(stderr, "Failed to open template: error %d\n", err);
        return 1;
    }
    
    printf("Template opened\n");
    
    zip_t* output_zip = zip_open(OUTPUT_PATH, ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!output_zip) {
        fprintf(stderr, "Failed to create output: error %d\n", err);
        zip_close(template_zip);
        return 1;
    }
    
    printf("Copying template structure...\n");
    
    const char* entries[] = {
        "[Content_Types].xml",
        "_rels/.rels",
        "docProps/app.xml",
        "docProps/core.xml",
        "docProps/custom.xml",
        "xl/_rels/workbook.bin.rels",
        "xl/sharedStrings.bin",
        "xl/styles.bin",
        "xl/theme/theme1.xml",
        "xl/workbook.bin"
    };
    
    for (int i = 0; i < 10; i++) {
        copy_zip_entry(template_zip, output_zip, entries[i]);
        printf("  Copied: %s\n", entries[i]);
    }
    
    zip_close(template_zip);
    
    printf("\nCreating new sheet data...\n");
    cxlsb_writer_t* writer = cxlsb_writer_create("/tmp/temp_sheet.xlsb");
    cxlsb_writer_write_batch(writer, "Sheet1", data_supplier, NULL, 15, 12);
    cxlsb_writer_close(writer);
    
    zip_t* temp_zip = zip_open("/tmp/temp_sheet.xlsb", ZIP_RDONLY, &err);
    if (!temp_zip) {
        fprintf(stderr, "Failed to read temp sheet\n");
        cxlsb_writer_destroy(writer);
        zip_close(output_zip);
        return 1;
    }
    
    copy_zip_entry(temp_zip, output_zip, "xl/worksheets/sheet1.bin");
    printf("  Added new sheet\n");
    
    zip_close(temp_zip);
    cxlsb_writer_destroy(writer);
    
    if (zip_close(output_zip) != 0) {
        fprintf(stderr, "Failed to close output\n");
        return 1;
    }
    
    FILE* f = fopen(OUTPUT_PATH, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        printf("\nOutput: %ld bytes\n", ftell(f));
        fclose(f);
    }
    
    printf("\nSuccess! Open %s with WPS.\n", OUTPUT_PATH);
    printf("Data at I13-L15:\n");
    printf("  张三  北京  25  男\n");
    printf("  李四  上海  30  女\n");
    printf("  王五  广州  28  男\n");
    
    return 0;
}