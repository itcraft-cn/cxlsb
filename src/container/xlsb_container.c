#include "xlsb_container.h"
#include <zip.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

cxlsb_container_t* cxlsb_container_create(const char* path) {
    if (path == NULL) return NULL;
    
    cxlsb_container_t* container = (cxlsb_container_t*)malloc(sizeof(cxlsb_container_t));
    if (container == NULL) return NULL;
    
    container->path = strdup(path);
    if (container->path == NULL) { free(container); return NULL; }
    
    int error = 0;
    container->zip_handle = zip_open(path, ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (container->zip_handle == NULL) {
        free(container->path);
        free(container);
        return NULL;
    }
    
    container->is_open = true;
    return container;
}

void cxlsb_container_destroy(cxlsb_container_t* container) {
    if (container != NULL) {
        if (container->is_open && container->zip_handle != NULL) {
            zip_close(container->zip_handle);
        }
        if (container->path != NULL) free(container->path);
        free(container);
    }
}

cxlsb_error_t cxlsb_container_add_entry(cxlsb_container_t* container,
                                          const char* name,
                                          const uint8_t* data,
                                          size_t length) {
    if (container == NULL || name == NULL || data == NULL) return CXLSB_ERR_NULL_PTR;
    if (!container->is_open || container->zip_handle == NULL) return CXLSB_ERR_ZIP_OPEN_FAILED;
    
    zip_source_t* source = zip_source_buffer(container->zip_handle, data, length, 0);
    if (source == NULL) return CXLSB_ERR_ZIP_ADD_FAILED;
    
    int64_t index = zip_file_add(container->zip_handle, name, source, 0);
    if (index < 0) {
        zip_source_free(source);
        return CXLSB_ERR_ZIP_ADD_FAILED;
    }
    
    if (zip_set_file_compression(container->zip_handle, index, ZIP_CM_STORE, 0) != 0) {
        return CXLSB_ERR_ZIP_ADD_FAILED;
    }
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_container_close(cxlsb_container_t* container) {
    if (container == NULL) return CXLSB_ERR_NULL_PTR;
    if (!container->is_open || container->zip_handle == NULL) return CXLSB_ERR_ZIP_OPEN_FAILED;
    
    int result = zip_close(container->zip_handle);
    if (result != 0) return CXLSB_ERR_ZIP_ADD_FAILED;
    
    container->zip_handle = NULL;
    container->is_open = false;
    return CXLSB_OK;
}