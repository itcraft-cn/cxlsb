# cxlsb Phase 8: 读取器实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现 BIFF12 读取器、Sheet 读取器、xlsb_reader API

**Architecture:** 解析 BIFF12 记录，提取单元格数据

**Tech Stack:** C99

---

## Task 1: 创建 biff12_reader 头文件

**Files:**
- Create: `src/format/biff12_reader.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_BIFF12_READER_H
#define CXLSB_BIFF12_READER_H

#include "../io/buffer_reader.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_biff12_record {
    uint32_t type;
    uint32_t size;
    size_t data_offset;
} cxlsb_biff12_record_t;

cxlsb_error_t cxlsb_biff12_reader_read_record_header(cxlsb_buffer_reader_t* reader,
                                                      cxlsb_biff12_record_t* record);

cxlsb_error_t cxlsb_biff12_reader_skip_record(cxlsb_buffer_reader_t* reader,
                                               const cxlsb_biff12_record_t* record);

cxlsb_error_t cxlsb_biff12_reader_read_cell_real(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_cell_data_t* cell,
                                                  uint32_t* row,
                                                  uint32_t* col);

cxlsb_error_t cxlsb_biff12_reader_read_cell_isst(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_cell_data_t* cell,
                                                  uint32_t* row,
                                                  uint32_t* col,
                                                  uint32_t* sst_index);

cxlsb_error_t cxlsb_biff12_reader_read_cell_st(cxlsb_buffer_reader_t* reader,
                                                cxlsb_cell_data_t* cell,
                                                uint32_t* row,
                                                uint32_t* col);

cxlsb_error_t cxlsb_biff12_reader_read_cell_bool(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_cell_data_t* cell,
                                                  uint32_t* row,
                                                  uint32_t* col);

cxlsb_error_t cxlsb_biff12_reader_read_row_header(cxlsb_buffer_reader_t* reader,
                                                   uint32_t* row,
                                                   uint32_t* first_col,
                                                   uint32_t* last_col);

cxlsb_error_t cxlsb_biff12_reader_read_wide_string(cxlsb_buffer_reader_t* reader,
                                                    size_t max_length,
                                                    char** out_str);

#endif /* CXLSB_BIFF12_READER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/biff12_reader.h
git commit -m "feat(format): add biff12_reader.h header"
```

---

## Task 2: 实现 biff12_reader

**Files:**
- Create: `src/format/biff12_reader.c`

- [ ] **Step 1: 写入内容**

```c
#include "biff12_reader.h"
#include "biff12_record_types.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include <stdlib.h>
#include <string.h>

cxlsb_error_t cxlsb_biff12_reader_read_record_header(cxlsb_buffer_reader_t* reader,
                                                      cxlsb_biff12_record_t* record) {
    if (!cxlsb_buffer_reader_has_remaining(reader, 2)) {
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    size_t bytes_read;
    uint32_t type_val;
    if (cxlsb_varint_decode_at(cxlsb_buffer_reader_get_buffer(reader) + 
                                cxlsb_buffer_reader_get_position(reader),
                                &type_val, &bytes_read) != 0) {
        return CXLSB_ERR_PARSE_FAILED;
    }
    cxlsb_buffer_reader_set_position(reader, 
                                      cxlsb_buffer_reader_get_position(reader) + bytes_read);
    
    uint32_t size_val;
    if (cxlsb_varint_decode_at(cxlsb_buffer_reader_get_buffer(reader) + 
                                cxlsb_buffer_reader_get_position(reader),
                                &size_val, &bytes_read) != 0) {
        return CXLSB_ERR_PARSE_FAILED;
    }
    cxlsb_buffer_reader_set_position(reader,
                                      cxlsb_buffer_reader_get_position(reader) + bytes_read);
    
    record->type = type_val;
    record->size = size_val;
    record->data_offset = cxlsb_buffer_reader_get_position(reader);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_skip_record(cxlsb_buffer_reader_t* reader,
                                               const cxlsb_biff12_record_t* record) {
    cxlsb_buffer_reader_set_position(reader, record->data_offset + record->size);
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_read_cell_real(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_cell_data_t* cell,
                                                  uint32_t* row,
                                                  uint32_t* col) {
    *row = cxlsb_buffer_reader_read_int(reader);
    *col = cxlsb_buffer_reader_read_int(reader);
    cxlsb_buffer_reader_read_int(reader);
    
    double value = cxlsb_buffer_reader_read_double(reader);
    
    cell->type = CXLSB_CELL_NUMBER;
    cell->value.number = value;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_read_cell_isst(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_cell_data_t* cell,
                                                  uint32_t* row,
                                                  uint32_t* col,
                                                  uint32_t* sst_index) {
    *row = cxlsb_buffer_reader_read_int(reader);
    *col = cxlsb_buffer_reader_read_int(reader);
    cxlsb_buffer_reader_read_int(reader);
    
    *sst_index = cxlsb_buffer_reader_read_int(reader);
    
    cell->type = CXLSB_CELL_TEXT;
    cell->value.text = NULL;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_read_cell_st(cxlsb_buffer_reader_t* reader,
                                                cxlsb_cell_data_t* cell,
                                                uint32_t* row,
                                                uint32_t* col) {
    *row = cxlsb_buffer_reader_read_int(reader);
    *col = cxlsb_buffer_reader_read_int(reader);
    cxlsb_buffer_reader_read_int(reader);
    
    cxlsb_buffer_reader_read_byte(reader);
    
    uint32_t str_len = cxlsb_buffer_reader_read_int(reader);
    
    if (str_len > 0) {
        size_t utf16_len = str_len * 2;
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf == NULL) {
            return CXLSB_ERR_NO_MEMORY;
        }
        
        cxlsb_buffer_reader_read_bytes(reader, utf16_buf, utf16_len);
        
        char* str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
        free(utf16_buf);
        
        cell->type = CXLSB_CELL_TEXT;
        cell->value.text = str;
    } else {
        cell->type = CXLSB_CELL_TEXT;
        cell->value.text = strdup("");
    }
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_read_cell_bool(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_cell_data_t* cell,
                                                  uint32_t* row,
                                                  uint32_t* col) {
    *row = cxlsb_buffer_reader_read_int(reader);
    *col = cxlsb_buffer_reader_read_int(reader);
    cxlsb_buffer_reader_read_int(reader);
    
    uint8_t value = cxlsb_buffer_reader_read_byte(reader);
    
    cell->type = CXLSB_CELL_BOOL;
    cell->value.boolean = (value != 0);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_read_row_header(cxlsb_buffer_reader_t* reader,
                                                   uint32_t* row,
                                                   uint32_t* first_col,
                                                   uint32_t* last_col) {
    *row = cxlsb_buffer_reader_read_int(reader);
    *first_col = cxlsb_buffer_reader_read_int(reader);
    *last_col = cxlsb_buffer_reader_read_int(reader);
    
    cxlsb_buffer_reader_read_byte(reader);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_biff12_reader_read_wide_string(cxlsb_buffer_reader_t* reader,
                                                    size_t max_length,
                                                    char** out_str) {
    uint32_t str_len = cxlsb_buffer_reader_read_int(reader);
    
    if (str_len > max_length / 2) {
        str_len = max_length / 2;
    }
    
    if (str_len > 0) {
        size_t utf16_len = str_len * 2;
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf == NULL) {
            return CXLSB_ERR_NO_MEMORY;
        }
        
        cxlsb_buffer_reader_read_bytes(reader, utf16_buf, utf16_len);
        
        *out_str = cxlsb_utf16le_to_utf8(utf16_buf, utf16_len);
        free(utf16_buf);
        
        if (*out_str == NULL) {
            return CXLSB_ERR_NO_MEMORY;
        }
    } else {
        *out_str = strdup("");
    }
    
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/biff12_reader.c
git commit -m "feat(format): add biff12_reader.c implementation"
```

---

## Task 3: 创建 sheet_reader 头文件

**Files:**
- Create: `src/format/sheet_reader.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_SHEET_READER_H
#define CXLSB_SHEET_READER_H

#include "../io/buffer_reader.h"
#include "sst_table.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_sheet_reader {
    cxlsb_buffer_reader_t* reader;
    cxlsb_sst_table_t* sst;
    uint32_t current_row;
    uint32_t first_col;
    uint32_t last_col;
} cxlsb_sheet_reader_t;

cxlsb_sheet_reader_t* cxlsb_sheet_reader_create(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_sst_table_t* sst);

void cxlsb_sheet_reader_destroy(cxlsb_sheet_reader_t* reader);

cxlsb_error_t cxlsb_sheet_reader_next_row(cxlsb_sheet_reader_t* reader);

cxlsb_error_t cxlsb_sheet_reader_read_cell(cxlsb_sheet_reader_t* reader,
                                            int col_index,
                                            cxlsb_cell_data_t* cell);

cxlsb_error_t cxlsb_sheet_reader_for_each_row(cxlsb_sheet_reader_t* reader,
                                               cxlsb_row_handler_fn handler,
                                               void* user_data);

bool cxlsb_sheet_reader_has_more(cxlsb_sheet_reader_t* reader);

#endif /* CXLSB_SHEET_READER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/sheet_reader.h
git commit -m "feat(format): add sheet_reader.h header"
```

---

## Task 4: 实现 sheet_reader

**Files:**
- Create: `src/format/sheet_reader.c`

- [ ] **Step 1: 写入内容**

```c
#include "sheet_reader.h"
#include "biff12_reader.h"
#include "biff12_record_types.h"
#include <stdlib.h>

cxlsb_sheet_reader_t* cxlsb_sheet_reader_create(cxlsb_buffer_reader_t* reader,
                                                  cxlsb_sst_table_t* sst) {
    cxlsb_sheet_reader_t* sheet_reader = (cxlsb_sheet_reader_t*)malloc(sizeof(cxlsb_sheet_reader_t));
    if (sheet_reader == NULL) {
        return NULL;
    }
    
    sheet_reader->reader = reader;
    sheet_reader->sst = sst;
    sheet_reader->current_row = 0;
    sheet_reader->first_col = 0;
    sheet_reader->last_col = 0;
    
    return sheet_reader;
}

void cxlsb_sheet_reader_destroy(cxlsb_sheet_reader_t* reader) {
    if (reader != NULL) {
        free(reader);
    }
}

cxlsb_error_t cxlsb_sheet_reader_next_row(cxlsb_sheet_reader_t* reader) {
    cxlsb_biff12_record_t record;
    
    while (cxlsb_buffer_reader_has_remaining(reader->reader, 2)) {
        cxlsb_error_t err = cxlsb_biff12_reader_read_record_header(reader->reader, &record);
        if (err != CXLSB_OK) return err;
        
        if (record.type == BIFF12_BRT_END_SHEET_DATA) {
            return CXLSB_OK;
        }
        
        if (record.type == BIFF12_BRT_ROW_HDR) {
            uint32_t row, first_col, last_col;
            err = cxlsb_biff12_reader_read_row_header(reader->reader, &row, &first_col, &last_col);
            if (err != CXLSB_OK) return err;
            
            reader->current_row = row;
            reader->first_col = first_col;
            reader->last_col = last_col;
            
            return CXLSB_OK;
        }
        
        cxlsb_biff12_reader_skip_record(reader->reader, &record);
    }
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_reader_read_cell(cxlsb_sheet_reader_t* reader,
                                            int col_index,
                                            cxlsb_cell_data_t* cell) {
    cxlsb_biff12_record_t record;
    
    while (cxlsb_buffer_reader_has_remaining(reader->reader, 2)) {
        cxlsb_error_t err = cxlsb_biff12_reader_read_record_header(reader->reader, &record);
        if (err != CXLSB_OK) return err;
        
        if (record.type == BIFF12_BRT_ROW_HDR || 
            record.type == BIFF12_BRT_END_SHEET_DATA) {
            size_t header_len = cxlsb_buffer_reader_get_position(reader->reader) - record.data_offset;
            cxlsb_buffer_reader_set_position(reader->reader,
                                              cxlsb_buffer_reader_get_position(reader->reader) - 
                                              (record.size + header_len));
            *cell = cxlsb_cell_blank();
            return CXLSB_OK;
        }
        
        uint32_t row, col;
        
        if (record.type == BIFF12_BRT_CELL_REAL) {
            err = cxlsb_biff12_reader_read_cell_real(reader->reader, cell, &row, &col);
            if (err != CXLSB_OK) return err;
            
            if ((int)col == col_index) {
                return CXLSB_OK;
            }
            continue;
        }
        
        if (record.type == BIFF12_BRT_CELL_ISST) {
            uint32_t sst_index;
            err = cxlsb_biff12_reader_read_cell_isst(reader->reader, cell, &row, &col, &sst_index);
            if (err != CXLSB_OK) return err;
            
            if ((int)col == col_index) {
                if (reader->sst != NULL) {
                    const char* str = cxlsb_sst_table_get_string(reader->sst, sst_index);
                    if (str != NULL) {
                        cell->value.text = strdup(str);
                    }
                }
                return CXLSB_OK;
            }
            continue;
        }
        
        if (record.type == BIFF12_BRT_CELL_ST) {
            err = cxlsb_biff12_reader_read_cell_st(reader->reader, cell, &row, &col);
            if (err != CXLSB_OK) return err;
            
            if ((int)col == col_index) {
                return CXLSB_OK;
            }
            continue;
        }
        
        if (record.type == BIFF12_BRT_CELL_BOOL) {
            err = cxlsb_biff12_reader_read_cell_bool(reader->reader, cell, &row, &col);
            if (err != CXLSB_OK) return err;
            
            if ((int)col == col_index) {
                return CXLSB_OK;
            }
            continue;
        }
        
        if (record.type == BIFF12_BRT_CELL_BLANK) {
            cxlsb_buffer_reader_read_int(reader->reader);
            col = cxlsb_buffer_reader_read_int(reader->reader);
            cxlsb_buffer_reader_read_int(reader->reader);
            
            if ((int)col == col_index) {
                *cell = cxlsb_cell_blank();
                return CXLSB_OK;
            }
            continue;
        }
        
        cxlsb_biff12_reader_skip_record(reader->reader, &record);
    }
    
    *cell = cxlsb_cell_blank();
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_reader_for_each_row(cxlsb_sheet_reader_t* reader,
                                               cxlsb_row_handler_fn handler,
                                               void* user_data) {
    while (cxlsb_sheet_reader_has_more(reader)) {
        cxlsb_error_t err = cxlsb_sheet_reader_next_row(reader);
        if (err != CXLSB_OK) return err;
        
        if (reader->last_col >= reader->first_col) {
            int col_count = reader->last_col - reader->first_col + 1;
            cxlsb_cell_data_t* cells = (cxlsb_cell_data_t*)malloc(sizeof(cxlsb_cell_data_t) * col_count);
            if (cells == NULL) {
                return CXLSB_ERR_NO_MEMORY;
            }
            
            for (int i = reader->first_col; i <= reader->last_col; i++) {
                err = cxlsb_sheet_reader_read_cell(reader, i, &cells[i - reader->first_col]);
                if (err != CXLSB_OK) {
                    free(cells);
                    return err;
                }
            }
            
            handler(reader->current_row, col_count, cells, user_data);
            
            for (int i = 0; i < col_count; i++) {
                if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
                    free(cells[i].value.text);
                }
            }
            free(cells);
        }
    }
    
    return CXLSB_OK;
}

bool cxlsb_sheet_reader_has_more(cxlsb_sheet_reader_t* reader) {
    size_t pos = cxlsb_buffer_reader_get_position(reader->reader);
    
    if (!cxlsb_buffer_reader_has_remaining(reader->reader, 2)) {
        return false;
    }
    
    cxlsb_biff12_record_t record;
    cxlsb_error_t err = cxlsb_biff12_reader_read_record_header(reader->reader, &record);
    
    cxlsb_buffer_reader_set_position(reader->reader, pos);
    
    if (err != CXLSB_OK) {
        return false;
    }
    
    return record.type != BIFF12_BRT_END_SHEET_DATA;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/sheet_reader.c
git commit -m "feat(format): add sheet_reader.c implementation"
```

---

## Task 5: 创建 xlsb_reader 头文件

**Files:**
- Create: `include/cxlsb/xlsb_reader.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_XLSB_READER_H
#define CXLSB_XLSB_READER_H

#include "types.h"
#include "error_codes.h"

typedef struct cxlsb_reader cxlsb_reader_t;

cxlsb_reader_t* cxlsb_reader_create(const char* path);

void cxlsb_reader_destroy(cxlsb_reader_t* reader);

int cxlsb_reader_get_sheet_count(cxlsb_reader_t* reader);

cxlsb_error_t cxlsb_reader_for_each_row(cxlsb_reader_t* reader,
                                         int sheet_index,
                                         cxlsb_row_handler_fn handler,
                                         void* user_data);

cxlsb_error_t cxlsb_reader_for_each_cell(cxlsb_reader_t* reader,
                                          int sheet_index,
                                          cxlsb_cell_handler_fn handler,
                                          void* user_data);

#endif /* CXLSB_XLSB_READER_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/cxlsb/xlsb_reader.h
git commit -m "feat(api): add xlsb_reader.h header"
```

---

## Task 6: 实现 xlsb_reader

**Files:**
- Create: `src/api/xlsb_reader.c`

- [ ] **Step 1: 写入内容**

```c
#include "cxlsb/xlsb_reader.h"
#include "../container/xlsb_container_reader.h"
#include "../format/sst_table.h"
#include "../format/sheet_reader.h"
#include "../format/biff12_reader.h"
#include "../format/biff12_record_types.h"
#include <stdlib.h>
#include <string.h>

struct cxlsb_reader {
    cxlsb_container_reader_t* container;
    cxlsb_sst_table_t* sst;
    uint32_t sheet_count;
};

cxlsb_reader_t* cxlsb_reader_create(const char* path) {
    if (path == NULL) {
        return NULL;
    }
    
    cxlsb_reader_t* reader = (cxlsb_reader_t*)malloc(sizeof(cxlsb_reader_t));
    if (reader == NULL) {
        return NULL;
    }
    
    reader->container = cxlsb_container_reader_create(path);
    if (reader->container == NULL) {
        free(reader);
        return NULL;
    }
    
    reader->sst = NULL;
    reader->sheet_count = cxlsb_container_reader_get_sheet_count(reader->container);
    
    if (cxlsb_container_reader_has_sst(reader->container)) {
        cxlsb_buffer_reader_t* sst_stream = cxlsb_container_reader_get_sst_stream(reader->container);
        if (sst_stream != NULL) {
            reader->sst = cxlsb_sst_table_create(1024);
            if (reader->sst != NULL) {
                cxlsb_biff12_record_t record;
                while (cxlsb_buffer_reader_has_remaining(sst_stream, 2)) {
                    cxlsb_error_t err = cxlsb_biff12_reader_read_record_header(sst_stream, &record);
                    if (err != CXLSB_OK) break;
                    
                    if (record.type == BIFF12_BRT_SST_ITEM) {
                        cxlsb_buffer_reader_read_byte(sst_stream);
                        
                        char* str = NULL;
                        err = cxlsb_biff12_reader_read_wide_string(sst_stream, record.size, &str);
                        if (err == CXLSB_OK && str != NULL) {
                            cxlsb_sst_table_add_string(reader->sst, str);
                            free(str);
                        }
                    } else {
                        cxlsb_biff12_reader_skip_record(sst_stream, &record);
                    }
                }
            }
            
            uint8_t* buf = cxlsb_buffer_reader_get_buffer(sst_stream);
            cxlsb_buffer_reader_destroy(sst_stream);
            free(buf);
        }
    }
    
    return reader;
}

void cxlsb_reader_destroy(cxlsb_reader_t* reader) {
    if (reader != NULL) {
        if (reader->sst != NULL) {
            cxlsb_sst_table_destroy(reader->sst);
        }
        if (reader->container != NULL) {
            cxlsb_container_reader_destroy(reader->container);
        }
        free(reader);
    }
}

int cxlsb_reader_get_sheet_count(cxlsb_reader_t* reader) {
    if (reader == NULL) {
        return -1;
    }
    return (int)reader->sheet_count;
}

cxlsb_error_t cxlsb_reader_for_each_row(cxlsb_reader_t* reader,
                                         int sheet_index,
                                         cxlsb_row_handler_fn handler,
                                         void* user_data) {
    if (reader == NULL || handler == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (sheet_index < 0 || (uint32_t)sheet_index >= reader->sheet_count) {
        return CXLSB_ERR_INVALID_SHEET_INDEX;
    }
    
    cxlsb_buffer_reader_t* sheet_stream = cxlsb_container_reader_get_sheet_stream(reader->container,
                                                                                    sheet_index);
    if (sheet_stream == NULL) {
        return CXLSB_ERR_ZIP_ENTRY_NOT_FOUND;
    }
    
    cxlsb_sheet_reader_t* sheet_reader = cxlsb_sheet_reader_create(sheet_stream, reader->sst);
    if (sheet_reader == NULL) {
        uint8_t* buf = cxlsb_buffer_reader_get_buffer(sheet_stream);
        cxlsb_buffer_reader_destroy(sheet_stream);
        free(buf);
        return CXLSB_ERR_NO_MEMORY;
    }
    
    cxlsb_biff12_record_t record;
    while (cxlsb_buffer_reader_has_remaining(sheet_stream, 2)) {
        cxlsb_error_t err = cxlsb_biff12_reader_read_record_header(sheet_stream, &record);
        if (err != CXLSB_OK) break;
        
        if (record.type == BIFF12_BRT_BEGIN_SHEET_DATA) {
            cxlsb_sheet_reader_for_each_row(sheet_reader, handler, user_data);
            break;
        }
        
        cxlsb_biff12_reader_skip_record(sheet_stream, &record);
    }
    
    cxlsb_sheet_reader_destroy(sheet_reader);
    
    uint8_t* buf = cxlsb_buffer_reader_get_buffer(sheet_stream);
    cxlsb_buffer_reader_destroy(sheet_stream);
    free(buf);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_reader_for_each_cell(cxlsb_reader_t* reader,
                                          int sheet_index,
                                          cxlsb_cell_handler_fn handler,
                                          void* user_data) {
    cxlsb_row_handler_fn row_handler = (cxlsb_row_handler_fn)handler;
    return cxlsb_reader_for_each_row(reader, sheet_index, row_handler, user_data);
}
```

- [ ] **Step 2: Commit**

```bash
git add src/api/xlsb_reader.c
git commit -m "feat(api): add xlsb_reader.c implementation"
```

---

## Phase 8 完成检查

- [ ] biff12_reader 已实现
- [ ] sheet_reader 已实现
- [ ] xlsb_reader API 已实现
- [ ] Git 已提交

**下一步:** 进入 Phase 9 - 示例和集成测试