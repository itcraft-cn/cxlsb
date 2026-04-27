#include "sheet_parser.h"
#include "biff12_record_types.h"
#include <stdlib.h>
#include <string.h>

static int varint_decode(const uint8_t* buf, int* pos) {
    uint32_t result = 0;
    int shift = 0;
    while (1) {
        uint8_t byte = buf[(*pos)++];
        result |= (uint32_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

static int read_int32_le(const uint8_t* buf, int pos) {
    return buf[pos] | (buf[pos+1]<<8) | (buf[pos+2]<<16) | (buf[pos+3]<<24);
}

static double read_double_le(const uint8_t* buf, int pos) {
    double d;
    memcpy(&d, buf + pos, 8);
    return d;
}

static int read_style_index(const uint8_t* buf, int offset) {
    return buf[offset+4] | (buf[offset+5]<<8) | (buf[offset+6]<<16);
}

static double decode_rk(int rk_value) {
    int is_int = rk_value & 1;
    int div100 = rk_value & 2;
    int value_bits = rk_value & 0xFFFFFFFC;
    double value;
    if (is_int) {
        value = value_bits >> 2;
    } else {
        long long double_bits = (long long)value_bits << 32;
        memcpy(&value, &double_bits, 8);
    }
    if (div100) value /= 100.0;
    return value;
}

cxlsb_sheet_parser* cxlsb_sheet_parser_create(const uint8_t* data, size_t len, const char** sst_strings, int sst_count) {
    cxlsb_sheet_parser* parser = calloc(1, sizeof(cxlsb_sheet_parser));
    parser->cell_capacity = 256;
    parser->cells = calloc(parser->cell_capacity, sizeof(cxlsb_cell_info));
    parser->merge_capacity = 16;
    parser->merges = calloc(parser->merge_capacity, sizeof(cxlsb_merge_cell));
    
    int pos = 0;
    int current_row = -1;
    
    while (pos + 2 <= (int)len) {
        int record_type = varint_decode(data, &pos);
        int record_size = varint_decode(data, &pos);
        
        if (pos + record_size > (int)len) break;
        
        switch (record_type) {
            case BIFF12_BRT_ROW_HDR:
                current_row = read_int32_le(data, pos);
                if (current_row > parser->max_row) parser->max_row = current_row;
                break;
            
            case BIFF12_BRT_CELL_ISST: {
                int col = read_int32_le(data, pos);
                int style_idx = read_style_index(data, pos);
                int sst_idx = read_int32_le(data, pos + 8);
                if (sst_idx >= 0 && sst_idx < sst_count && sst_strings[sst_idx]) {
                    if (parser->cell_count >= parser->cell_capacity) {
                        parser->cell_capacity *= 2;
                        parser->cells = realloc(parser->cells, parser->cell_capacity * sizeof(cxlsb_cell_info));
                    }
                    cxlsb_cell_info* ci = &parser->cells[parser->cell_count++];
                    ci->row = current_row;
                    ci->col = col;
                    ci->type = 1;
                    ci->style_index = style_idx;
                    ci->data.text = strdup(sst_strings[sst_idx]);
                    if (col > parser->max_col) parser->max_col = col;
                }
                break;
            }
            
            case BIFF12_BRT_CELL_REAL: {
                int col = read_int32_le(data, pos);
                int style_idx = read_style_index(data, pos);
                double value = read_double_le(data, pos + 8);
                if (parser->cell_count >= parser->cell_capacity) {
                    parser->cell_capacity *= 2;
                    parser->cells = realloc(parser->cells, parser->cell_capacity * sizeof(cxlsb_cell_info));
                }
                cxlsb_cell_info* ci = &parser->cells[parser->cell_count++];
                ci->row = current_row;
                ci->col = col;
                ci->type = 2;
                ci->style_index = style_idx;
                ci->data.number = value;
                if (col > parser->max_col) parser->max_col = col;
                break;
            }
            
            case BIFF12_BRT_CELL_RK: {
                int col = read_int32_le(data, pos);
                int style_idx = read_style_index(data, pos);
                int rk_val = read_int32_le(data, pos + 8);
                double value = decode_rk(rk_val);
                if (parser->cell_count >= parser->cell_capacity) {
                    parser->cell_capacity *= 2;
                    parser->cells = realloc(parser->cells, parser->cell_capacity * sizeof(cxlsb_cell_info));
                }
                cxlsb_cell_info* ci = &parser->cells[parser->cell_count++];
                ci->row = current_row;
                ci->col = col;
                ci->type = 2;
                ci->style_index = style_idx;
                ci->data.number = value;
                if (col > parser->max_col) parser->max_col = col;
                break;
            }
            
            case BIFF12_BRT_CELL_BOOL: {
                int col = read_int32_le(data, pos);
                int style_idx = read_style_index(data, pos);
                int value = data[pos + 8];
                if (parser->cell_count >= parser->cell_capacity) {
                    parser->cell_capacity *= 2;
                    parser->cells = realloc(parser->cells, parser->cell_capacity * sizeof(cxlsb_cell_info));
                }
                cxlsb_cell_info* ci = &parser->cells[parser->cell_count++];
                ci->row = current_row;
                ci->col = col;
                ci->type = 3;
                ci->style_index = style_idx;
                ci->data.bool_val = value;
                if (col > parser->max_col) parser->max_col = col;
                break;
            }
            
            case BIFF12_BRT_MERGE_CELL: {
                int rf = read_int32_le(data, pos);
                int rl = read_int32_le(data, pos + 4);
                int cf = read_int32_le(data, pos + 8);
                int cl = read_int32_le(data, pos + 12);
                if (parser->merge_count >= parser->merge_capacity) {
                    parser->merge_capacity *= 2;
                    parser->merges = realloc(parser->merges, parser->merge_capacity * sizeof(cxlsb_merge_cell));
                }
                cxlsb_merge_cell* mc = &parser->merges[parser->merge_count++];
                mc->row_first = rf;
                mc->row_last = rl;
                mc->col_first = cf;
                mc->col_last = cl;
                if (rl > parser->max_row) parser->max_row = rl;
                if (cl > parser->max_col) parser->max_col = cl;
                break;
            }
            
            case BIFF12_BRT_CELL_BLANK: {
                int col = read_int32_le(data, pos);
                if (col > parser->max_col) parser->max_col = col;
                break;
            }
        }
        
        pos += record_size;
    }
    
    return parser;
}

void cxlsb_sheet_parser_destroy(cxlsb_sheet_parser* parser) {
    if (!parser) return;
    for (int i = 0; i < parser->cell_count; i++) {
        if (parser->cells[i].type == 1 && parser->cells[i].data.text) {
            free(parser->cells[i].data.text);
        }
    }
    free(parser->cells);
    free(parser->merges);
    free(parser);
}