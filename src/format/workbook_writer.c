#include "workbook_writer.h"
#include "biff12_record_types.h"
#include "../io/buffer_writer.h"
#include "../util/utf16.h"
#include <stdlib.h>
#include <string.h>

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

cxlsb_error_t cxlsb_workbook_writer_to_bytes(cxlsb_workbook_writer_t* writer, uint8_t** out_data, size_t* out_length) {
    cxlsb_biff12_writer_t* biff12 = cxlsb_biff12_writer_create(4096);
    if (biff12 == NULL) return CXLSB_ERR_NO_MEMORY;
    
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BOOK);
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_FILE_VERSION, 16);
    cxlsb_buffer_writer_write_short(biff12->buffer, 0);
    cxlsb_buffer_writer_write_short(biff12->buffer, 0);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0x00020010);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_WB_PROP);
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BOOK_VIEWS);
    cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BOOK_VIEW, 12);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    cxlsb_buffer_writer_write_short(biff12->buffer, 0);
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BOOK_VIEWS);
    cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BUNDLE_SHS);
    
    for (uint32_t i = 0; i < writer->sheet_count; i++) {
        const char* name = writer->sheet_names[i];
        size_t utf16_len = cxlsb_utf16le_byte_length(name);
        uint32_t bundle_sh_size = 8 + 4 + utf16_len;
        cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BUNDLE_SH, bundle_sh_size);
        cxlsb_buffer_writer_write_int(biff12->buffer, i);
        cxlsb_buffer_writer_write_int(biff12->buffer, 0);
        cxlsb_buffer_writer_write_int(biff12->buffer, (uint32_t)strlen(name));
        
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf != NULL) {
            cxlsb_utf8_to_utf16le(name, utf16_buf, &utf16_len);
            cxlsb_buffer_writer_write_bytes(biff12->buffer, utf16_buf, utf16_len);
            free(utf16_buf);
        }
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