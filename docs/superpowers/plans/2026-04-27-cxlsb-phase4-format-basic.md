# cxlsb Phase 4: Format层基础（BIFF12 + SST）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现 BIFF12 记录类型常量、BIFF12 写入器、共享字符串表（SST）

**Architecture:** VarInt 记录头格式，SST 带哈希优化查找

**Tech Stack:** C99, VarInt 编码，动态数组

---

## Task 1: 创建 BIFF12 记录类型头文件

**Files:**
- Create: `src/format/biff12_record_types.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_BIFF12_RECORD_TYPES_H
#define CXLSB_BIFF12_RECORD_TYPES_H

#define BIFF12_BRT_ROW_HDR           0
#define BIFF12_BRT_CELL_BLANK        1
#define BIFF12_BRT_CELL_RK           2
#define BIFF12_BRT_CELL_ERROR        3
#define BIFF12_BRT_CELL_BOOL         4
#define BIFF12_BRT_CELL_REAL         5
#define BIFF12_BRT_CELL_ST           6
#define BIFF12_BRT_CELL_ISST         7

#define BIFF12_BRT_SST_ITEM          19

#define BIFF12_BRT_FILE_VERSION      128
#define BIFF12_BRT_BEGIN_SHEET       129
#define BIFF12_BRT_END_SHEET         130
#define BIFF12_BRT_BEGIN_BOOK        131
#define BIFF12_BRT_END_BOOK          132
#define BIFF12_BRT_BEGIN_WS_VIEWS    133
#define BIFF12_BRT_END_WS_VIEWS      134
#define BIFF12_BRT_BEGIN_BOOK_VIEWS  135
#define BIFF12_BRT_END_BOOK_VIEWS    136
#define BIFF12_BRT_BEGIN_WS_VIEW     137
#define BIFF12_BRT_END_WS_VIEW       138
#define BIFF12_BRT_BEGIN_BUNDLE_SHS  143
#define BIFF12_BRT_END_BUNDLE_SHS    144
#define BIFF12_BRT_BEGIN_SHEET_DATA  145
#define BIFF12_BRT_END_SHEET_DATA    146
#define BIFF12_BRT_WS_PROP           147
#define BIFF12_BRT_WS_DIM            148
#define BIFF12_BRT_WB_PROP           153
#define BIFF12_BRT_BUNDLE_SH         156
#define BIFF12_BRT_BOOK_VIEW         158
#define BIFF12_BRT_BEGIN_SST         159
#define BIFF12_BRT_END_SST           160

#define BIFF12_BRT_FMT               44
#define BIFF12_BRT_FONT              43
#define BIFF12_BRT_FILL              45
#define BIFF12_BRT_BORDER            46
#define BIFF12_BRT_XF                47
#define BIFF12_BRT_BEGIN_STYLE_SHEET 370
#define BIFF12_BRT_END_STYLE_SHEET   371

#define BIFF12_BRT_MERGE_CELL        176
#define BIFF12_BRT_BEGIN_MERGE_CELLS 177
#define BIFF12_BRT_END_MERGE_CELLS   178

#endif /* CXLSB_BIFF12_RECORD_TYPES_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/biff12_record_types.h
git commit -m "feat(format): add BIFF12 record type constants"
```

---

## Task 2: 创建 biff12_writer 头文件

**Files:**
- Create: `src/format/biff12_writer.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_BIFF12_WRITER_H
#define CXLSB_BIFF12_WRITER_H

#include "../io/buffer_writer.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_biff12_writer {
    cxlsb_buffer_writer_t* buffer;
} cxlsb_biff12_writer_t;

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create(size_t initial_capacity);

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create_fixed(size_t capacity);

void cxlsb_biff12_writer_destroy(cxlsb_biff12_writer_t* writer);

cxlsb_error_t cxlsb_biff12_writer_write_record_header(cxlsb_biff12_writer_t* writer,
                                                       uint32_t record_type,
                                                       uint32_t record_size);

cxlsb_error_t cxlsb_biff12_writer_write_empty_record(cxlsb_biff12_writer_t* writer,
                                                      uint32_t record_type);

cxlsb_error_t cxlsb_biff12_writer_write_varint(cxlsb_biff12_writer_t* writer, 
                                                uint32_t value);

cxlsb_error_t cxlsb_biff12_writer_write_wide_string(cxlsb_biff12_writer_t* writer,
                                                     const char* str);

cxlsb_error_t cxlsb_biff12_writer_write_cell_real(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   double value, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_rk(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 double value, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_st(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 const char* str, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_isst(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   uint32_t sst_index, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_bool(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   bool value, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_blank(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, uint32_t col,
                                                    uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_row_header(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, 
                                                    uint32_t first_col,
                                                    uint32_t last_col);

cxlsb_error_t cxlsb_biff12_writer_write_ws_dim(cxlsb_biff12_writer_t* writer,
                                                uint32_t first_row,
                                                uint32_t first_col,
                                                uint32_t last_row,
                                                uint32_t last_col);

uint8_t* cxlsb_biff12_writer_get_buffer(cxlsb_biff12_writer_t* writer);

size_t cxlsb_biff12_writer_get_length(cxlsb_biff12_writer_t* writer);

void cxlsb_biff12_writer_reset(cxlsb_biff12_writer_t* writer);

#endif /* CXLSB_BIFF12_WRITER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/biff12_writer.h
git commit -m "feat(format): add biff12_writer.h header"
```

---

## Task 3: 实现 biff12_writer

**Files:**
- Create: `src/format/biff12_writer.c`

- [ ] **Step 1: 写入内容**

```c
#include "biff12_writer.h"
#include "biff12_record_types.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include <stdlib.h>
#include <math.h>

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create(size_t initial_capacity) {
    cxlsb_biff12_writer_t* writer = (cxlsb_biff12_writer_t*)malloc(sizeof(cxlsb_biff12_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    writer->buffer = cxlsb_buffer_writer_create(initial_capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    
    return writer;
}

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create_fixed(size_t capacity) {
    cxlsb_biff12_writer_t* writer = (cxlsb_biff12_writer_t*)malloc(sizeof(cxlsb_biff12_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    writer->buffer = cxlsb_buffer_writer_create_fixed(capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    
    return writer;
}

void cxlsb_biff12_writer_destroy(cxlsb_biff12_writer_t* writer) {
    if (writer != NULL) {
        if (writer->buffer != NULL) {
            cxlsb_buffer_writer_destroy(writer->buffer);
        }
        free(writer);
    }
}

cxlsb_error_t cxlsb_biff12_writer_write_record_header(cxlsb_biff12_writer_t* writer,
                                                       uint32_t record_type,
                                                       uint32_t record_size) {
    uint8_t type_buf[5];
    uint8_t size_buf[5];
    
    int type_len = cxlsb_varint_encode(record_type, type_buf);
    int size_len = cxlsb_varint_encode(record_size, size_buf);
    
    cxlsb_error_t err = cxlsb_buffer_writer_write_bytes(writer->buffer, type_buf, type_len);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_bytes(writer->buffer, size_buf, size_len);
}

cxlsb_error_t cxlsb_biff12_writer_write_empty_record(cxlsb_biff12_writer_t* writer,
                                                      uint32_t record_type) {
    return cxlsb_biff12_writer_write_record_header(writer, record_type, 0);
}

cxlsb_error_t cxlsb_biff12_writer_write_varint(cxlsb_biff12_writer_t* writer, 
                                                uint32_t value) {
    uint8_t buf[5];
    int len = cxlsb_varint_encode(value, buf);
    return cxlsb_buffer_writer_write_bytes(writer->buffer, buf, len);
}

cxlsb_error_t cxlsb_biff12_writer_write_wide_string(cxlsb_biff12_writer_t* writer,
                                                     const char* str) {
    if (str == NULL) {
        return cxlsb_buffer_writer_write_int(writer->buffer, 0);
    }
    
    size_t str_len = strlen(str);
    cxlsb_error_t err = cxlsb_buffer_writer_write_int(writer->buffer, (uint32_t)str_len);
    if (err != CXLSB_OK) return err;
    
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
    if (utf16_buf == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    err = cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
    if (err != CXLSB_OK) {
        free(utf16_buf);
        return err;
    }
    
    err = cxlsb_buffer_writer_write_bytes(writer->buffer, utf16_buf, utf16_len);
    free(utf16_buf);
    return err;
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_real(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   double value, uint32_t xf_id) {
    uint32_t record_size = 4 + 4 + 8;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer, 
                                                                 BIFF12_BRT_CELL_REAL, 
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, col);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_double(writer->buffer, value);
}

static bool can_encode_as_rk(double value) {
    if (value == 0.0) return true;
    
    double abs_val = fabs(value);
    if (abs_val < 1e-10) return false;
    
    double scaled = value * 100.0;
    int64_t int_val = (int64_t)round(scaled);
    if (scaled != (double)int_val) return false;
    
    return (int_val >= -536870912 && int_val <= 536870911);
}

static uint32_t encode_rk(double value) {
    int64_t int_val = (int64_t)round(value * 100.0);
    uint32_t rk = (uint32_t)(int_val & 0x1FFFFFFF);
    rk |= 0x80000000;
    rk |= 0x02000000;
    return rk;
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_rk(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 double value, uint32_t xf_id) {
    uint32_t record_size = 4 + 4 + 4;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_CELL_RK,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, col);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    if (err != CXLSB_OK) return err;
    
    uint32_t rk = encode_rk(value);
    return cxlsb_buffer_writer_write_int(writer->buffer, rk);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_st(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 const char* str, uint32_t xf_id) {
    size_t str_len = strlen(str);
    size_t utf16_len = cxlsb_utf16le_byte_length(str);
    uint32_t record_size = 4 + 4 + 4 + 1 + 4 + utf16_len;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_CELL_ST,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, col);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_byte(writer->buffer, 0);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_biff12_writer_write_wide_string(writer, str);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_isst(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   uint32_t sst_index, uint32_t xf_id) {
    uint32_t record_size = 4 + 4 + 4 + 4;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_CELL_ISST,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, col);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_int(writer->buffer, sst_index);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_bool(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   bool value, uint32_t xf_id) {
    uint32_t record_size = 4 + 4 + 4 + 1;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_CELL_BOOL,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, col);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_byte(writer->buffer, value ? 1 : 0);
}

cxlsb_error_t cxlsb_biff12_writer_write_cell_blank(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, uint32_t col,
                                                    uint32_t xf_id) {
    uint32_t record_size = 4 + 4 + 4;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_CELL_BLANK,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, col);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_int(writer->buffer, xf_id);
}

cxlsb_error_t cxlsb_biff12_writer_write_row_header(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, 
                                                    uint32_t first_col,
                                                    uint32_t last_col) {
    uint32_t record_size = 4 + 4 + 4 + 1;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_ROW_HDR,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, first_col);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, last_col);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_byte(writer->buffer, 0);
}

cxlsb_error_t cxlsb_biff12_writer_write_ws_dim(cxlsb_biff12_writer_t* writer,
                                                uint32_t first_row,
                                                uint32_t first_col,
                                                uint32_t last_row,
                                                uint32_t last_col) {
    uint32_t record_size = 16;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_WS_DIM,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, first_row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, last_row);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, first_col);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_buffer_writer_write_int(writer->buffer, last_col);
}

uint8_t* cxlsb_biff12_writer_get_buffer(cxlsb_biff12_writer_t* writer) {
    return cxlsb_buffer_writer_get_buffer(writer->buffer);
}

size_t cxlsb_biff12_writer_get_length(cxlsb_biff12_writer_t* writer) {
    return cxlsb_buffer_writer_get_length(writer->buffer);
}

void cxlsb_biff12_writer_reset(cxlsb_biff12_writer_t* writer) {
    cxlsb_buffer_writer_reset(writer->buffer);
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/biff12_writer.c
git commit -m "feat(format): add biff12_writer.c implementation"
```

---

## Task 4: 创建 SST 头文件

**Files:**
- Create: `src/format/sst_table.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_SST_TABLE_H
#define CXLSB_SST_TABLE_H

#include <stdint.h>
#include "biff12_writer.h"

typedef struct cxlsb_sst_entry {
    char* string;
    uint32_t hash;
    uint32_t count;
} cxlsb_sst_entry_t;

typedef struct cxlsb_sst_table {
    cxlsb_sst_entry_t* entries;
    uint32_t* hash_to_index;
    uint32_t capacity;
    uint32_t count;
    uint32_t total_ref_count;
} cxlsb_sst_table_t;

cxlsb_sst_table_t* cxlsb_sst_table_create(uint32_t initial_capacity);

void cxlsb_sst_table_destroy(cxlsb_sst_table_t* table);

uint32_t cxlsb_sst_table_add_string(cxlsb_sst_table_t* table, const char* str);

int cxlsb_sst_table_find_string(cxlsb_sst_table_t* table, const char* str);

const char* cxlsb_sst_table_get_string(cxlsb_sst_table_t* table, uint32_t index);

uint32_t cxlsb_sst_table_get_count(cxlsb_sst_table_t* table);

uint32_t cxlsb_sst_table_get_total_count(cxlsb_sst_table_t* table);

cxlsb_error_t cxlsb_sst_table_serialize(cxlsb_sst_table_t* table,
                                         cxlsb_biff12_writer_t* writer);

#endif /* CXLSB_SST_TABLE_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/sst_table.h
git commit -m "feat(format): add sst_table.h header"
```

---

## Task 5: 实现 SST

**Files:**
- Create: `src/format/sst_table.c`

- [ ] **Step 1: 写入内容**

```c
#include "sst_table.h"
#include "biff12_record_types.h"
#include <stdlib.h>
#include <string.h>

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (uint8_t)(*str++);
    }
    return hash;
}

cxlsb_sst_table_t* cxlsb_sst_table_create(uint32_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 1024;
    }
    
    cxlsb_sst_table_t* table = (cxlsb_sst_table_t*)malloc(sizeof(cxlsb_sst_table_t));
    if (table == NULL) {
        return NULL;
    }
    
    table->entries = (cxlsb_sst_entry_t*)malloc(sizeof(cxlsb_sst_entry_t) * initial_capacity);
    if (table->entries == NULL) {
        free(table);
        return NULL;
    }
    
    table->hash_to_index = (uint32_t*)malloc(sizeof(uint32_t) * initial_capacity * 2);
    if (table->hash_to_index == NULL) {
        free(table->entries);
        free(table);
        return NULL;
    }
    
    memset(table->hash_to_index, 0xFF, sizeof(uint32_t) * initial_capacity * 2);
    
    table->capacity = initial_capacity;
    table->count = 0;
    table->total_ref_count = 0;
    
    return table;
}

void cxlsb_sst_table_destroy(cxlsb_sst_table_t* table) {
    if (table != NULL) {
        for (uint32_t i = 0; i < table->count; i++) {
            if (table->entries[i].string != NULL) {
                free(table->entries[i].string);
            }
        }
        free(table->entries);
        free(table->hash_to_index);
        free(table);
    }
}

static cxlsb_error_t sst_table_expand(cxlsb_sst_table_t* table) {
    uint32_t new_capacity = table->capacity * 2;
    
    cxlsb_sst_entry_t* new_entries = (cxlsb_sst_entry_t*)realloc(
        table->entries, sizeof(cxlsb_sst_entry_t) * new_capacity);
    if (new_entries == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    uint32_t* new_hash = (uint32_t*)realloc(
        table->hash_to_index, sizeof(uint32_t) * new_capacity * 2);
    if (new_hash == NULL) {
        table->entries = new_entries;
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memset(new_hash + table->capacity * 2, 0xFF, 
           sizeof(uint32_t) * (new_capacity - table->capacity) * 2);
    
    table->entries = new_entries;
    table->hash_to_index = new_hash;
    table->capacity = new_capacity;
    
    return CXLSB_OK;
}

uint32_t cxlsb_sst_table_add_string(cxlsb_sst_table_t* table, const char* str) {
    if (str == NULL) {
        return 0;
    }
    
    uint32_t hash = hash_string(str);
    uint32_t hash_slot = hash % (table->capacity * 2);
    
    uint32_t existing_index = table->hash_to_index[hash_slot];
    if (existing_index != 0xFFFFFFFF) {
        if (strcmp(table->entries[existing_index].string, str) == 0) {
            table->entries[existing_index].count++;
            table->total_ref_count++;
            return existing_index;
        }
    }
    
    if (table->count >= table->capacity) {
        if (sst_table_expand(table) != CXLSB_OK) {
            return 0;
        }
        hash_slot = hash % (table->capacity * 2);
    }
    
    uint32_t new_index = table->count;
    
    table->entries[new_index].string = strdup(str);
    if (table->entries[new_index].string == NULL) {
        return 0;
    }
    
    table->entries[new_index].hash = hash;
    table->entries[new_index].count = 1;
    
    table->hash_to_index[hash_slot] = new_index;
    table->count++;
    table->total_ref_count++;
    
    return new_index;
}

int cxlsb_sst_table_find_string(cxlsb_sst_table_t* table, const char* str) {
    if (str == NULL || table->count == 0) {
        return -1;
    }
    
    uint32_t hash = hash_string(str);
    uint32_t hash_slot = hash % (table->capacity * 2);
    
    uint32_t index = table->hash_to_index[hash_slot];
    if (index == 0xFFFFFFFF) {
        return -1;
    }
    
    if (strcmp(table->entries[index].string, str) == 0) {
        return (int)index;
    }
    
    return -1;
}

const char* cxlsb_sst_table_get_string(cxlsb_sst_table_t* table, uint32_t index) {
    if (index >= table->count) {
        return NULL;
    }
    return table->entries[index].string;
}

uint32_t cxlsb_sst_table_get_count(cxlsb_sst_table_t* table) {
    return table->count;
}

uint32_t cxlsb_sst_table_get_total_count(cxlsb_sst_table_t* table) {
    return table->total_ref_count;
}

cxlsb_error_t cxlsb_sst_table_serialize(cxlsb_sst_table_t* table,
                                         cxlsb_biff12_writer_t* writer) {
    uint32_t record_size = 8;
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_record_header(writer,
                                                                 BIFF12_BRT_BEGIN_SST,
                                                                 record_size);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, table->total_ref_count);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_buffer_writer_write_int(writer->buffer, table->count);
    if (err != CXLSB_OK) return err;
    
    for (uint32_t i = 0; i < table->count; i++) {
        const char* str = table->entries[i].string;
        size_t str_len = strlen(str);
        size_t utf16_len = cxlsb_utf16le_byte_length(str);
        
        uint32_t sst_item_size = 1 + 4 + utf16_len;
        
        err = cxlsb_biff12_writer_write_record_header(writer,
                                                       BIFF12_BRT_SST_ITEM,
                                                       sst_item_size);
        if (err != CXLSB_OK) return err;
        
        err = cxlsb_buffer_writer_write_byte(writer->buffer, 0);
        if (err != CXLSB_OK) return err;
        
        err = cxlsb_buffer_writer_write_int(writer->buffer, (uint32_t)str_len);
        if (err != CXLSB_OK) return err;
        
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf == NULL) {
            return CXLSB_ERR_NO_MEMORY;
        }
        
        err = cxlsb_utf8_to_utf16le(str, utf16_buf, &utf16_len);
        if (err != CXLSB_OK) {
            free(utf16_buf);
            return err;
        }
        
        err = cxlsb_buffer_writer_write_bytes(writer->buffer, utf16_buf, utf16_len);
        free(utf16_buf);
        if (err != CXLSB_OK) return err;
    }
    
    return cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_SST);
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/sst_table.c
git commit -m "feat(format): add sst_table.c implementation"
```

---

## Task 6: 创建 Format 层测试

**Files:**
- Create: `test/test_format.c`

- [ ] **Step 1: 写入测试内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "src/format/biff12_writer.h"
#include "src/format/sst_table.h"

static void test_biff12_write_int_cell(void) {
    cxlsb_biff12_writer_t* writer = cxlsb_biff12_writer_create(1024);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_cell_real(writer, 0, 0, 123.45, 0);
    assert(err == CXLSB_OK);
    
    size_t len = cxlsb_biff12_writer_get_length(writer);
    assert(len > 0);
    
    uint8_t* buf = cxlsb_biff12_writer_get_buffer(writer);
    assert(buf != NULL);
    
    cxlsb_biff12_writer_destroy(writer);
    printf("test_biff12_write_int_cell: PASS\n");
}

static void test_biff12_write_text_cell(void) {
    cxlsb_biff12_writer_t* writer = cxlsb_biff12_writer_create(1024);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_cell_st(writer, 0, 0, "Hello", 0);
    assert(err == CXLSB_OK);
    
    size_t len = cxlsb_biff12_writer_get_length(writer);
    assert(len > 8);
    
    cxlsb_biff12_writer_destroy(writer);
    printf("test_biff12_write_text_cell: PASS\n");
}

static void test_biff12_write_row(void) {
    cxlsb_biff12_writer_t* writer = cxlsb_biff12_writer_create(1024);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_biff12_writer_write_row_header(writer, 0, 0, 9);
    assert(err == CXLSB_OK);
    
    cxlsb_biff12_writer_destroy(writer);
    printf("test_biff12_write_row: PASS\n");
}

static void test_sst_add_string(void) {
    cxlsb_sst_table_t* table = cxlsb_sst_table_create(16);
    assert(table != NULL);
    
    uint32_t idx1 = cxlsb_sst_table_add_string(table, "Hello");
    assert(idx1 == 0);
    
    uint32_t idx2 = cxlsb_sst_table_add_string(table, "World");
    assert(idx2 == 1);
    
    uint32_t idx3 = cxlsb_sst_table_add_string(table, "Hello");
    assert(idx3 == 0);
    
    assert(cxlsb_sst_table_get_count(table) == 2);
    assert(cxlsb_sst_table_get_total_count(table) == 3);
    
    const char* str = cxlsb_sst_table_get_string(table, 0);
    assert(strcmp(str, "Hello") == 0);
    
    cxlsb_sst_table_destroy(table);
    printf("test_sst_add_string: PASS\n");
}

static void test_sst_serialize(void) {
    cxlsb_sst_table_t* table = cxlsb_sst_table_create(16);
    cxlsb_sst_table_add_string(table, "Test");
    
    cxlsb_biff12_writer_t* writer = cxlsb_biff12_writer_create(256);
    
    cxlsb_error_t err = cxlsb_sst_table_serialize(table, writer);
    assert(err == CXLSB_OK);
    
    size_t len = cxlsb_biff12_writer_get_length(writer);
    assert(len > 0);
    
    cxlsb_biff12_writer_destroy(writer);
    cxlsb_sst_table_destroy(table);
    printf("test_sst_serialize: PASS\n");
}

int main(void) {
    test_biff12_write_int_cell();
    test_biff12_write_text_cell();
    test_biff12_write_row();
    test_sst_add_string();
    test_sst_serialize();
    
    printf("\nAll format tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add test/test_format.c
git commit -m "test(format): add format tests"
```

---

## Task 7: 构建并运行测试

- [ ] **Step 1: 重新构建**

```bash
cd build && cmake .. && make
```

- [ ] **Step 2: 运行测试**

```bash
./test_format
```
Expected: 输出 "All format tests passed!"

---

## Phase 4 完成检查

- [ ] BIFF12 记录类型常量已定义
- [ ] BIFF12 写入器已实现
- [ ] SST 表已实现（带哈希优化）
- [ ] 测试已通过
- [ ] Git 已提交

**下一步:** 进入 Phase 5 - Container层实现