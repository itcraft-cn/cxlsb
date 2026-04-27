#include "sst_table.h"
#include "biff12_record_types.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include "../io/buffer_writer.h"
#include <stdlib.h>
#include <string.h>

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    while (*str) hash = ((hash << 5) + hash) + (uint8_t)(*str++);
    return hash;
}

cxlsb_sst_table_t* cxlsb_sst_table_create(uint32_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 1024;
    
    cxlsb_sst_table_t* table = (cxlsb_sst_table_t*)malloc(sizeof(cxlsb_sst_table_t));
    if (table == NULL) return NULL;
    
    table->entries = (cxlsb_sst_entry_t*)malloc(sizeof(cxlsb_sst_entry_t) * initial_capacity);
    if (table->entries == NULL) { free(table); return NULL; }
    
    table->hash_to_index = (uint32_t*)malloc(sizeof(uint32_t) * initial_capacity * 2);
    if (table->hash_to_index == NULL) { free(table->entries); free(table); return NULL; }
    
    memset(table->hash_to_index, 0xFF, sizeof(uint32_t) * initial_capacity * 2);
    table->capacity = initial_capacity;
    table->count = 0;
    table->total_ref_count = 0;
    return table;
}

void cxlsb_sst_table_destroy(cxlsb_sst_table_t* table) {
    if (table != NULL) {
        for (uint32_t i = 0; i < table->count; i++) {
            if (table->entries[i].string != NULL) free(table->entries[i].string);
        }
        free(table->entries);
        free(table->hash_to_index);
        free(table);
    }
}

static cxlsb_error_t sst_table_expand(cxlsb_sst_table_t* table) {
    uint32_t new_capacity = table->capacity * 2;
    cxlsb_sst_entry_t* new_entries = (cxlsb_sst_entry_t*)realloc(table->entries, sizeof(cxlsb_sst_entry_t) * new_capacity);
    if (new_entries == NULL) return CXLSB_ERR_NO_MEMORY;
    
    uint32_t* new_hash = (uint32_t*)realloc(table->hash_to_index, sizeof(uint32_t) * new_capacity * 2);
    if (new_hash == NULL) { table->entries = new_entries; return CXLSB_ERR_NO_MEMORY; }
    
    memset(new_hash + table->capacity * 2, 0xFF, sizeof(uint32_t) * (new_capacity - table->capacity) * 2);
    table->entries = new_entries;
    table->hash_to_index = new_hash;
    table->capacity = new_capacity;
    return CXLSB_OK;
}

uint32_t cxlsb_sst_table_add_string(cxlsb_sst_table_t* table, const char* str) {
    if (str == NULL) return 0;
    
    uint32_t hash = hash_string(str);
    uint32_t hash_slot = hash % (table->capacity * 2);
    uint32_t existing_index = table->hash_to_index[hash_slot];
    
    if (existing_index != 0xFFFFFFFF && strcmp(table->entries[existing_index].string, str) == 0) {
        table->entries[existing_index].count++;
        table->total_ref_count++;
        return existing_index;
    }
    
    if (table->count >= table->capacity) {
        if (sst_table_expand(table) != CXLSB_OK) return 0;
        hash_slot = hash % (table->capacity * 2);
    }
    
    uint32_t new_index = table->count;
    table->entries[new_index].string = strdup(str);
    if (table->entries[new_index].string == NULL) return 0;
    
    table->entries[new_index].hash = hash;
    table->entries[new_index].count = 1;
    table->hash_to_index[hash_slot] = new_index;
    table->count++;
    table->total_ref_count++;
    return new_index;
}

const char* cxlsb_sst_table_get_string(cxlsb_sst_table_t* table, uint32_t index) {
    if (index >= table->count) return NULL;
    return table->entries[index].string;
}

uint32_t cxlsb_sst_table_get_count(cxlsb_sst_table_t* table) { return table->count; }
uint32_t cxlsb_sst_table_get_total_count(cxlsb_sst_table_t* table) { return table->total_ref_count; }

cxlsb_error_t cxlsb_sst_table_serialize(cxlsb_sst_table_t* table, cxlsb_biff12_writer_t* writer) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_BEGIN_SST, 8);
    if (err != CXLSB_OK) return err;
    cxlsb_buffer_writer_write_int(writer->buffer, table->total_ref_count);
    cxlsb_buffer_writer_write_int(writer->buffer, table->count);
    
    for (uint32_t i = 0; i < table->count; i++) {
        const char* str = table->entries[i].string;
        size_t utf16_len = cxlsb_utf16le_byte_length(str);
        uint32_t sst_item_size = 1 + 4 + utf16_len;
        
        err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_SST_ITEM, sst_item_size);
        if (err != CXLSB_OK) return err;
        cxlsb_buffer_writer_write_byte(writer->buffer, 0);
        cxlsb_buffer_writer_write_int(writer->buffer, (uint32_t)strlen(str));
        
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf == NULL) return CXLSB_ERR_NO_MEMORY;
        cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
        err = cxlsb_buffer_writer_write_bytes(writer->buffer, utf16_buf, utf16_len);
        free(utf16_buf);
        if (err != CXLSB_OK) return err;
    }
    return cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_SST);
}