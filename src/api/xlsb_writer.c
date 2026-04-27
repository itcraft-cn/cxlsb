#include "cxlsb/xlsb_writer.h"
#include "../container/xlsb_container.h"
#include "../format/sst_table.h"
#include "../format/styles_registry.h"
#include "../format/sheet_writer.h"
#include "../format/workbook_writer.h"
#include "../util/xml_gen.h"
#include "../util/rels_gen.h"
#include "../format/biff12_writer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_SHEETS 256

struct cxlsb_writer {
    cxlsb_container_t* container;
    cxlsb_sst_table_t* sst;
    cxlsb_styles_registry_t* styles;
    cxlsb_workbook_writer_t* workbook;
    cxlsb_sheet_writer_t* sheet_writer;
    int sheet_count;
    bool is_closed;
    char* current_sheet_name;
    int current_col_count;
    int current_row_count;
    bool streaming_active;
    cxlsb_sheet_data_entry_t sheet_data_cache[MAX_SHEETS];
};

cxlsb_writer_t* cxlsb_writer_create(const char* path) {
    if (path == NULL) return NULL;
    
    cxlsb_writer_t* writer = (cxlsb_writer_t*)malloc(sizeof(cxlsb_writer_t));
    if (writer == NULL) return NULL;
    
    memset(writer->sheet_data_cache, 0, sizeof(writer->sheet_data_cache));
    
    writer->container = cxlsb_container_create(path);
    if (writer->container == NULL) { free(writer); return NULL; }
    
    writer->sst = cxlsb_sst_table_create(1024);
    writer->styles = cxlsb_styles_registry_create();
    writer->workbook = cxlsb_workbook_writer_create();
    writer->sheet_writer = cxlsb_sheet_writer_create(writer->sst, writer->styles);
    
    if (writer->sst == NULL || writer->styles == NULL || writer->workbook == NULL || writer->sheet_writer == NULL) {
        if (writer->sst) cxlsb_sst_table_destroy(writer->sst);
        if (writer->styles) cxlsb_styles_registry_destroy(writer->styles);
        if (writer->workbook) cxlsb_workbook_writer_destroy(writer->workbook);
        if (writer->sheet_writer) cxlsb_sheet_writer_destroy(writer->sheet_writer);
        cxlsb_container_destroy(writer->container);
        free(writer);
        return NULL;
    }
    
    writer->sheet_count = 0;
    writer->is_closed = false;
    writer->current_sheet_name = NULL;
    writer->current_col_count = 0;
    writer->current_row_count = 0;
    writer->streaming_active = false;
    return writer;
}

void cxlsb_writer_destroy(cxlsb_writer_t* writer) {
    if (writer != NULL) {
        if (!writer->is_closed) cxlsb_writer_close(writer);
        if (writer->sheet_writer) cxlsb_sheet_writer_destroy(writer->sheet_writer);
        if (writer->workbook) cxlsb_workbook_writer_destroy(writer->workbook);
        if (writer->styles) cxlsb_styles_registry_destroy(writer->styles);
        if (writer->sst) cxlsb_sst_table_destroy(writer->sst);
        if (writer->container) cxlsb_container_destroy(writer->container);
        if (writer->current_sheet_name) free(writer->current_sheet_name);
        for (int i = 0; i < writer->sheet_count; i++) {
            if (writer->sheet_data_cache[i].name) free(writer->sheet_data_cache[i].name);
        }
        free(writer);
    }
}

cxlsb_error_t cxlsb_writer_write_batch(cxlsb_writer_t* writer, const char* sheet_name, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count, int col_count) {
    if (writer == NULL || sheet_name == NULL) return CXLSB_ERR_NULL_PTR;
    if (writer->sheet_count >= MAX_SHEETS) return CXLSB_ERR_INVALID_ARG;
    
    cxlsb_error_t err = cxlsb_workbook_writer_add_sheet(writer->workbook, sheet_name);
    if (err != CXLSB_OK) return err;
    
    uint8_t* sheet_data;
    size_t sheet_length;
    err = cxlsb_sheet_writer_write_sheet(writer->sheet_writer, supplier, user_data, row_count, col_count, &sheet_data, &sheet_length);
    if (err != CXLSB_OK) return err;
    
    char entry_name[64];
    snprintf(entry_name, sizeof(entry_name), "xl/worksheets/sheet%d.bin", writer->sheet_count + 1);
    err = cxlsb_container_add_entry(writer->container, entry_name, sheet_data, sheet_length);
    if (err != CXLSB_OK) {
        free(sheet_data);
        return err;
    }
    
    writer->sheet_data_cache[writer->sheet_count].data = sheet_data;
    writer->sheet_data_cache[writer->sheet_count].length = sheet_length;
    writer->sheet_data_cache[writer->sheet_count].name = strdup(entry_name);
    
    writer->sheet_count++;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_start_sheet(cxlsb_writer_t* writer, const char* sheet_name, int col_count) {
    if (writer == NULL || sheet_name == NULL) return CXLSB_ERR_NULL_PTR;
    if (writer->streaming_active) return CXLSB_ERR_SHEET_NOT_ENDED;
    
    writer->current_sheet_name = strdup(sheet_name);
    if (writer->current_sheet_name == NULL) return CXLSB_ERR_NO_MEMORY;
    
    writer->current_col_count = col_count;
    writer->current_row_count = 0;
    writer->streaming_active = true;
    return cxlsb_sheet_writer_start_streaming(writer->sheet_writer, col_count);
}

cxlsb_error_t cxlsb_writer_write_rows(cxlsb_writer_t* writer, int start_row, cxlsb_cell_supplier_fn supplier, void* user_data, int row_count) {
    if (writer == NULL) return CXLSB_ERR_NULL_PTR;
    if (!writer->streaming_active) return CXLSB_ERR_SHEET_NOT_STARTED;
    
    cxlsb_error_t err = cxlsb_sheet_writer_append_rows(writer->sheet_writer, supplier, user_data, start_row, row_count);
    if (err != CXLSB_OK) return err;
    
    writer->current_row_count = start_row + row_count;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_end_sheet(cxlsb_writer_t* writer) {
    if (writer == NULL) return CXLSB_ERR_NULL_PTR;
    if (!writer->streaming_active) return CXLSB_ERR_SHEET_NOT_STARTED;
    
    uint8_t* sheet_data;
    size_t sheet_length;
    cxlsb_error_t err = cxlsb_sheet_writer_finalize_streaming(writer->sheet_writer, writer->current_row_count, writer->current_col_count, &sheet_data, &sheet_length);
    if (err != CXLSB_OK) return err;
    
    cxlsb_workbook_writer_add_sheet(writer->workbook, writer->current_sheet_name);
    
    char entry_name[64];
    snprintf(entry_name, sizeof(entry_name), "xl/worksheets/sheet%d.bin", writer->sheet_count + 1);
    err = cxlsb_container_add_entry(writer->container, entry_name, sheet_data, sheet_length);
    free(sheet_data);
    if (err != CXLSB_OK) return err;
    
    free(writer->current_sheet_name);
    writer->current_sheet_name = NULL;
    writer->streaming_active = false;
    writer->sheet_count++;
    return CXLSB_OK;
}

int cxlsb_writer_get_sheet_count(cxlsb_writer_t* writer) {
    return writer == NULL ? -1 : writer->sheet_count;
}

static cxlsb_error_t write_container_structure(cxlsb_writer_t* writer) {
    uint8_t* data;
    size_t length;
    cxlsb_error_t err;
    
    err = cxlsb_xml_gen_content_types(writer->sheet_count, cxlsb_sst_table_get_count(writer->sst) > 0, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "[Content_Types].xml", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_rels_gen_root(&data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "_rels/.rels", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_xml_gen_app(writer->sheet_count, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "docProps/app.xml", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_xml_gen_core(&data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "docProps/core.xml", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_workbook_writer_to_bytes(writer->workbook, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/workbook.bin", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_styles_registry_to_bytes(writer->styles, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/styles.bin", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_xml_gen_theme(&data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/theme/theme1.xml", data, length);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_rels_gen_workbook(writer->sheet_count, cxlsb_sst_table_get_count(writer->sst) > 0, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/_rels/workbook.bin.rels", data, length);
    if (err != CXLSB_OK) return err;
    
    if (cxlsb_sst_table_get_count(writer->sst) > 0) {
        cxlsb_biff12_writer_t* biff12 = cxlsb_biff12_writer_create(4096);
        if (biff12 == NULL) return CXLSB_ERR_NO_MEMORY;
        
        err = cxlsb_sst_table_serialize(writer->sst, biff12);
        if (err != CXLSB_OK) { cxlsb_biff12_writer_destroy(biff12); return err; }
        
        length = cxlsb_biff12_writer_get_length(biff12);
        data = (uint8_t*)malloc(length);
        if (data == NULL) { cxlsb_biff12_writer_destroy(biff12); return CXLSB_ERR_NO_MEMORY; }
        memcpy(data, cxlsb_biff12_writer_get_buffer(biff12), length);
        
        err = cxlsb_container_add_entry(writer->container, "xl/sharedStrings.bin", data, length);
        cxlsb_biff12_writer_destroy(biff12);
        if (err != CXLSB_OK) return err;
    }
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_close(cxlsb_writer_t* writer) {
    if (writer == NULL) return CXLSB_ERR_NULL_PTR;
    if (writer->is_closed) return CXLSB_OK;
    
    if (writer->streaming_active) cxlsb_writer_end_sheet(writer);
    
    if (writer->sheet_count > 0) {
        cxlsb_error_t err = write_container_structure(writer);
        if (err != CXLSB_OK) return err;
    }
    
    cxlsb_error_t err = cxlsb_container_close(writer->container);
    if (err != CXLSB_OK) return err;
    
    writer->is_closed = true;
    return CXLSB_OK;
}