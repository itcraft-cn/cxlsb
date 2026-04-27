# cxlsb Phase 3: Util层实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现 VarInt 编解码、UTF-16LE 转换、XML 生成、关系文件生成

**Architecture:** 纯工具函数，无状态，可独立测试

**Tech Stack:** C99, 标准库函数

---

## Task 1: 创建 varint 头文件

**Files:**
- Create: `src/util/varint.h`

- [ ] **Step 1: 写入 varint.h 内容**

```c
#ifndef CXLSB_VARINT_H
#define CXLSB_VARINT_H

#include <stdint.h>
#include <stddef.h>

int cxlsb_varint_encode(uint32_t value, uint8_t* out);

int cxlsb_varint_decode(const uint8_t* buffer, size_t offset, uint32_t* out_value);

int cxlsb_varint_size(uint32_t value);

int cxlsb_varint_decode_at(const uint8_t* buffer, uint32_t* out_value, size_t* bytes_read);

#endif /* CXLSB_VARINT_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/util/varint.h
git commit -m "feat(util): add varint.h header"
```

---

## Task 2: 实现 varint

**Files:**
- Create: `src/util/varint.c`

- [ ] **Step 1: 写入 varint.c 内容**

```c
#include "varint.h"

int cxlsb_varint_encode(uint32_t value, uint8_t* out) {
    int count = 0;
    
    while (value >= 0x80) {
        out[count++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    out[count++] = (uint8_t)value;
    
    return count;
}

int cxlsb_varint_decode(const uint8_t* buffer, size_t offset, uint32_t* out_value) {
    uint32_t result = 0;
    int shift = 0;
    size_t pos = offset;
    int count = 0;
    
    while (true) {
        uint8_t byte = buffer[pos++];
        count++;
        
        result |= (uint32_t)(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            break;
        }
        
        shift += 7;
        if (shift >= 32) {
            return -1;
        }
    }
    
    *out_value = result;
    return count;
}

int cxlsb_varint_decode_at(const uint8_t* buffer, uint32_t* out_value, size_t* bytes_read) {
    uint32_t result = 0;
    int shift = 0;
    size_t pos = 0;
    
    while (true) {
        uint8_t byte = buffer[pos++];
        
        result |= (uint32_t)(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            break;
        }
        
        shift += 7;
        if (shift >= 32) {
            *bytes_read = pos;
            return -1;
        }
    }
    
    *out_value = result;
    *bytes_read = pos;
    return 0;
}

int cxlsb_varint_size(uint32_t value) {
    if (value < 0x80) return 1;
    if (value < 0x4000) return 2;
    if (value < 0x200000) return 3;
    if (value < 0x10000000) return 4;
    return 5;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/util/varint.c
git commit -m "feat(util): add varint.c implementation"
```

---

## Task 3: 创建 utf16 头文件

**Files:**
- Create: `src/util/utf16.h`

- [ ] **Step 1: 写入 utf16.h 内容**

```c
#ifndef CXLSB_UTF16_H
#define CXLSB_UTF16_H

#include <stdint.h>
#include <stddef.h>
#include "cxlsb/error_codes.h"

cxlsb_error_t cxlsb_utf8_to_utf16le(const char* utf8, 
                                      uint8_t* out_utf16, 
                                      size_t* out_length);

char* cxlsb_utf16le_to_utf8(const uint8_t* utf16, size_t length);

size_t cxlsb_utf16le_byte_length(const char* utf8);

#endif /* CXLSB_UTF16_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/util/utf16.h
git commit -m "feat(util): add utf16.h header"
```

---

## Task 4: 实现 utf16

**Files:**
- Create: `src/util/utf16.c`

- [ ] **Step 1: 写入 utf16.c 内容**

```c
#include "utf16.h"
#include <stdlib.h>
#include <string.h>

cxlsb_error_t cxlsb_utf8_to_utf16le(const char* utf8, 
                                      uint8_t* out_utf16, 
                                      size_t* out_length) {
    if (utf8 == NULL || out_utf16 == NULL || out_length == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    size_t utf8_len = strlen(utf8);
    size_t utf16_pos = 0;
    
    for (size_t i = 0; i < utf8_len; i++) {
        uint8_t c = (uint8_t)utf8[i];
        
        if (c < 0x80) {
            out_utf16[utf16_pos++] = c;
            out_utf16[utf16_pos++] = 0;
        } else if (c < 0xC0) {
            continue;
        } else if (c < 0xE0) {
            if (i + 1 < utf8_len) {
                uint16_t wc = ((c & 0x1F) << 6) | (utf8[i+1] & 0x3F);
                out_utf16[utf16_pos++] = (uint8_t)(wc & 0xFF);
                out_utf16[utf16_pos++] = (uint8_t)(wc >> 8);
                i++;
            }
        } else if (c < 0xF0) {
            if (i + 2 < utf8_len) {
                uint16_t wc = ((c & 0x0F) << 12) | 
                              ((utf8[i+1] & 0x3F) << 6) | 
                              (utf8[i+2] & 0x3F);
                out_utf16[utf16_pos++] = (uint8_t)(wc & 0xFF);
                out_utf16[utf16_pos++] = (uint8_t)(wc >> 8);
                i += 2;
            }
        } else {
            uint32_t wc = ((c & 0x07) << 18) | 
                          ((utf8[i+1] & 0x3F) << 12) | 
                          ((utf8[i+2] & 0x3F) << 6) | 
                          (utf8[i+3] & 0x3F);
            wc -= 0x10000;
            uint16_t high = (uint16_t)(0xD800 + (wc >> 10));
            uint16_t low = (uint16_t)(0xDC00 + (wc & 0x3FF));
            out_utf16[utf16_pos++] = (uint8_t)(high & 0xFF);
            out_utf16[utf16_pos++] = (uint8_t)(high >> 8);
            out_utf16[utf16_pos++] = (uint8_t)(low & 0xFF);
            out_utf16[utf16_pos++] = (uint8_t)(low >> 8);
            i += 3;
        }
    }
    
    *out_length = utf16_pos;
    return CXLSB_OK;
}

char* cxlsb_utf16le_to_utf8(const uint8_t* utf16, size_t length) {
    if (utf16 == NULL || length == 0) {
        return NULL;
    }
    
    size_t char_count = length / 2;
    char* utf8 = (char*)malloc(char_count * 3 + 1);
    if (utf8 == NULL) {
        return NULL;
    }
    
    size_t utf8_pos = 0;
    
    for (size_t i = 0; i < length; i += 2) {
        uint16_t wc = utf16[i] | (utf16[i+1] << 8);
        
        if (wc < 0x80) {
            utf8[utf8_pos++] = (char)wc;
        } else if (wc < 0x800) {
            utf8[utf8_pos++] = (char)(0xC0 | (wc >> 6));
            utf8[utf8_pos++] = (char)(0x80 | (wc & 0x3F));
        } else if (wc >= 0xD800 && wc <= 0xDBFF) {
            if (i + 3 < length) {
                uint16_t low = utf16[i+2] | (utf16[i+3] << 8);
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    uint32_t cp = 0x10000 + 
                                  ((wc - 0xD800) << 10) + 
                                  (low - 0xDC00);
                    utf8[utf8_pos++] = (char)(0xF0 | (cp >> 18));
                    utf8[utf8_pos++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                    utf8[utf8_pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    utf8[utf8_pos++] = (char)(0x80 | (cp & 0x3F));
                    i += 2;
                }
            }
        } else {
            utf8[utf8_pos++] = (char)(0xE0 | (wc >> 12));
            utf8[utf8_pos++] = (char)(0x80 | ((wc >> 6) & 0x3F));
            utf8[utf8_pos++] = (char)(0x80 | (wc & 0x3F));
        }
    }
    
    utf8[utf8_pos] = '\0';
    return utf8;
}

size_t cxlsb_utf16le_byte_length(const char* utf8) {
    if (utf8 == NULL) {
        return 0;
    }
    
    size_t len = strlen(utf8);
    size_t byte_count = 0;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t c = (uint8_t)utf8[i];
        
        if (c < 0x80) {
            byte_count += 2;
        } else if (c < 0xC0) {
            continue;
        } else if (c < 0xE0) {
            byte_count += 2;
            i++;
        } else if (c < 0xF0) {
            byte_count += 2;
            i += 2;
        } else {
            byte_count += 4;
            i += 3;
        }
    }
    
    return byte_count;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/util/utf16.c
git commit -m "feat(util): add utf16.c implementation"
```

---

## Task 5: 创建 xml_gen 头文件

**Files:**
- Create: `src/util/xml_gen.h`

- [ ] **Step 1: 写入 xml_gen.h 内容**

```c
#ifndef CXLSB_XML_GEN_H
#define CXLSB_XML_GEN_H

#include <stdint.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

cxlsb_error_t cxlsb_xml_gen_theme(uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_xml_gen_app(int sheet_count, uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_xml_gen_core(uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_xml_gen_content_types(int sheet_count, 
                                            bool has_sst,
                                            uint8_t** out, 
                                            size_t* out_length);

#endif /* CXLSB_XML_GEN_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/util/xml_gen.h
git commit -m "feat(util): add xml_gen.h header"
```

---

## Task 6: 实现 xml_gen

**Files:**
- Create: `src/util/xml_gen.c`

- [ ] **Step 1: 写入 xml_gen.c 内容**

```c
#include "xml_gen.h"
#include <stdlib.h>
#include <stdio.h>

static const char* THEME_XML = 
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"Office Theme\">\n"
"<a:themeElements>\n"
"<a:clrScheme name=\"Office\">\n"
"<a:dk1><a:sysClr val=\"windowText\" lastClr=\"000000\"/></a:dk1>\n"
"<a:lt1><a:sysClr val=\"window\" lastClr=\"FFFFFF\"/></a:lt1>\n"
"<a:dk2><a:srgbClr val=\"1F497D\"/></a:dk2>\n"
"<a:lt2><a:srgbClr val=\"EEECE1\"/></a:lt2>\n"
"<a:accent1><a:srgbClr val=\"4F81BD\"/></a:accent1>\n"
"<a:accent2><a:srgbClr val=\"C0504D\"/></a:accent2>\n"
"<a:accent3><a:srgbClr val=\"9BBB59\"/></a:accent3>\n"
"<a:accent4><a:srgbClr val=\"8064A2\"/></a:accent4>\n"
"<a:accent5><a:srgbClr val=\"4BACC6\"/></a:accent5>\n"
"<a:accent6><a:srgbClr val=\"F79646\"/></a:accent6>\n"
"<a:hlink><a:srgbClr val=\"0000FF\"/></a:hlink>\n"
"<a:folHlink><a:srgbClr val=\"800080\"/></a:folHlink>\n"
"</a:clrScheme>\n"
"<a:fontScheme name=\"Office\">\n"
"<a:majorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont>\n"
"<a:minorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont>\n"
"</a:fontScheme>\n"
"<a:fmtScheme name=\"Office\">\n"
"<a:fillStyleLst><a:noFill/><a:solidFill/><a:gradFill/></a:fillStyleLst>\n"
"<a:lnStyleLst><a:noLn/><a:ln w=\"25400\"/><a:ln w=\"25400\"/></a:lnStyleLst>\n"
"<a:effectStyleLst><a:effectStyle/><a:effectStyle/><a:effectStyle/></a:effectStyleLst>\n"
"</a:fmtScheme>\n"
"</a:themeElements>\n"
"</a:theme>\n";

static const char* CORE_XML =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\""
" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
" xmlns:dcterms=\"http://purl.org/dc/terms/\""
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
"<dc:creator>cxlsb</dc:creator>\n"
"<dcterms:created xsi:type=\"dcterms:W3CDTF\">2026-04-27T00:00:00Z</dcterms:created>\n"
"</cp:coreProperties>\n";

cxlsb_error_t cxlsb_xml_gen_theme(uint8_t** out, size_t* out_length) {
    size_t len = strlen(THEME_XML);
    uint8_t* buffer = (uint8_t*)malloc(len + 1);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(buffer, THEME_XML, len);
    buffer[len] = '\0';
    
    *out = buffer;
    *out_length = len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_xml_gen_app(int sheet_count, uint8_t** out, size_t* out_length) {
    char* buffer = (char*)malloc(1024);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int len = snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">\n"
        "<Application>cxlsb</Application>\n"
        "<Sheets>%d</Sheets>\n"
        "</Properties>\n",
        sheet_count);
    
    if (len < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_xml_gen_core(uint8_t** out, size_t* out_length) {
    size_t len = strlen(CORE_XML);
    uint8_t* buffer = (uint8_t*)malloc(len + 1);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(buffer, CORE_XML, len);
    buffer[len] = '\0';
    
    *out = buffer;
    *out_length = len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_xml_gen_content_types(int sheet_count, 
                                            bool has_sst,
                                            uint8_t** out, 
                                            size_t* out_length) {
    char* buffer = (char*)malloc(4096);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int pos = snprintf(buffer, 4096,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        "<Override PartName=\"/docProps/app.xml\""
        " ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>\n"
        "<Override PartName=\"/docProps/core.xml\""
        " ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>\n"
        "<Override PartName=\"/xl/workbook.bin\""
        " ContentType=\"application/vnd.ms-excel.workbook.binary\"/>\n"
        "<Override PartName=\"/xl/styles.bin\""
        " ContentType=\"application/vnd.ms-excel.styles\"/>\n"
        "<Override PartName=\"/xl/theme/theme1.xml\""
        " ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>\n");
    
    if (pos < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    if (has_sst) {
        pos += snprintf(buffer + pos, 4096 - pos,
            "<Override PartName=\"/xl/sharedStrings.bin\""
            " ContentType=\"application/vnd.ms-excel.sharedStrings\"/>\n");
    }
    
    for (int i = 1; i <= sheet_count; i++) {
        pos += snprintf(buffer + pos, 4096 - pos,
            "<Override PartName=\"/xl/worksheets/sheet%d.bin\""
            " ContentType=\"application/vnd.ms-excel.worksheet\"/>\n", i);
    }
    
    pos += snprintf(buffer + pos, 4096 - pos, "</Types>\n");
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)pos;
    
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/util/xml_gen.c
git commit -m "feat(util): add xml_gen.c implementation"
```

---

## Task 7: 创建 rels_gen 头文件

**Files:**
- Create: `src/util/rels_gen.h`

- [ ] **Step 1: 写入 rels_gen.h 内容**

```c
#ifndef CXLSB_RELS_GEN_H
#define CXLSB_RELS_GEN_H

#include <stdint.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

cxlsb_error_t cxlsb_rels_gen_root(uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_rels_gen_workbook(int sheet_count, 
                                        bool has_sst,
                                        uint8_t** out, 
                                        size_t* out_length);

#endif /* CXLSB_RELS_GEN_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/util/rels_gen.h
git commit -m "feat(util): add rels_gen.h header"
```

---

## Task 8: 实现 rels_gen

**Files:**
- Create: `src/util/rels_gen.c`

- [ ] **Step 1: 写入 rels_gen.c 内容**

```c
#include "rels_gen.h"
#include <stdlib.h>
#include <stdio.h>

cxlsb_error_t cxlsb_rels_gen_root(uint8_t** out, size_t* out_length) {
    char* buffer = (char*)malloc(512);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int len = snprintf(buffer, 512,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.bin\"/>\n"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>\n"
        "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>\n"
        "</Relationships>\n");
    
    if (len < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_rels_gen_workbook(int sheet_count, 
                                        bool has_sst,
                                        uint8_t** out, 
                                        size_t* out_length) {
    char* buffer = (char*)malloc(2048);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int pos = snprintf(buffer, 2048,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n");
    
    if (pos < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    int rId = 1;
    
    for (int i = 1; i <= sheet_count; i++) {
        pos += snprintf(buffer + pos, 2048 - pos,
            "<Relationship Id=\"rId%d\""
            " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\""
            " Target=\"worksheets/sheet%d.bin\"/>\n", rId, i);
        rId++;
    }
    
    pos += snprintf(buffer + pos, 2048 - pos,
        "<Relationship Id=\"rId%d\""
        " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\""
        " Target=\"styles.bin\"/>\n", rId);
    rId++;
    
    pos += snprintf(buffer + pos, 2048 - pos,
        "<Relationship Id=\"rId%d\""
        " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\""
        " Target=\"theme/theme1.xml\"/>\n", rId);
    rId++;
    
    if (has_sst) {
        pos += snprintf(buffer + pos, 2048 - pos,
            "<Relationship Id=\"rId%d\""
            " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings\""
            " Target=\"sharedStrings.bin\"/>\n", rId);
    }
    
    pos += snprintf(buffer + pos, 2048 - pos, "</Relationships>\n");
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)pos;
    
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/util/rels_gen.c
git commit -m "feat(util): add rels_gen.c implementation"
```

---

## Task 9: 创建 Util 层测试

**Files:**
- Create: `test/test_util.c`

- [ ] **Step 1: 写入测试文件内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "src/util/varint.h"
#include "src/util/utf16.h"
#include "src/util/xml_gen.h"
#include "src/util/rels_gen.h"

static void test_varint_encode_decode(void) {
    uint8_t buf[10];
    
    int len = cxlsb_varint_encode(127, buf);
    assert(len == 1);
    assert(buf[0] == 127);
    
    uint32_t val;
    int dec_len = cxlsb_varint_decode(buf, 0, &val);
    assert(dec_len == 1);
    assert(val == 127);
    printf("test_varint_small: PASS\n");
    
    len = cxlsb_varint_encode(300, buf);
    assert(len == 2);
    
    dec_len = cxlsb_varint_decode(buf, 0, &val);
    assert(dec_len == 2);
    assert(val == 300);
    printf("test_varint_medium: PASS\n");
    
    len = cxlsb_varint_encode(100000, buf);
    assert(len >= 2);
    
    dec_len = cxlsb_varint_decode(buf, 0, &val);
    assert(val == 100000);
    printf("test_varint_large: PASS\n");
}

static void test_utf16_convert(void) {
    const char* text = "Hello";
    uint8_t utf16[20];
    size_t utf16_len;
    
    cxlsb_error_t err = cxlsb_utf8_to_utf16le(text, utf16, &utf16_len);
    assert(err == CXLSB_OK);
    assert(utf16_len == 10);
    
    assert(utf16[0] == 'H');
    assert(utf16[1] == 0);
    assert(utf16[2] == 'e');
    assert(utf16[3] == 0);
    
    char* back = cxlsb_utf16le_to_utf8(utf16, utf16_len);
    assert(back != NULL);
    assert(strcmp(back, text) == 0);
    free(back);
    
    printf("test_utf16_convert: PASS\n");
}

static void test_xml_gen(void) {
    uint8_t* theme;
    size_t theme_len;
    
    cxlsb_error_t err = cxlsb_xml_gen_theme(&theme, &theme_len);
    assert(err == CXLSB_OK);
    assert(theme_len > 0);
    assert(strncmp((char*)theme, "<?xml", 5) == 0);
    free(theme);
    
    uint8_t* app;
    size_t app_len;
    err = cxlsb_xml_gen_app(3, &app, &app_len);
    assert(err == CXLSB_OK);
    free(app);
    
    uint8_t* core;
    size_t core_len;
    err = cxlsb_xml_gen_core(&core, &core_len);
    assert(err == CXLSB_OK);
    free(core);
    
    printf("test_xml_gen: PASS\n");
}

static void test_rels_gen(void) {
    uint8_t* root;
    size_t root_len;
    
    cxlsb_error_t err = cxlsb_rels_gen_root(&root, &root_len);
    assert(err == CXLSB_OK);
    assert(root_len > 0);
    free(root);
    
    uint8_t* wb;
    size_t wb_len;
    err = cxlsb_rels_gen_workbook(3, true, &wb, &wb_len);
    assert(err == CXLSB_OK);
    assert(wb_len > 0);
    free(wb);
    
    printf("test_rels_gen: PASS\n");
}

int main(void) {
    test_varint_encode_decode();
    test_utf16_convert();
    test_xml_gen();
    test_rels_gen();
    
    printf("\nAll util tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add test/test_util.c
git commit -m "test(util): add util tests"
```

---

## Task 10: 构建并运行测试

- [ ] **Step 1: 重新构建**

```bash
cd build && cmake .. && make
```

- [ ] **Step 2: 运行测试**

```bash
./test_util
```
Expected: 输出 "All util tests passed!"

---

## Phase 3 完成检查

- [ ] varint 编解码已实现
- [ ] UTF-16LE 转换已实现
- [ ] XML 生成已实现
- [ ] 关系文件生成已实现
- [ ] 测试已通过
- [ ] Git 已提交

**下一步:** 进入 Phase 4 - Format层基础实现