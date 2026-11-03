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

int cxlsb_sst_table_find_string(cxlsb_sst_table_t* table, const char* str) {
    if (str == NULL) return -1;
    uint32_t hash = hash_string(str);
    uint32_t hash_slot = hash % (table->capacity * 2);
    uint32_t existing_index = table->hash_to_index[hash_slot];
    if (existing_index != 0xFFFFFFFF && strcmp(table->entries[existing_index].string, str) == 0) {
        return existing_index;
    }
    return -1;
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
        cxlsb_buffer_writer_write_int(writer->buffer, (uint32_t)(utf16_len / 2));
        
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf == NULL) return CXLSB_ERR_NO_MEMORY;
        cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
        err = cxlsb_buffer_writer_write_bytes(writer->buffer, utf16_buf, utf16_len);
        free(utf16_buf);
        if (err != CXLSB_OK) return err;
    }
    return cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_SST);
}

cxlsb_error_t cxlsb_sst_table_load(cxlsb_sst_table_t* table, const uint8_t* data, size_t len) {
    size_t pos = 0;
    
    while (pos + 2 <= len) {
        uint32_t record_type;
        size_t bytes_read;
        if (cxlsb_varint_decode_at(data + pos, &record_type, &bytes_read) < 0) break;
        pos += bytes_read;
        
        uint32_t record_size;
        if (cxlsb_varint_decode_at(data + pos, &record_size, &bytes_read) < 0) break;
        pos += bytes_read;
        
        if (pos + record_size > len) break;
        
        if (record_type == BIFF12_BRT_SST_ITEM) {
            if (record_size < 5) { pos += record_size; continue; }
            int encoding = data[pos];
            pos++;
            uint32_t str_len = data[pos] | (data[pos+1]<<8) | (data[pos+2]<<16) | (data[pos+3]<<24);
            pos += 4;
            
            if (encoding == 0) {
                /* 一次边界检查后顺序解码，避免逐码元重复校验。 */
                size_t byte_len = (size_t)str_len * 2;
                if (byte_len > record_size - 5) break;

                uint8_t* utf8_buf = (uint8_t*)malloc(byte_len * 3 + 1);
                if (utf8_buf == NULL) return CXLSB_ERR_NO_MEMORY;

                size_t utf8_len = 0;
                const uint8_t* src = data + pos;
                uint32_t i = 0;
                while (i < str_len) {
                    uint32_t cp = (uint32_t)(src[i * 2] | (src[i * 2 + 1] << 8));
                    i++;
                    if (cp >= 0xD800 && cp <= 0xDBFF && i < str_len) {
                        uint32_t lo = (uint32_t)(src[i * 2] | (src[i * 2 + 1] << 8));
                        if (lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            i++;
                        }
                    }
                    if (cp < 0x80) {
                        utf8_buf[utf8_len++] = (uint8_t)cp;
                    } else if (cp < 0x800) {
                        utf8_buf[utf8_len++] = (uint8_t)(0xC0 | (cp >> 6));
                        utf8_buf[utf8_len++] = (uint8_t)(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        utf8_buf[utf8_len++] = (uint8_t)(0xE0 | (cp >> 12));
                        utf8_buf[utf8_len++] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
                        utf8_buf[utf8_len++] = (uint8_t)(0x80 | (cp & 0x3F));
                    } else {
                        utf8_buf[utf8_len++] = (uint8_t)(0xF0 | (cp >> 18));
                        utf8_buf[utf8_len++] = (uint8_t)(0x80 | ((cp >> 12) & 0x3F));
                        utf8_buf[utf8_len++] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
                        utf8_buf[utf8_len++] = (uint8_t)(0x80 | (cp & 0x3F));
                    }
                }
                utf8_buf[utf8_len] = '\0';
                cxlsb_sst_table_add_string(table, (char*)utf8_buf);
                free(utf8_buf);
            }
            pos += str_len * 2;
        } else {
            pos += record_size;
        }
    }
    return CXLSB_OK;
}