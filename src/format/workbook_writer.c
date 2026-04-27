#include "workbook_writer.h"
#include "biff12_record_types.h"
#include "../io/buffer_writer.h"
#include "../util/utf16.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cxlsb_workbook_writer_t* cxlsb_workbook_writer_create(void) {
    cxlsb_workbook_writer_t* writer = (cxlsb_workbook_writer_t*)malloc(sizeof(cxlsb_workbook_writer_t));
    if (writer == NULL) return NULL;
    
    writer->capacity = 16;
    writer->sheet_names = (char**)malloc(sizeof(char*) * writer->capacity);
    if (writer->sheet_names == NULL) { free(writer); return NULL; }
    writer->sheet_count = 0;
    return writer;
}

void cxlsb_workbook_writer_destroy(cxlsb_workbook_writer_t* writer) {
    if (writer != NULL) {
        for (uint32_t i = 0; i < writer->sheet_count; i++) {
            if (writer->sheet_names[i] != NULL) free(writer->sheet_names[i]);
        }
        free(writer->sheet_names);
        free(writer);
    }
}

cxlsb_error_t cxlsb_workbook_writer_add_sheet(cxlsb_workbook_writer_t* writer, const char* name) {
    if (writer == NULL || name == NULL) return CXLSB_ERR_NULL_PTR;
    
    if (writer->sheet_count >= writer->capacity) {
        uint32_t new_capacity = writer->capacity * 2;
        char** new_names = (char**)realloc(writer->sheet_names, sizeof(char*) * new_capacity);
        if (new_names == NULL) return CXLSB_ERR_NO_MEMORY;
        writer->sheet_names = new_names;
        writer->capacity = new_capacity;
    }
    
    writer->sheet_names[writer->sheet_count] = strdup(name);
    if (writer->sheet_names[writer->sheet_count] == NULL) return CXLSB_ERR_NO_MEMORY;
    writer->sheet_count++;
    return CXLSB_OK;
}

uint32_t cxlsb_workbook_writer_get_count(cxlsb_workbook_writer_t* writer) { return writer->sheet_count; }

static void write_brt_file_version(cxlsb_biff12_writer_t* biff12) {
    static const uint8_t data[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0x00, 0x00, 0x78, 0x00, 0x6c, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x33, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x35, 0x00, 0x04, 0x00, 0x00, 0x00, 0x39, 0x00,
        0x33, 0x00, 0x30, 0x00, 0x32, 0x00, 0x00
    };
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_FILE_VERSION, sizeof(data));
    cxlsb_buffer_writer_write_bytes(biff12->buffer, data, sizeof(data));
}

static void write_brt_wb_prop(cxlsb_biff12_writer_t* biff12) {
    static const uint8_t data[] = {
        0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_WB_PROP, sizeof(data));
    cxlsb_buffer_writer_write_bytes(biff12->buffer, data, sizeof(data));
}

static void write_brt_book_view(cxlsb_biff12_writer_t* biff12) {
    static const uint8_t data[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x80, 0x70, 0x00, 0x00, 0xcf, 0x30, 0x00, 0x00,
        0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x78
    };
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BOOK_VIEW, sizeof(data));
    cxlsb_buffer_writer_write_bytes(biff12->buffer, data, sizeof(data));
}

static void write_xl_wide_string(cxlsb_biff12_writer_t* biff12, const char* str) {
    size_t len = strlen(str);
    size_t utf16_len = len * 2;
    
    cxlsb_buffer_writer_write_int(biff12->buffer, len);
    
    uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
    if (utf16_buf == NULL) return;
    cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
    cxlsb_buffer_writer_write_bytes(biff12->buffer, utf16_buf, utf16_len);
    free(utf16_buf);
}

static void write_brt_bundle_sh(cxlsb_biff12_writer_t* biff12, uint32_t sheet_id, const char* name) {
    char rel_id[16];
    snprintf(rel_id, sizeof(rel_id), "rId%d", sheet_id);
    
    size_t rel_id_len = strlen(rel_id);
    size_t name_len = strlen(name);
    uint32_t record_size = 4 + 4 + (4 + rel_id_len * 2) + (4 + name_len * 2);
    
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BUNDLE_SH, record_size);
    
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    cxlsb_buffer_writer_write_int(biff12->buffer, 1);
    
    write_xl_wide_string(biff12, rel_id);
    write_xl_wide_string(biff12, name);
}

cxlsb_error_t cxlsb_workbook_writer_to_bytes(cxlsb_workbook_writer_t* writer, uint8_t** out_data, size_t* out_length) {
    cxlsb_biff12_writer_t* biff12 = cxlsb_biff12_writer_create(4096);
    if (biff12 == NULL) return CXLSB_ERR_NO_MEMORY;
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BOOK);
    
    write_brt_file_version(biff12);
    
    write_brt_wb_prop(biff12);
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BOOK_VIEWS);
    
    write_brt_book_view(biff12);
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BOOK_VIEWS);
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BUNDLE_SHS);
    
    for (uint32_t i = 0; i < writer->sheet_count; i++) {
        write_brt_bundle_sh(biff12, i + 1, writer->sheet_names[i]);
    }
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BUNDLE_SHS);
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BOOK);
    
    size_t len = cxlsb_biff12_writer_get_length(biff12);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) { cxlsb_biff12_writer_destroy(biff12); return CXLSB_ERR_NO_MEMORY; }
    memcpy(data, cxlsb_biff12_writer_get_buffer(biff12), len);
    
    *out_data = data;
    *out_length = len;
    cxlsb_biff12_writer_destroy(biff12);
    return CXLSB_OK;
}