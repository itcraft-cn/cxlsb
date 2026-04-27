#include "styles_registry.h"
#include "biff12_record_types.h"
#include "../io/buffer_writer.h"
#include <stdlib.h>
#include <string.h>

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void) {
    cxlsb_styles_registry_t* registry = (cxlsb_styles_registry_t*)malloc(sizeof(cxlsb_styles_registry_t));
    if (registry == NULL) return NULL;
    
    registry->xf_capacity = 16;
    registry->xfs = (cxlsb_xf_t*)malloc(sizeof(cxlsb_xf_t) * registry->xf_capacity);
    if (registry->xfs == NULL) { free(registry); return NULL; }
    
    registry->xf_count = 0;
    registry->default_xf_id = 0;
    
    cxlsb_xf_t default_xf = {0, 0, 0, 0, 0};
    registry->xfs[registry->xf_count++] = default_xf;
    return registry;
}

void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry) {
    if (registry != NULL) {
        if (registry->xfs != NULL) free(registry->xfs);
        free(registry);
    }
}

cxlsb_error_t cxlsb_styles_registry_to_bytes(cxlsb_styles_registry_t* registry, uint8_t** out_data, size_t* out_length) {
    cxlsb_biff12_writer_t* writer = cxlsb_biff12_writer_create(4096);
    if (writer == NULL) return CXLSB_ERR_NO_MEMORY;
    
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_STYLE_SHEET);
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_FILLS);
    cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_FILL, 4);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_FILLS);
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_BORDERS);
    cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_BORDER, 8);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_BORDERS);
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_CELL_XFS);
    
    for (uint32_t i = 0; i < registry->xf_count; i++) {
        cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_XF, 12);
        cxlsb_buffer_writer_write_int(writer->buffer, registry->xfs[i].font_id);
        cxlsb_buffer_writer_write_int(writer->buffer, registry->xfs[i].fill_id);
        cxlsb_buffer_writer_write_int(writer->buffer, registry->xfs[i].border_id);
    }
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_CELL_XFS);
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_STYLE_SHEET);
    
    size_t len = cxlsb_biff12_writer_get_length(writer);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) { cxlsb_biff12_writer_destroy(writer); return CXLSB_ERR_NO_MEMORY; }
    memcpy(data, cxlsb_biff12_writer_get_buffer(writer), len);
    
    *out_data = data;
    *out_length = len;
    cxlsb_biff12_writer_destroy(writer);
    return CXLSB_OK;
}