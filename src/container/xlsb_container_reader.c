#include "xlsb_container_reader.h"
#include <zip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cxlsb_container_reader_t* cxlsb_container_reader_create(const char* path) {
    if (path == NULL) return NULL;
    
    cxlsb_container_reader_t* reader = (cxlsb_container_reader_t*)malloc(sizeof(cxlsb_container_reader_t));
    if (reader == NULL) return NULL;
    
    reader->path = strdup(path);
    if (reader->path == NULL) { free(reader); return NULL; }
    
    int error = 0;
    reader->zip_handle = zip_open(path, ZIP_RDONLY, &error);
    if (reader->zip_handle == NULL) {
        free(reader->path);
        free(reader);
        return NULL;
    }
    
    reader->is_open = true;
    return reader;
}

void cxlsb_container_reader_destroy(cxlsb_container_reader_t* reader) {
    if (reader != NULL) {
        if (reader->is_open && reader->zip_handle != NULL) {
            zip_close(reader->zip_handle);
        }
        if (reader->path != NULL) free(reader->path);
        free(reader);
    }
}

bool cxlsb_container_reader_has_entry(cxlsb_container_reader_t* reader, const char* name) {
    if (reader == NULL || name == NULL || !reader->is_open) return false;
    int64_t index = zip_name_locate(reader->zip_handle, name, 0);
    return index >= 0;
}

cxlsb_error_t cxlsb_container_reader_get_entry(cxlsb_container_reader_t* reader,
                                                const char* name,
                                                uint8_t** out_data,
                                                size_t* out_length) {
    if (reader == NULL || name == NULL || out_data == NULL || out_length == NULL) 
        return CXLSB_ERR_NULL_PTR;
    if (!reader->is_open || reader->zip_handle == NULL) return CXLSB_ERR_ZIP_OPEN_FAILED;
    
    int64_t index = zip_name_locate(reader->zip_handle, name, 0);
    if (index < 0) return CXLSB_ERR_ZIP_ENTRY_NOT_FOUND;
    
    struct zip_stat st;
    if (zip_stat_index(reader->zip_handle, index, 0, &st) != 0) return CXLSB_ERR_ZIP_READ_FAILED;
    
    *out_length = st.size;
    *out_data = (uint8_t*)malloc(st.size);
    if (*out_data == NULL) return CXLSB_ERR_NO_MEMORY;
    
    zip_file_t* file = zip_fopen_index(reader->zip_handle, index, 0);
    if (file == NULL) { free(*out_data); *out_data = NULL; return CXLSB_ERR_ZIP_READ_FAILED; }
    
    zip_int64_t bytes_read = zip_fread(file, *out_data, st.size);
    zip_fclose(file);
    
    if (bytes_read != (zip_int64_t)st.size) {
        free(*out_data);
        *out_data = NULL;
        return CXLSB_ERR_ZIP_READ_FAILED;
    }
    return CXLSB_OK;
}

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sheet_stream(
    cxlsb_container_reader_t* reader, int sheet_index) {
    if (reader == NULL || sheet_index < 0) return NULL;
    
    char name[64];
    snprintf(name, sizeof(name), "xl/worksheets/sheet%d.bin", sheet_index + 1);
    
    size_t length;
    uint8_t* data = NULL;
    if (cxlsb_container_reader_get_entry(reader, name, &data, &length) != CXLSB_OK) return NULL;
    
    return cxlsb_buffer_reader_create(data, length);
}

bool cxlsb_container_reader_has_sst(cxlsb_container_reader_t* reader) {
    return cxlsb_container_reader_has_entry(reader, "xl/sharedStrings.bin");
}

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sst_stream(cxlsb_container_reader_t* reader) {
    if (reader == NULL) return NULL;
    
    size_t length;
    uint8_t* data = NULL;
    if (cxlsb_container_reader_get_entry(reader, "xl/sharedStrings.bin", &data, &length) != CXLSB_OK)
        return NULL;
    return cxlsb_buffer_reader_create(data, length);
}

uint32_t cxlsb_container_reader_get_sheet_count(cxlsb_container_reader_t* reader) {
    if (reader == NULL || !reader->is_open) return 0;
    
    int64_t num_entries = zip_get_num_entries(reader->zip_handle, 0);
    uint32_t count = 0;
    
    for (int64_t i = 0; i < num_entries; i++) {
        const char* name = zip_get_name(reader->zip_handle, i, 0);
        if (name != NULL && strncmp(name, "xl/worksheets/sheet", 19) == 0) {
            const char* ext = strstr(name, ".bin");
            if (ext != NULL && ext[4] == '\0') count++;
        }
    }
    return count;
}