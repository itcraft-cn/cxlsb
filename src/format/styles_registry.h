#ifndef CXLSB_STYLES_REGISTRY_H
#define CXLSB_STYLES_REGISTRY_H

#include <stdint.h>
#include "biff12_writer.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_xf {
    uint32_t num_fmt_id;
    uint32_t font_id;
    uint32_t fill_id;
    uint32_t border_id;
    uint32_t xf_id;
} cxlsb_xf_t;

typedef struct cxlsb_styles_registry {
    cxlsb_xf_t* xfs;
    uint32_t xf_count;
    uint32_t xf_capacity;
    uint32_t default_xf_id;
} cxlsb_styles_registry_t;

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void);
void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry);
cxlsb_error_t cxlsb_styles_registry_to_bytes(cxlsb_styles_registry_t* registry, uint8_t** out_data, size_t* out_length);

#endif /* CXLSB_STYLES_REGISTRY_H */