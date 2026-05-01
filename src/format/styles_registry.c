#include "styles_registry.h"
#include "../util/utf16.h"
#include "biff12_writer.h"
#include "biff12_record_types.h"
#include <stdlib.h>
#include <string.h>

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void) {
    cxlsb_styles_registry_t* registry = (cxlsb_styles_registry_t*)malloc(sizeof(cxlsb_styles_registry_t));
    if (registry == NULL) return NULL;
    
    registry->style_capacity = 16;
    registry->styles = (cxlsb_style_t*)malloc(sizeof(cxlsb_style_t) * registry->style_capacity);
    if (registry->styles == NULL) { free(registry); return NULL; }
    
    registry->style_count = 0;
    cxlsb_style_t default_style = {0, 0, 0, 0};
    registry->styles[registry->style_count++] = default_style;
    
    registry->format_capacity = 16;
    registry->custom_formats = (cxlsb_format_t*)malloc(sizeof(cxlsb_format_t) * registry->format_capacity);
    if (registry->custom_formats == NULL) { free(registry->styles); free(registry); return NULL; }
    registry->format_count = 0;
    registry->next_format_id = 164;
    
    return registry;
}

void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry) {
    if (registry != NULL) {
        if (registry->styles != NULL) free(registry->styles);
        if (registry->custom_formats != NULL) free(registry->custom_formats);
        free(registry);
    }
}

int cxlsb_styles_registry_add_format(cxlsb_styles_registry_t* registry, const char* format_string) {
    for (uint32_t i = 0; i < registry->format_count; i++) {
        if (strcmp(registry->custom_formats[i].format_string, format_string) == 0) {
            return registry->custom_formats[i].format_id;
        }
    }
    
    if (registry->format_count >= registry->format_capacity) {
        uint32_t new_cap = registry->format_capacity * 2;
        cxlsb_format_t* new_formats = (cxlsb_format_t*)realloc(registry->custom_formats, sizeof(cxlsb_format_t) * new_cap);
        if (new_formats == NULL) return -1;
        registry->custom_formats = new_formats;
        registry->format_capacity = new_cap;
    }
    
    cxlsb_format_t* fmt = &registry->custom_formats[registry->format_count];
    fmt->format_id = registry->next_format_id++;
    fmt->format_string = format_string;
    registry->format_count++;
    return fmt->format_id;
}

int cxlsb_styles_registry_add_style(cxlsb_styles_registry_t* registry, int num_fmt_id) {
    for (uint32_t i = 0; i < registry->style_count; i++) {
        if (registry->styles[i].num_fmt_id == num_fmt_id) {
            return i;
        }
    }
    
    if (registry->style_count >= registry->style_capacity) {
        uint32_t new_cap = registry->style_capacity * 2;
        cxlsb_style_t* new_styles = (cxlsb_style_t*)realloc(registry->styles, sizeof(cxlsb_style_t) * new_cap);
        if (new_styles == NULL) return -1;
        registry->styles = new_styles;
        registry->style_capacity = new_cap;
    }
    
    cxlsb_style_t* style = &registry->styles[registry->style_count];
    style->num_fmt_id = num_fmt_id;
    style->font_id = 0;
    style->fill_id = 0;
    style->border_id = 0;
    return registry->style_count++;
}

static void write_brt_font(cxlsb_biff12_writer_t* w) {
    static const uint8_t font_data[] = {
        0xdc, 0x00, 0x00, 0x00,
        0x90, 0x01, 0x00, 0x00,
        0x00, 0x00,
        0x86, 0x00,
        0x07, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0xff,
        0x02, 0x02, 0x00, 0x00, 0x00,
        0x8b, 0x5b, 0x53, 0x4f
    };
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_FONT, sizeof(font_data));
    cxlsb_buffer_writer_write_bytes(w->buffer, font_data, sizeof(font_data));
}

static void write_brt_fill(cxlsb_biff12_writer_t* w, const uint8_t* data, size_t len) {
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_FILL, len);
    cxlsb_buffer_writer_write_bytes(w->buffer, data, len);
}

static void write_brt_border(cxlsb_biff12_writer_t* w) {
    uint8_t border_data[24] = {0};
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_BORDER, 24);
    cxlsb_buffer_writer_write_bytes(w->buffer, border_data, 24);
}

static void write_brt_xf_default(cxlsb_biff12_writer_t* w) {
    uint8_t xf_data[16] = {0};
    xf_data[0] = 0xff;
    xf_data[1] = 0xff;
    xf_data[12] = 0x08;
    xf_data[13] = 0x10;
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_XF, 16);
    cxlsb_buffer_writer_write_bytes(w->buffer, xf_data, 16);
}

static void write_brt_xf_style(cxlsb_biff12_writer_t* w, int format_id, int is_first) {
    uint8_t xf_data[16] = {0};
    xf_data[2] = format_id & 0xff;
    xf_data[3] = (format_id >> 8) & 0xff;
    xf_data[12] = 0x08;
    xf_data[13] = 0x10;
    if (!is_first) xf_data[14] = 0x01;
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_XF, 16);
    cxlsb_buffer_writer_write_bytes(w->buffer, xf_data, 16);
}

static void write_brt_fmt(cxlsb_biff12_writer_t* w, uint16_t format_id, const char* format_string) {
    size_t utf16_len = cxlsb_utf16le_byte_length(format_string);
    uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
    if (utf16_buf == NULL) return;
    
    size_t actual_len;
    cxlsb_utf8_to_utf16le(format_string, utf16_buf, &actual_len);
    
    size_t char_count = utf16_len / 2;
    uint32_t record_size = 2 + 4 + utf16_len;
    
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_FMT, record_size);
    cxlsb_buffer_writer_write_short(w->buffer, format_id);
    cxlsb_buffer_writer_write_int(w->buffer, char_count);
    cxlsb_buffer_writer_write_bytes(w->buffer, utf16_buf, utf16_len);
    
    free(utf16_buf);
}

cxlsb_error_t cxlsb_styles_registry_to_bytes(cxlsb_styles_registry_t* registry, uint8_t** out_data, size_t* out_length) {
    cxlsb_biff12_writer_t* w = cxlsb_biff12_writer_create(4096);
    if (w == NULL) return CXLSB_ERR_NO_MEMORY;
    
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_BEGIN_CELL_STYLE_XFS);
    
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_BEGIN_FMTS, 4);
    cxlsb_buffer_writer_write_int(w->buffer, registry->format_count);
    for (uint32_t i = 0; i < registry->format_count; i++) {
        write_brt_fmt(w, registry->custom_formats[i].format_id, registry->custom_formats[i].format_string);
    }
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_END_FMTS);
    
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_BEGIN_FONTS, 4);
    cxlsb_buffer_writer_write_int(w->buffer, 1);
    write_brt_font(w);
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_END_FONTS);
    
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_BEGIN_FILLS, 4);
    cxlsb_buffer_writer_write_int(w->buffer, 2);
    uint8_t fill1[4] = {0};
    write_brt_fill(w, fill1, 4);
    uint8_t fill2[4] = {0x02, 0x00, 0x80, 0x00};
    write_brt_fill(w, fill2, 4);
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_END_FILLS);
    
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_BEGIN_BORDERS, 4);
    cxlsb_buffer_writer_write_int(w->buffer, 1);
    write_brt_border(w);
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_END_BORDERS);
    
    cxlsb_biff12_writer_write_record_header(w, BIFF12_BRT_BEGIN_XFS, 4);
    cxlsb_buffer_writer_write_int(w->buffer, registry->style_count);
    for (uint32_t i = 0; i < registry->style_count; i++) {
        if (i == 0) {
            write_brt_xf_default(w);
        } else {
            write_brt_xf_style(w, registry->styles[i].num_fmt_id, false);
        }
    }
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_END_XFS);
    
    cxlsb_biff12_writer_write_empty_record(w, BIFF12_BRT_END_CELL_STYLE_XFS);
    
    size_t len = cxlsb_biff12_writer_get_length(w);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) { cxlsb_biff12_writer_destroy(w); return CXLSB_ERR_NO_MEMORY; }
    memcpy(data, cxlsb_biff12_writer_get_buffer(w), len);
    
    *out_data = data;
    *out_length = len;
    cxlsb_biff12_writer_destroy(w);
    return CXLSB_OK;
}