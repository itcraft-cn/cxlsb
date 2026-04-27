#include "cxlsb/xlsb_reader.h"
#include "../src/format/sst_table.h"
#include "../src/container/xlsb_container_reader.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <xlsb>\n", argv[0]);
        return 1;
    }
    
    cxlsb_reader_t* r = cxlsb_reader_create(argv[1]);
    if (!r) {
        printf("Failed to create reader\n");
        return 1;
    }
    
    printf("SST count: %d\n", cxlsb_sst_table_get_count(r->sst));
    for (uint32_t i = 0; i < cxlsb_sst_table_get_count(r->sst); i++) {
        const char* s = cxlsb_sst_table_get_string(r->sst, i);
        printf("  SST[%u] = '%s' (len=%zu)\n", i, s, strlen(s));
        for (int j = 0; s[j]; j++) {
            printf("    byte[%d]=0x%02x\n", j, (unsigned char)s[j]);
        }
    }
    
    cxlsb_reader_destroy(r);
    return 0;
}