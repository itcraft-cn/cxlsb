#ifndef CXLSB_STYLES_REGISTRY_H
#define CXLSB_STYLES_REGISTRY_H

#include <stdint.h>
#include "biff12_writer.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_style {
    int num_fmt_id;
    int font_id;
    int fill_id;
    int border_id;
} cxlsb_style_t;

typedef struct cxlsb_format {
    uint16_t format_id;
    const char* format_string;
} cxlsb_format_t;

typedef struct cxlsb_styles_registry {
    cxlsb_style_t* styles;
    uint32_t style_count;
    uint32_t style_capacity;
    cxlsb_format_t* custom_formats;
    uint32_t format_count;
    uint32_t format_capacity;
    uint16_t next_format_id;
} cxlsb_styles_registry_t;

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void);
void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry);
int cxlsb_styles_registry_add_format(cxlsb_styles_registry_t* registry, const char* format_string);
int cxlsb_styles_registry_add_style(cxlsb_styles_registry_t* registry, int num_fmt_id);
cxlsb_error_t cxlsb_styles_registry_to_bytes(cxlsb_styles_registry_t* registry, uint8_t** out_data, size_t* out_length);

#endif