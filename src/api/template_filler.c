#include "cxlsb/template_filler.h"
#include "cxlsb/cell_data.h"
#include "../format/sheet_parser.h"
#include "../format/sst_table.h"
#include "../format/biff12_writer.h"
#include "../format/limits.h"
#include "../io/buffer_writer.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include <zip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct cxlsb_template_filler {
    zip_t* template_zip;
    cxlsb_sheet_parser** sheets;
    cxlsb_sst_table_t* sst;
    cxlsb_fill_result_t* fill_results;
    char** sheet_names;
    int sheet_count;
    int* sheet_data_lens;
    uint8_t** sheet_datas;
    int fill_applied;
};

static uint8_t* load_entry(zip_t* zip, const char* name, size_t* len) {
    zip_file_t* f = zip_fopen(zip, name, 0);
    if (!f) return NULL;
    zip_stat_t st;
    zip_stat(zip, name, 0, &st);
    uint8_t* data = malloc(st.size);
    if (!data) {
        zip_fclose(f);
        return NULL;
    }
    *len = zip_fread(f, data, st.size);
    zip_fclose(f);
    return data;
}

static int copy_entry(zip_t* src, zip_t* dst, const char* name) {
    zip_file_t* f = zip_fopen(src, name, 0);
    if (!f) return -1;
    zip_stat_t st;
    zip_stat(src, name, 0, &st);
    uint8_t* data = malloc(st.size);
    if (!data) {
        zip_fclose(f);
        return -1;
    }
    zip_fread(f, data, st.size);
    zip_fclose(f);
    zip_source_t* src_buf = zip_source_buffer(dst, data, st.size, 1);
    zip_file_add(dst, name, src_buf, ZIP_FL_ENC_UTF_8);
    return 0;
}

static void write_cell_isst(uint8_t* buf, int* pos, int col, int xf_id, int sst_idx) {
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(7, hdr);
    int slen = cxlsb_varint_encode(12, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t col_val = col;
    memcpy(buf + *pos, &col_val, 4);
    buf[*pos + 4] = xf_id & 0xFF;
    buf[*pos + 5] = (xf_id >> 8) & 0xFF;
    buf[*pos + 6] = (xf_id >> 16) & 0xFF;
    buf[*pos + 7] = 0;
    *pos += 8;
    
    uint32_t sst_idx_val = sst_idx;
    memcpy(buf + *pos, &sst_idx_val, 4);
    *pos += 4;
}

static void write_cell_real(uint8_t* buf, int* pos, int col, int xf_id, double val) {
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(4, hdr);
    int slen = cxlsb_varint_encode(16, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t col_val = col;
    memcpy(buf + *pos, &col_val, 4);
    buf[*pos + 4] = xf_id & 0xFF;
    buf[*pos + 5] = (xf_id >> 8) & 0xFF;
    buf[*pos + 6] = (xf_id >> 16) & 0xFF;
    buf[*pos + 7] = 0;
    *pos += 8;
    
    memcpy(buf + *pos, &val, 8);
    *pos += 8;
}

static void write_cell_rk(uint8_t* buf, int* pos, int col, int xf_id, double val) {
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(2, hdr);
    int slen = cxlsb_varint_encode(12, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;

    uint32_t col_val = (uint32_t)col;
    memcpy(buf + *pos, &col_val, 4);
    buf[*pos + 4] = xf_id & 0xFF;
    buf[*pos + 5] = (xf_id >> 8) & 0xFF;
    buf[*pos + 6] = (xf_id >> 16) & 0xFF;
    buf[*pos + 7] = 0;
    *pos += 8;

    int32_t iv = (int32_t)val;
    uint32_t rk = ((uint32_t)iv << 2) | 0x02;
    memcpy(buf + *pos, &rk, 4);
    *pos += 4;
}

/* 整数且 |n| < 2^29 用 RK，否则用 Real。 */
static void write_cell_number(uint8_t* buf, int* pos, int col, int xf_id, double val) {
    if (val >= -536870912.0 && val <= 536870911.0 && val == (double)(int32_t)val) {
        write_cell_rk(buf, pos, col, xf_id, val);
    } else {
        write_cell_real(buf, pos, col, xf_id, val);
    }
}

static void write_cell_bool(uint8_t* buf, int* pos, int col, int xf_id, int val) {
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(9, hdr);
    int slen = cxlsb_varint_encode(9, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t col_val = col;
    memcpy(buf + *pos, &col_val, 4);
    buf[*pos + 4] = xf_id & 0xFF;
    buf[*pos + 5] = (xf_id >> 8) & 0xFF;
    buf[*pos + 6] = (xf_id >> 16) & 0xFF;
    buf[*pos + 7] = 0;
    *pos += 8;
    
    buf[*pos] = val ? 1 : 0;
    *pos += 1;
}

static void write_row_hdr(uint8_t* buf, int* pos, int row, int col_count) {
    int num_spans = (col_count > 0) ? ((col_count - 1) / 1024 + 1) : 1;
    int record_size = 17 + num_spans * 8;
    
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(0, hdr);
    int slen = cxlsb_varint_encode(record_size, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t r = row;
    memcpy(buf + *pos, &r, 4);
    memset(buf + *pos + 4, 0, 4);
    buf[*pos + 8] = 0x0E;
    buf[*pos + 9] = 0x01;
    memset(buf + *pos + 10, 0, 3);
    
    uint32_t cs = num_spans;
    memcpy(buf + *pos + 13, &cs, 4);
    
    for (int seg = 0; seg < num_spans; seg++) {
        uint32_t mic = seg * 1024;
        uint32_t mac = mic + 1023;
        if (mac >= col_count) mac = col_count - 1;
        memcpy(buf + *pos + 17 + seg * 8, &mic, 4);
        memcpy(buf + *pos + 21 + seg * 8, &mac, 4);
    }
    *pos += 17 + num_spans * 8;
}

static void write_ws_dim(uint8_t* buf, int* pos, int r1, int r2, int c1, int c2) {
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(148, hdr);
    int slen = cxlsb_varint_encode(16, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t vals[4] = {r1, r2, c1, c2};
    memcpy(buf + *pos, vals, 16);
    *pos += 16;
}

static size_t write_sst_item(uint8_t* buf, int* pos, const char* str) {
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    uint8_t* utf16 = malloc(utf16_len);
    if (!utf16) return 0;
    cxlsb_utf8_to_utf16le(str, utf16, &utf16_len);
    
    size_t item_size = 1 + 4 + utf16_len;
    
    uint8_t hdr[10];
    int hlen = cxlsb_varint_encode(19, hdr);
    int slen = cxlsb_varint_encode(item_size, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    buf[(*pos)++] = 0;
    uint32_t sl = utf16_len / 2;
    memcpy(buf + *pos, &sl, 4);
    *pos += 4;
    
    for (size_t i = 0; i < utf16_len; i++) {
        buf[*pos + i] = utf16[i];
    }
    *pos += utf16_len;
    
    free(utf16);
    return hlen + slen + item_size;
}

cxlsb_template_filler_t* cxlsb_template_filler_create(const char* template_path) {
    int err;
    zip_t* zip = zip_open(template_path, ZIP_RDONLY, &err);
    if (!zip) return NULL;
    
    cxlsb_template_filler_t* filler = calloc(1, sizeof(cxlsb_template_filler_t));
    filler->template_zip = zip;
    filler->fill_applied = 0;
    
    size_t sst_len;
    uint8_t* sst_data = load_entry(zip, "xl/sharedStrings.bin", &sst_len);
    if (sst_data) {
        filler->sst = cxlsb_sst_table_create(256);
        cxlsb_sst_table_load(filler->sst, sst_data, sst_len);
        free(sst_data);
    } else {
        filler->sst = cxlsb_sst_table_create(256);
    }
    
    size_t wb_len;
    uint8_t* wb_data = load_entry(zip, "xl/workbook.bin", &wb_len);
    if (wb_data) {
        int sheet_count = 0;
        for (size_t i = 0; i + 1 < wb_len; i++) {
            if (wb_data[i] == 133 && wb_data[i+1] == 0x01) {
                sheet_count++;
            }
        }
        filler->sheet_count = sheet_count > 0 ? sheet_count : 1;
        filler->sheet_names = calloc(filler->sheet_count, sizeof(char*));
        filler->sheets = calloc(filler->sheet_count, sizeof(cxlsb_sheet_parser*));
        filler->sheet_datas = calloc(filler->sheet_count, sizeof(uint8_t*));
        filler->sheet_data_lens = calloc(filler->sheet_count, sizeof(int));
        filler->fill_results = calloc(filler->sheet_count, sizeof(cxlsb_fill_result_t));
        
        for (int i = 0; i < filler->sheet_count; i++) {
            char name[32];
            snprintf(name, sizeof(name), "Sheet%d", i + 1);
            filler->sheet_names[i] = strdup(name);
        }
        free(wb_data);
    }
    
    for (int i = 0; i < filler->sheet_count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "xl/worksheets/sheet%d.bin", i + 1);
        size_t len;
        uint8_t* data = load_entry(zip, path, &len);
        if (data) {
            filler->sheet_datas[i] = data;
            filler->sheet_data_lens[i] = len;
            
            int sst_count = cxlsb_sst_table_get_count(filler->sst);
            const char** sst_strings = malloc(sst_count * sizeof(char*));
            for (int j = 0; j < sst_count; j++) {
                sst_strings[j] = cxlsb_sst_table_get_string(filler->sst, j);
            }
            
            filler->sheets[i] = cxlsb_sheet_parser_create(data, len, sst_strings, sst_count);
            free(sst_strings);
        }
    }
    
    return filler;
}

void cxlsb_template_filler_destroy(cxlsb_template_filler_t* filler) {
    if (!filler) return;
    
    for (int i = 0; i < filler->sheet_count; i++) {
        if (filler->sheets[i]) cxlsb_sheet_parser_destroy(filler->sheets[i]);
        if (filler->sheet_names[i]) free(filler->sheet_names[i]);
        if (filler->sheet_datas[i]) free(filler->sheet_datas[i]);
        if (filler->fill_results[i].sst_indices) free(filler->fill_results[i].sst_indices);
        if (filler->fill_results[i].xf_indices) free(filler->fill_results[i].xf_indices);
        if (filler->fill_results[i].cell_types) free(filler->fill_results[i].cell_types);
        if (filler->fill_results[i].numbers) free(filler->fill_results[i].numbers);
    }
    free(filler->sheets);
    free(filler->sheet_names);
    free(filler->sheet_datas);
    free(filler->sheet_data_lens);
    free(filler->fill_results);
    
    if (filler->sst) cxlsb_sst_table_destroy(filler->sst);
    if (filler->template_zip) zip_close(filler->template_zip);
    
    free(filler);
}

int cxlsb_template_filler_get_sheet_count(cxlsb_template_filler_t* filler) {
    return filler ? filler->sheet_count : 0;
}

const char* cxlsb_template_filler_get_sheet_name(cxlsb_template_filler_t* filler, int sheet_index) {
    if (!filler || sheet_index < 0 || sheet_index >= filler->sheet_count) return NULL;
    return filler->sheet_names[sheet_index];
}

int cxlsb_template_filler_find_marker_row(cxlsb_template_filler_t* filler,
                                          int sheet_index,
                                          const char* marker) {
    if (!filler || sheet_index < 0 || sheet_index >= filler->sheet_count) return -1;
    if (!filler->sheets[sheet_index]) return -1;
    
    cxlsb_sheet_parser* parser = filler->sheets[sheet_index];
    for (int i = 0; i < parser->cell_count; i++) {
        cxlsb_cell_info* ci = &parser->cells[i];
        if (ci->type == 1 && ci->data.text) {
            if (strcmp(ci->data.text, marker) == 0) {
                return ci->row;
            }
        }
    }
    return -1;
}

int cxlsb_template_filler_find_marker_col(cxlsb_template_filler_t* filler,
                                          int sheet_index,
                                          const char* marker) {
    if (!filler || sheet_index < 0 || sheet_index >= filler->sheet_count) return -1;
    if (!filler->sheets[sheet_index]) return -1;
    
    cxlsb_sheet_parser* parser = filler->sheets[sheet_index];
    for (int i = 0; i < parser->cell_count; i++) {
        cxlsb_cell_info* ci = &parser->cells[i];
        if (ci->type == 1 && ci->data.text) {
            if (strcmp(ci->data.text, marker) == 0) {
                return ci->col;
            }
        }
    }
    return -1;
}

cxlsb_error_t cxlsb_template_filler_fill_index(cxlsb_template_filler_t* filler,
                                                int sheet_index,
                                                int start_row,
                                                int start_col,
                                                cxlsb_cell_supplier_fn supplier,
                                                void* user_data,
                                                int row_count,
                                                int col_count) {
    if (!filler) return CXLSB_ERR_NULL_PTR;
    if (sheet_index < 0 || sheet_index >= filler->sheet_count) return CXLSB_ERR_INVALID_ARG;
    if (!supplier) return CXLSB_ERR_NULL_PTR;
    
    cxlsb_fill_result_t* result = &filler->fill_results[sheet_index];

    /* 重复填充时先释放上一轮缓冲。 */
    free(result->sst_indices);
    free(result->xf_indices);
    free(result->cell_types);
    free(result->numbers);
    result->sst_indices = NULL;
    result->xf_indices = NULL;
    result->cell_types = NULL;
    result->numbers = NULL;

    size_t count = (size_t)row_count * (size_t)col_count;
    result->start_row = start_row;
    result->start_col = start_col;
    result->row_count = row_count;
    result->col_count = col_count;
    result->sst_indices = malloc(count * sizeof(int));
    result->xf_indices = malloc(count * sizeof(int));
    result->cell_types = malloc(count * sizeof(int));
    result->numbers = malloc(count * sizeof(double));
    if (!result->sst_indices || !result->xf_indices || !result->cell_types || !result->numbers) {
        free(result->sst_indices);
        free(result->xf_indices);
        free(result->cell_types);
        free(result->numbers);
        result->sst_indices = NULL;
        result->xf_indices = NULL;
        result->cell_types = NULL;
        result->numbers = NULL;
        return CXLSB_ERR_NO_MEMORY;
    }
    
    for (int r = 0; r < row_count; r++) {
        for (int c = 0; c < col_count; c++) {
            cxlsb_cell_data_t cell = supplier(r, c, user_data);
            size_t idx = (size_t)r * (size_t)col_count + (size_t)c;
            
            result->sst_indices[idx] = -1;
            result->xf_indices[idx] = cell.style_index;
            result->cell_types[idx] = cell.type;
            result->numbers[idx] = 0.0;

            switch (cell.type) {
                case CXLSB_CELL_TEXT:
                    result->sst_indices[idx] = cxlsb_sst_table_add_string(filler->sst, cell.value.text);
                    break;
                case CXLSB_CELL_NUMBER:
                case CXLSB_CELL_DATE:
                    result->numbers[idx] = cell.value.number;
                    break;
                case CXLSB_CELL_BOOL:
                    result->numbers[idx] = cell.value.boolean ? 1.0 : 0.0;
                    break;
                default:
                    break;
            }
        }
    }
    
    filler->fill_applied = 1;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_template_filler_fill_marker(cxlsb_template_filler_t* filler,
                                                 int sheet_index,
                                                 const char* marker,
                                                 cxlsb_cell_supplier_fn supplier,
                                                 void* user_data,
                                                 int row_count,
                                                 int col_count) {
    int marker_row = cxlsb_template_filler_find_marker_row(filler, sheet_index, marker);
    int marker_col = cxlsb_template_filler_find_marker_col(filler, sheet_index, marker);
    
    if (marker_row < 0 || marker_col < 0) {
        return CXLSB_ERR_MARKER_NOT_FOUND;
    }
    
    return cxlsb_template_filler_fill_index(filler, sheet_index, marker_row, marker_col,
                                            supplier, user_data, row_count, col_count);
}

static uint8_t* generate_filled_sheet(cxlsb_template_filler_t* filler,
                                       int sheet_index,
                                       size_t* out_len) {
    cxlsb_sheet_parser* parser = filler->sheets[sheet_index];
    cxlsb_fill_result_t* result = &filler->fill_results[sheet_index];
    
    if (!parser || result->row_count == 0) {
        *out_len = filler->sheet_data_lens[sheet_index];
        uint8_t* data = malloc(*out_len);
        if (!data) {
            *out_len = 0;
            return NULL;
        }
        memcpy(data, filler->sheet_datas[sheet_index], *out_len);
        return data;
    }
    
    if (parser->cell_count == 0) {
        *out_len = 0;
        return NULL;
    }
    
    int start_row = parser->cells[0].row;
    int max_row = parser->max_row;
    if (result->start_row + result->row_count - 1 > max_row) {
        max_row = result->start_row + result->row_count - 1;
    }
    
    int min_col = parser->cells[0].col;
    int max_col = parser->max_col;
    if (result->start_col + result->col_count - 1 > max_col) {
        max_col = result->start_col + result->col_count - 1;
    }
    if (result->start_col < min_col) {
        min_col = result->start_col;
    }
    
    /* 底层维度限制：与批量/流式写入使用同一策略（模板填充也受控）。 */
    cxlsb_sheet_limits_t limits;
    if (cxlsb_sheet_limits_begin(max_col + 1, &limits) != CXLSB_OK) {
        *out_len = 0;
        return NULL;
    }
    int actual_rows = 0;
    if (cxlsb_sheet_limits_rows(&limits, 0, max_row + 1, &actual_rows) != CXLSB_OK) {
        *out_len = 0;
        return NULL;
    }
    max_row = actual_rows > 0 ? actual_rows - 1 : 0;
    max_col = cxlsb_sheet_limits_columns(&limits) - 1;
    if (max_col < min_col || max_row < start_row) {
        *out_len = 0;
        return NULL;
    }

    int num_rows = max_row - start_row + 1;
    int num_cols = max_col - min_col + 1;
    size_t estimated_size = 100
        + (size_t)num_rows * ((size_t)50 + (size_t)num_cols * 20)
        + (size_t)parser->merge_count * 20;
    if (estimated_size < 1024) estimated_size = 1024;
    
    uint8_t* sheet = malloc(estimated_size);
    if (!sheet) {
        *out_len = 0;
        return NULL;
    }
    int pos = 0;
    
    uint8_t begin_sheet[] = {0x81, 0x01, 0x00};
    memcpy(sheet + pos, begin_sheet, 3);
    pos += 3;
    
    uint8_t wsprop[] = {0x93, 0x01, 0x17, 0xC9, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    memcpy(sheet + pos, wsprop, 26);
    pos += 26;
    
    write_ws_dim(sheet, &pos, start_row, max_row, min_col, max_col);
    
    uint8_t wsview_block[] = {
        0x85, 0x01, 0x00,
        0x89, 0x01, 0x1E, 
        0xDC, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x98, 0x01, 0x24, 0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00,
        0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x8A, 0x01, 0x00,
        0x86, 0x01, 0x00
    };
    memcpy(sheet + pos, wsview_block, sizeof(wsview_block));
    pos += sizeof(wsview_block);
    
    uint8_t wsfmt[] = {0xE5, 0x03, 0x0C, 0x00, 0x09, 0x00, 0x00, 0x08, 0x00,
                       0x0E, 0x01, 0x00, 0x00, 0x00, 0x00};
    memcpy(sheet + pos, wsfmt, 15);
    pos += 15;
    
    uint8_t begin_sheet_data[] = {0x91, 0x01, 0x00};
    memcpy(sheet + pos, begin_sheet_data, 3);
    pos += 3;
    
    for (int row = start_row; row <= max_row; row++) {
        write_row_hdr(sheet, &pos, row, max_col + 1);
        
        // Preserve template cells for rows outside fill region
        if (row < result->start_row || row >= result->start_row + result->row_count) {
            for (int i = 0; i < parser->cell_count; i++) {
                cxlsb_cell_info* ci = &parser->cells[i];
                if (ci->row == row && ci->col <= max_col) {
                    if (ci->type == 1) {
                        int sst_idx = cxlsb_sst_table_find_string(filler->sst, ci->data.text);
                        if (sst_idx >= 0) {
                            write_cell_isst(sheet, &pos, ci->col, ci->style_index, sst_idx);
                        }
                    } else if (ci->type == 2) {
                        write_cell_number(sheet, &pos, ci->col, ci->style_index, ci->data.number);
                    }
                }
            }
        }
        
        // Write fill data for rows in fill region
        if (row >= result->start_row && row < result->start_row + result->row_count
            && result->start_col <= max_col) {
            int data_row = row - result->start_row;
            int fill_end = result->start_col + result->col_count - 1;
            if (fill_end > max_col) fill_end = max_col;
            for (int col = result->start_col; col <= fill_end; col++) {
                int data_col = col - result->start_col;
                int idx = data_row * result->col_count + data_col;
                
                switch (result->cell_types[idx]) {
                    case CXLSB_CELL_TEXT:
                        if (result->sst_indices[idx] >= 0) {
                            write_cell_isst(sheet, &pos, col,
                                            result->xf_indices[idx], result->sst_indices[idx]);
                        }
                        break;
                    case CXLSB_CELL_NUMBER:
                    case CXLSB_CELL_DATE:
                        write_cell_number(sheet, &pos, col,
                                          result->xf_indices[idx], result->numbers[idx]);
                        break;
                    case CXLSB_CELL_BOOL:
                        write_cell_bool(sheet, &pos, col,
                                        result->xf_indices[idx],
                                        result->numbers[idx] != 0.0);
                        break;
                    default:
                        /* 空白单元格不生成记录。 */
                        break;
                }
            }
        }
    }
    
    uint8_t end_sheet_data[] = {0x92, 0x01, 0x00};
    memcpy(sheet + pos, end_sheet_data, 3);
    pos += 3;
    
    if (parser->merge_count > 0) {
        uint8_t hdr[10];
        int hlen = cxlsb_varint_encode(177, hdr);
        int slen = cxlsb_varint_encode(4, hdr + hlen);
        memcpy(sheet + pos, hdr, hlen + slen);
        pos += hlen + slen;
        
        uint32_t mc_count = parser->merge_count;
        memcpy(sheet + pos, &mc_count, 4);
        pos += 4;
        
        for (int i = 0; i < parser->merge_count; i++) {
            cxlsb_merge_cell* mc = &parser->merges[i];
            hlen = cxlsb_varint_encode(176, hdr);
            slen = cxlsb_varint_encode(16, hdr + hlen);
            memcpy(sheet + pos, hdr, hlen + slen);
            pos += hlen + slen;
            
            uint32_t vals[4] = {mc->row_first, mc->row_last, mc->col_first, mc->col_last};
            memcpy(sheet + pos, vals, 16);
            pos += 16;
        }
        
        hlen = cxlsb_varint_encode(178, hdr);
        slen = cxlsb_varint_encode(0, hdr + hlen);
        memcpy(sheet + pos, hdr, hlen + slen);
        pos += hlen + slen;
    }
    
    uint8_t end_sheet[] = {0x82, 0x01, 0x00};
    memcpy(sheet + pos, end_sheet, 3);
    pos += 3;
    
    *out_len = pos;
    return sheet;
}

cxlsb_error_t cxlsb_template_filler_save(cxlsb_template_filler_t* filler,
                                          const char* output_path) {
    if (!filler || !output_path) return CXLSB_ERR_NULL_PTR;
    
    int err;
    zip_t* out = zip_open(output_path, ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!out) return CXLSB_ERR_FILE_OPEN_FAILED;
    
    const char* static_entries[] = {
        "[Content_Types].xml",
        "_rels/.rels",
        "docProps/app.xml",
        "docProps/core.xml",
        "xl/_rels/workbook.bin.rels",
        "xl/styles.bin",
        "xl/theme/theme1.xml",
        "xl/workbook.bin"
    };
    
    for (int i = 0; i < 8; i++) {
        copy_entry(filler->template_zip, out, static_entries[i]);
    }
    
    uint32_t total_cnt = cxlsb_sst_table_get_count(filler->sst);
    size_t sst_estimated = 11 + 3;
    for (uint32_t i = 0; i < total_cnt; i++) {
        const char* str = cxlsb_sst_table_get_string(filler->sst, i);
        if (str) {
            size_t utf16_len = cxlsb_utf16le_byte_length(str);
            sst_estimated += 10 + 1 + 4 + utf16_len;
        }
    }
    if (sst_estimated < 1024) sst_estimated = 1024;
    
    uint8_t* new_sst = malloc(sst_estimated);
    if (!new_sst) {
        zip_close(out);
        return CXLSB_ERR_NO_MEMORY;
    }
    int sst_pos = 0;
    
    uint8_t sst_begin[] = {0x9F, 0x01, 0x08};
    memcpy(new_sst + sst_pos, sst_begin, 3);
    sst_pos += 3;
    
    uint32_t total_ref = cxlsb_sst_table_get_total_count(filler->sst);
    memcpy(new_sst + sst_pos, &total_ref, 4);
    memcpy(new_sst + sst_pos + 4, &total_cnt, 4);
    sst_pos += 8;
    
    for (uint32_t i = 0; i < total_cnt; i++) {
        write_sst_item(new_sst, &sst_pos, cxlsb_sst_table_get_string(filler->sst, i));
    }
    
    uint8_t sst_end[] = {0xA0, 0x01, 0x00};
    memcpy(new_sst + sst_pos, sst_end, 3);
    sst_pos += 3;
    
    zip_source_t* sst_src = zip_source_buffer(out, new_sst, sst_pos, 1);
    zip_file_add(out, "xl/sharedStrings.bin", sst_src, ZIP_FL_ENC_UTF_8);
    
    for (int i = 0; i < filler->sheet_count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "xl/worksheets/sheet%d.bin", i + 1);
        
        size_t sheet_len;
        uint8_t* sheet_data = generate_filled_sheet(filler, i, &sheet_len);
        
        zip_source_t* src = zip_source_buffer(out, sheet_data, sheet_len, 1);
        zip_file_add(out, path, src, ZIP_FL_ENC_UTF_8);
    }
    
    zip_close(out);
    return CXLSB_OK;
}