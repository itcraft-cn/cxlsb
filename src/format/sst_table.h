#ifndef CXLSB_SST_TABLE_H
#define CXLSB_SST_TABLE_H

#include <stdint.h>
#include "biff12_writer.h"

typedef struct cxlsb_sst_entry {
    char* string;
    uint32_t hash;
    uint32_t count;
} cxlsb_sst_entry_t;

typedef struct cxlsb_sst_table {
    cxlsb_sst_entry_t* entries;
    uint32_t* hash_to_index;
    uint32_t capacity;
    uint32_t count;
    uint32_t total_ref_count;
} cxlsb_sst_table_t;

cxlsb_sst_table_t* cxlsb_sst_table_create(uint32_t initial_capacity);

void cxlsb_sst_table_destroy(cxlsb_sst_table_t* table);

uint32_t cxlsb_sst_table_add_string(cxlsb_sst_table_t* table, const char* str);

int cxlsb_sst_table_find_string(cxlsb_sst_table_t* table, const char* str);

const char* cxlsb_sst_table_get_string(cxlsb_sst_table_t* table, uint32_t index);

uint32_t cxlsb_sst_table_get_count(cxlsb_sst_table_t* table);

uint32_t cxlsb_sst_table_get_total_count(cxlsb_sst_table_t* table);

cxlsb_error_t cxlsb_sst_table_serialize(cxlsb_sst_table_t* table,
                                         cxlsb_biff12_writer_t* writer);

#endif /* CXLSB_SST_TABLE_H */