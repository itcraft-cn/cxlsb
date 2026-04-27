#include "rels_gen.h"
#include <stdlib.h>
#include <stdio.h>

cxlsb_error_t cxlsb_rels_gen_root(uint8_t** out, size_t* out_length) {
    char* buffer = (char*)malloc(1024);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int len = snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.bin\"/>\n"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>\n"
        "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>\n"
        "</Relationships>\n");
    
    if (len < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_rels_gen_workbook(int sheet_count, 
                                        bool has_sst,
                                        uint8_t** out, 
                                        size_t* out_length) {
    char* buffer = (char*)malloc(2048);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int pos = snprintf(buffer, 2048,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n");
    
    if (pos < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    int rId = 1;
    
    for (int i = 1; i <= sheet_count; i++) {
        pos += snprintf(buffer + pos, 2048 - pos,
            "<Relationship Id=\"rId%d\""
            " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\""
            " Target=\"worksheets/sheet%d.bin\"/>\n", rId, i);
        rId++;
    }
    
    pos += snprintf(buffer + pos, 2048 - pos,
        "<Relationship Id=\"rId%d\""
        " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\""
        " Target=\"styles.bin\"/>\n", rId);
    rId++;
    
    pos += snprintf(buffer + pos, 2048 - pos,
        "<Relationship Id=\"rId%d\""
        " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\""
        " Target=\"theme/theme1.xml\"/>\n", rId);
    rId++;
    
    if (has_sst) {
        pos += snprintf(buffer + pos, 2048 - pos,
            "<Relationship Id=\"rId%d\""
            " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings\""
            " Target=\"sharedStrings.bin\"/>\n", rId);
    }
    
    pos += snprintf(buffer + pos, 2048 - pos, "</Relationships>\n");
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)pos;
    
    return CXLSB_OK;
}