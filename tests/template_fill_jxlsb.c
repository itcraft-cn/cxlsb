#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <stdint.h>
#include "../src/format/sheet_parser.h"
#include "../src/format/sst_table.h"

typedef cxlsb_sst_table_t cxlsb_sst_table;

static int varint_encode(uint32_t val, uint8_t* out) {
    int n = 0;
    while (val >= 0x80) {
        out[n++] = (val & 0x7F) | 0x80;
        val >>= 7;
    }
    out[n++] = val;
    return n;
}

static void write_cell_isst(uint8_t* buf, int* pos, int col, int xf_id, int sst_idx) {
    uint32_t cell_sz = 12;
    
    uint8_t hdr[10];
    int hlen = varint_encode(7, hdr);
    int slen = varint_encode(cell_sz, hdr + hlen);
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

static void write_row_hdr(uint8_t* buf, int* pos, int row, int col_count) {
    int num_spans = (col_count > 0) ? ((col_count - 1) / 1024 + 1) : 1;
    int record_size = 4 + 4 + 2 + 3 + 4 + (num_spans * 8);
    
    uint8_t hdr[10];
    int hlen = varint_encode(0, hdr);
    int slen = varint_encode(record_size, hdr + hlen);
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
        int seg_start = seg * 1024;
        int seg_end = (seg + 1) * 1024 - 1;
        if (seg_end >= col_count) seg_end = col_count - 1;
        uint32_t mic = seg_start, mac = seg_end;
        memcpy(buf + *pos + 17 + seg * 8, &mic, 4);
        memcpy(buf + *pos + 21 + seg * 8, &mac, 4);
    }
    *pos += 17 + num_spans * 8;
}

static void write_ws_dim(uint8_t* buf, int* pos, int r1, int r2, int c1, int c2) {
    uint8_t hdr[10];
    int hlen = varint_encode(148, hdr);
    int slen = varint_encode(16, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t vals[4] = {r1, r2, c1, c2};
    memcpy(buf + *pos, vals, 16);
    *pos += 16;
}

static int is_outside_fill_area(int cell_row, int cell_col, int start_row, int row_count, int start_col, int col_count) {
    return cell_row < start_row || cell_row >= start_row + row_count ||
           cell_col < start_col || cell_col >= start_col + col_count;
}

static int copy_entry(zip_t* src, zip_t* dst, const char* name) {
    zip_file_t* f = zip_fopen(src, name, 0);
    if (!f) return -1;
    zip_stat_t st;
    zip_stat(src, name, 0, &st);
    uint8_t* d = malloc(st.size);
    zip_fread(f, d, st.size);
    zip_fclose(f);
    zip_source_t* s = zip_source_buffer(dst, d, st.size, 1);
    zip_file_add(dst, name, s, ZIP_FL_ENC_UTF_8);
    return 0;
}

static size_t write_sst_item(uint8_t* buf, int* pos, const char* str) {
    size_t str_len = 0;
    for (size_t i = 0; str[i]; i++) {
        uint8_t c = str[i];
        if (c < 0x80 || (c & 0xC0) == 0xC0) str_len++;
    }
    
    size_t utf16_len = str_len * 2;
    size_t item_size = 1 + 4 + utf16_len;
    
    uint8_t hdr[10];
    int hlen = varint_encode(19, hdr);
    int slen = varint_encode(item_size, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    int hdr_size = hlen + slen;
    *pos += hdr_size;
    
    buf[(*pos)++] = 0;
    uint32_t sl = str_len;
    memcpy(buf + *pos, &sl, 4);
    *pos += 4;
    
    for (size_t i = 0; str[i]; i++) {
        uint8_t c = str[i];
        if (c < 0x80) {
            buf[*pos] = c; buf[*pos+1] = 0;
            *pos += 2;
        } else if ((c & 0xE0) == 0xC0) {
            uint16_t ch = ((c & 0x1F) << 6) | (str[i+1] & 0x3F);
            buf[*pos] = ch & 0xFF; buf[*pos+1] = (ch >> 8) & 0xFF;
            *pos += 2;
            i++;
        } else if ((c & 0xF0) == 0xE0) {
            uint16_t ch = ((c & 0x0F) << 12) | ((str[i+1] & 0x3F) << 6) | (str[i+2] & 0x3F);
            buf[*pos] = ch & 0xFF; buf[*pos+1] = (ch >> 8) & 0xFF;
            *pos += 2;
            i += 2;
        }
    }
    
    return hdr_size + item_size;
}

int main(void) {
    printf("=== Template Fill (jxlsb approach - CellIsst) ===\n");
    
    const char* fill_data[][4] = {
        {"张三", "北京", "25", "男"},
        {"李四", "上海", "30", "女"},
        {"王五", "广州", "28", "男"}
    };
    int fill_row_count = 3;
    int fill_col_count = 4;
    
    int err;
    zip_t* tpl = zip_open("tests/resources/template/demo_template.xlsb", ZIP_RDONLY, &err);
    if (!tpl) { printf("Cannot open template\n"); return 1; }
    
    zip_file_t* sf = zip_fopen(tpl, "xl/worksheets/sheet1.bin", 0);
    if (!sf) { printf("Cannot open sheet\n"); return 1; }
    zip_stat_t sheet_st;
    zip_stat(tpl, "xl/worksheets/sheet1.bin", 0, &sheet_st);
    uint8_t* orig_sheet = malloc(sheet_st.size);
    size_t sheet_len = zip_fread(sf, orig_sheet, sheet_st.size);
    zip_fclose(sf);
    
    zip_file_t* sstf = zip_fopen(tpl, "xl/sharedStrings.bin", 0);
    if (!sstf) { printf("Cannot open SST\n"); return 1; }
    zip_stat_t sst_st;
    zip_stat(tpl, "xl/sharedStrings.bin", 0, &sst_st);
    uint8_t* sst_data = malloc(sst_st.size);
    size_t sst_len = zip_fread(sstf, sst_data, sst_st.size);
    zip_fclose(sstf);
    
    cxlsb_sst_table_t* sst = cxlsb_sst_table_create(256);
    cxlsb_sst_table_load(sst, sst_data, sst_len);
    
    int sst_count = cxlsb_sst_table_get_count(sst);
    printf("Template SST count: %d\n", sst_count);
    for (int i = 0; i < sst_count; i++) {
        printf("  SST[%d] = '%s'\n", i, cxlsb_sst_table_get_string(sst, i));
    }
    
    const char** sst_strings = malloc(sst_count * sizeof(char*));
    for (int i = 0; i < sst_count; i++) {
        sst_strings[i] = cxlsb_sst_table_get_string(sst, i);
    }
    
    cxlsb_sheet_parser* parser = cxlsb_sheet_parser_create(orig_sheet, sheet_len, sst_strings, sst_count);
    
    printf("Template parsed: %d cells, max_row=%d, max_col=%d, %d merges\n",
           parser->cell_count, parser->max_row, parser->max_col, parser->merge_count);
    
    int marker_row = -1;
    int marker_col = -1;
    for (int i = 0; i < parser->cell_count; i++) {
        cxlsb_cell_info* ci = &parser->cells[i];
        if (ci->type == 1 && ci->data.text && strcmp(ci->data.text, "${data}") == 0) {
            marker_row = ci->row;
            marker_col = ci->col;
            printf("Found ${data} marker at row=%d, col=%d\n", marker_row, marker_col);
            break;
        }
    }
    
    if (marker_row < 0) {
        printf("Marker ${data} not found!\n");
        return 1;
    }
    
    int start_row = marker_row;
    int start_col = marker_col;
    
    int sst_idx[3][4];
    for (int r = 0; r < fill_row_count; r++) {
        for (int c = 0; c < fill_col_count; c++) {
            sst_idx[r][c] = cxlsb_sst_table_add_string(sst, fill_data[r][c]);
            printf("Added '%s' to SST at index %d\n", fill_data[r][c], sst_idx[r][c]);
        }
    }
    
    int total_rows = (parser->max_row + 1 > start_row + fill_row_count) ? parser->max_row + 1 : start_row + fill_row_count;
    int total_cols = (parser->max_col + 1 > start_col + fill_col_count) ? parser->max_col + 1 : start_col + fill_col_count;
    
    uint8_t new_sheet[4096];
    int pos = 0;
    
    uint8_t begin_sheet[] = {0x81, 0x01, 0x00};
    memcpy(new_sheet + pos, begin_sheet, 3);
    pos += 3;
    
    uint8_t wsprop[] = {0x93, 0x01, 0x17, 0xC9, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    memcpy(new_sheet + pos, wsprop, 26);
    pos += 26;
    
    write_ws_dim(new_sheet, &pos, parser->cells[0].row, start_row + fill_row_count - 1, start_col, start_col + fill_col_count - 1);
    
    uint8_t wsviews_begin[] = {0x85, 0x01, 0x00};
    memcpy(new_sheet + pos, wsviews_begin, 3);
    pos += 3;
    
    uint8_t wsview_hdr[] = {0x89, 0x01, 0x1E};
    memcpy(new_sheet + pos, wsview_hdr, 3);
    pos += 3;
    
    uint8_t wsview_data[] = {0xDC, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x40, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(new_sheet + pos, wsview_data, 30);
    pos += 30;
    
    uint8_t sel_hdr[] = {0x98, 0x01, 0x24};
    memcpy(new_sheet + pos, sel_hdr, 3);
    pos += 3;
    
    uint8_t sel_data[] = {0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
                          0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
                          0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
                          0x07, 0x00, 0x00, 0x00};
    memcpy(new_sheet + pos, sel_data, 36);
    pos += 36;
    
    uint8_t wsview_end[] = {0x8A, 0x01, 0x00};
    memcpy(new_sheet + pos, wsview_end, 3);
    pos += 3;
    
    uint8_t wsviews_end[] = {0x86, 0x01, 0x00};
    memcpy(new_sheet + pos, wsviews_end, 3);
    pos += 3;
    
    uint8_t wsfmt_hdr[] = {0xE5, 0x03, 0x0C};
    memcpy(new_sheet + pos, wsfmt_hdr, 3);
    pos += 3;
    
    uint8_t wsfmt_data[] = {0x00, 0x09, 0x00, 0x00, 0x08, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00};
    memcpy(new_sheet + pos, wsfmt_data, 12);
    pos += 12;
    
    uint8_t begin_sheet_data[] = {0x91, 0x01, 0x00};
    memcpy(new_sheet + pos, begin_sheet_data, 3);
    pos += 3;
    
    printf("\nWriting rows (marker row %d will be replaced):\n", marker_row);
    
    for (int row = parser->cells[0].row; row < total_rows; row++) {
        write_row_hdr(new_sheet, &pos, row, total_cols);
        
        if (row == marker_row) {
            printf("  Row %d: REPLACE marker row with data row 0 (xf=0 default format)\n", row);
            for (int col = start_col; col < start_col + fill_col_count; col++) {
                int data_col = col - start_col;
                write_cell_isst(new_sheet, &pos, col, 0, sst_idx[0][data_col]);
                printf("    col=%d: '%s' (xf=0, sst_idx=%d)\n", col, fill_data[0][data_col], sst_idx[0][data_col]);
            }
        } else if (row >= start_row + 1 && row < start_row + fill_row_count) {
            int data_idx = row - start_row;
            printf("  Row %d: fill data row %d (xf=0 default format)\n", row, data_idx);
            for (int col = start_col; col < start_col + fill_col_count; col++) {
                int data_col = col - start_col;
                write_cell_isst(new_sheet, &pos, col, 0, sst_idx[data_idx][data_col]);
                printf("    col=%d: '%s' (xf=0, sst_idx=%d)\n", col, fill_data[data_idx][data_col], sst_idx[data_idx][data_col]);
            }
        } else {
            for (int i = 0; i < parser->cell_count; i++) {
                cxlsb_cell_info* ci = &parser->cells[i];
                if (ci->row == row) {
                    printf("  Row %d: preserve template cell col=%d xf=%d\n", row, ci->col, ci->style_index);
                    if (ci->type == 1) {
                        int idx = cxlsb_sst_table_find_string(sst, ci->data.text);
                        if (idx >= 0) {
                            write_cell_isst(new_sheet, &pos, ci->col, ci->style_index, idx);
                        }
                    }
                }
            }
        }
    }
    
    uint8_t end_sheet_data[] = {0x92, 0x01, 0x00};
    memcpy(new_sheet + pos, end_sheet_data, 3);
    pos += 3;
    
    if (parser->merge_count > 0) {
        uint8_t hdr[10];
        int hlen = varint_encode(177, hdr);
        int slen = varint_encode(4, hdr + hlen);
        memcpy(new_sheet + pos, hdr, hlen + slen);
        pos += hlen + slen;
        
        uint32_t mc_count = parser->merge_count;
        memcpy(new_sheet + pos, &mc_count, 4);
        pos += 4;
        
        for (int i = 0; i < parser->merge_count; i++) {
            cxlsb_merge_cell* mc = &parser->merges[i];
            hlen = varint_encode(176, hdr);
            slen = varint_encode(16, hdr + hlen);
            memcpy(new_sheet + pos, hdr, hlen + slen);
            pos += hlen + slen;
            
            uint32_t vals[4] = {mc->row_first, mc->row_last, mc->col_first, mc->col_last};
            memcpy(new_sheet + pos, vals, 16);
            pos += 16;
        }
        
        hlen = varint_encode(178, hdr);
        slen = varint_encode(0, hdr + hlen);
        memcpy(new_sheet + pos, hdr, hlen + slen);
        pos += hlen + slen;
    }
    
    uint8_t end_sheet[] = {0x82, 0x01, 0x00};
    memcpy(new_sheet + pos, end_sheet, 3);
    pos += 3;
    
    printf("New sheet: %d bytes\n", pos);
    
    uint8_t new_sst[4096];
    int sst_pos = 0;
    
    uint8_t sst_begin[] = {0x9F, 0x01, 0x08};
    memcpy(new_sst + sst_pos, sst_begin, 3);
    sst_pos += 3;
    
    uint32_t total_ref = cxlsb_sst_table_get_total_count(sst);
    uint32_t total_cnt = cxlsb_sst_table_get_count(sst);
    memcpy(new_sst + sst_pos, &total_ref, 4);
    memcpy(new_sst + sst_pos + 4, &total_cnt, 4);
    sst_pos += 8;
    
    for (uint32_t i = 0; i < total_cnt; i++) {
        write_sst_item(new_sst, &sst_pos, cxlsb_sst_table_get_string(sst, i));
    }
    
    uint8_t sst_end[] = {0xA0, 0x01, 0x00};
    memcpy(new_sst + sst_pos, sst_end, 3);
    sst_pos += 3;
    
    printf("New SST: %d bytes (%d strings)\n", sst_pos, total_cnt);
    
    zip_t* out = zip_open("/tmp/fill_jxlsb.xlsb", ZIP_CREATE | ZIP_TRUNCATE, &err);
    
    const char* entries[] = {
        "[Content_Types].xml", "_rels/.rels", "docProps/app.xml",
        "docProps/core.xml", "docProps/custom.xml", "xl/_rels/workbook.bin.rels",
        "xl/styles.bin", "xl/theme/theme1.xml", "xl/workbook.bin"
    };
    
    for (int i = 0; i < 9; i++) copy_entry(tpl, out, entries[i]);
    
    zip_source_t* ss = zip_source_buffer(out, new_sheet, pos, 0);
    zip_file_add(out, "xl/worksheets/sheet1.bin", ss, ZIP_FL_ENC_UTF_8);
    
    zip_source_t* ssts = zip_source_buffer(out, new_sst, sst_pos, 0);
    zip_file_add(out, "xl/sharedStrings.bin", ssts, ZIP_FL_ENC_UTF_8);
    
    cxlsb_sheet_parser_destroy(parser);
    free(sst_strings);
    cxlsb_sst_table_destroy(sst);
    free(orig_sheet);
    free(sst_data);
    zip_close(tpl);
    zip_close(out);
    
    printf("\nOutput: /tmp/fill_jxlsb.xlsb\n");
    return 0;
}