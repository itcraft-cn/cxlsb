#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <stdint.h>

int varint_decode(const uint8_t* buf, int* pos) {
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

int varint_encode(uint32_t val, uint8_t* out) {
    int n = 0;
    while (val >= 0x80) {
        out[n++] = (val & 0x7F) | 0x80;
        val >>= 7;
    }
    out[n++] = val;
    return n;
}

size_t utf8_to_utf16le(const char* utf8, uint8_t* utf16) {
    size_t len = 0;
    for (size_t i = 0; utf8[i]; i++) {
        uint8_t c = utf8[i];
        if (c < 0x80) {
            utf16[len++] = c;
            utf16[len++] = 0;
        } else if ((c & 0xE0) == 0xC0) {
            uint16_t ch = ((c & 0x1F) << 6) | (utf8[i+1] & 0x3F);
            utf16[len++] = ch & 0xFF;
            utf16[len++] = (ch >> 8) & 0xFF;
            i++;
        } else if ((c & 0xF0) == 0xE0) {
            uint16_t ch = ((c & 0x0F) << 12) | ((utf8[i+1] & 0x3F) << 6) | (utf8[i+2] & 0x3F);
            utf16[len++] = ch & 0xFF;
            utf16[len++] = (ch >> 8) & 0xFF;
            i += 2;
        }
    }
    return len;
}

void write_cell_st(uint8_t* buf, int* pos, int col, int xf_id, const char* str) {
    size_t str_len = strlen(str);
    size_t utf16_len = utf8_to_utf16le(str, buf + *pos + 13);
    uint32_t cell_sz = 8 + 1 + 4 + utf16_len;
    
    uint8_t hdr[10];
    int hlen = varint_encode(6, hdr);  // BrtCellSt type=6
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
    
    buf[*pos++] = 0;
    uint32_t sl = str_len;
    memcpy(buf + *pos, &sl, 4);
    *pos += 4;
    *pos += utf16_len;
}

void write_row_hdr(uint8_t* buf, int* pos, int row, int fc, int lc) {
    uint8_t hdr[10];
    int hlen = varint_encode(0, hdr);
    int slen = varint_encode(25, hdr + hlen);
    memcpy(buf + *pos, hdr, hlen + slen);
    *pos += hlen + slen;
    
    uint32_t r = row;
    memcpy(buf + *pos, &r, 4);
    memset(buf + *pos + 4, 0, 4);
    buf[*pos + 8] = 0x0E;
    buf[*pos + 9] = 0x01;
    memset(buf + *pos + 10, 0, 3);
    uint32_t cs = 1;
    memcpy(buf + *pos + 13, &cs, 4);
    uint32_t mic = fc, mac = lc;
    memcpy(buf + *pos + 17, &mic, 4);
    memcpy(buf + *pos + 21, &mac, 4);
    *pos += 25;
}

int copy_entry(zip_t* src, zip_t* dst, const char* name) {
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

int main(void) {
    printf("=== Fixed Template Fill ===\n");
    
    const char* data[][4] = {
        {"张三", "北京", "25", "男"},
        {"李四", "上海", "30", "女"},
        {"王五", "广州", "28", "男"}
    };
    
    int err;
    zip_t* tpl = zip_open("tests/resources/template/demo_template.xlsb", ZIP_RDONLY, &err);
    if (!tpl) { printf("Cannot open template\n"); return 1; }
    
    zip_file_t* sf = zip_fopen(tpl, "xl/worksheets/sheet1.bin", 0);
    uint8_t orig[1024];
    size_t orig_len = zip_fread(sf, orig, sizeof(orig));
    zip_fclose(sf);
    
    printf("Template sheet: %zu bytes\n", orig_len);
    
    uint8_t new_sheet[2048];
    memcpy(new_sheet, orig, 144);
    int pos = 144;
    
    new_sheet[pos++] = 0x91;
    new_sheet[pos++] = 0x01;
    
    printf("\nFilling rows 13-15, cols 8-11 with xf_id=2:\n");
    
    for (int row = 13; row <= 15; row++) {
        write_row_hdr(new_sheet, &pos, row, 8, 11);
        for (int col = 8; col <= 11; col++) {
            write_cell_st(new_sheet, &pos, col, 2, data[row-13][col-8]);
            printf("  Row %d, Col %d: %s (xf=2)\n", row, col, data[row-13][col-8]);
        }
    }
    
    new_sheet[pos++] = 0x92;
    new_sheet[pos++] = 0x01;
    
    memcpy(new_sheet + pos, orig + 344, orig_len - 344);
    pos += orig_len - 344;
    
    printf("New sheet: %d bytes\n", pos);
    
    zip_t* out = zip_open("/tmp/fill_fixed.xlsb", ZIP_CREATE | ZIP_TRUNCATE, &err);
    
    const char* entries[] = {
        "[Content_Types].xml", "_rels/.rels", "docProps/app.xml",
        "docProps/core.xml", "docProps/custom.xml", "xl/_rels/workbook.bin.rels",
        "xl/sharedStrings.bin", "xl/styles.bin", "xl/theme/theme1.xml", "xl/workbook.bin"
    };
    
    for (int i = 0; i < 10; i++) copy_entry(tpl, out, entries[i]);
    
    zip_source_t* ss = zip_source_buffer(out, new_sheet, pos, 0);
    zip_file_add(out, "xl/worksheets/sheet1.bin", ss, ZIP_FL_ENC_UTF_8);
    
    zip_close(tpl);
    zip_close(out);
    
    printf("\nOutput: /tmp/fill_fixed.xlsb\n");
    return 0;
}
