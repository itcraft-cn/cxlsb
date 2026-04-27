# cxlsb Phase 6: Format层高级（Sheet/Workbook写入器）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现 Sheet 写入器、Workbook 写入器、Styles 注册表

**Architecture:** 组合 BIFF12 写入器 + SST + Styles，生成完整 Sheet/Workbook 数据

**Tech Stack:** C99

---

## Task 1: 创建 workbook_writer 头文件

**Files:**
- Create: `src/format/workbook_writer.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_WORKBOOK_WRITER_H
#define CXLSB_WORKBOOK_WRITER_H

#include <stdint.h>
#include "biff12_writer.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_workbook_writer {
    char** sheet_names;
    uint32_t sheet_count;
    uint32_t capacity;
} cxlsb_workbook_writer_t;

cxlsb_workbook_writer_t* cxlsb_workbook_writer_create(void);

void cxlsb_workbook_writer_destroy(cxlsb_workbook_writer_t* writer);

cxlsb_error_t cxlsb_workbook_writer_add_sheet(cxlsb_workbook_writer_t* writer,
                                               const char* name);

uint32_t cxlsb_workbook_writer_get_count(cxlsb_workbook_writer_t* writer);

cxlsb_error_t cxlsb_workbook_writer_serialize(cxlsb_workbook_writer_t* writer,
                                               cxlsb_biff12_writer_t* biff12);

cxlsb_error_t cxlsb_workbook_writer_to_bytes(cxlsb_workbook_writer_t* writer,
                                              uint8_t** out_data,
                                              size_t* out_length);

#endif /* CXLSB_WORKBOOK_WRITER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/workbook_writer.h
git commit -m "feat(format): add workbook_writer.h header"
```

---

## Task 2: 实现 workbook_writer

**Files:**
- Create: `src/format/workbook_writer.c`

- [ ] **Step 1: 写入内容**

```c
#include "workbook_writer.h"
#include "biff12_record_types.h"
#include "../util/varint.h"
#include <stdlib.h>
#include <string.h>

cxlsb_workbook_writer_t* cxlsb_workbook_writer_create(void) {
    cxlsb_workbook_writer_t* writer = (cxlsb_workbook_writer_t*)malloc(sizeof(cxlsb_workbook_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    writer->capacity = 16;
    writer->sheet_names = (char**)malloc(sizeof(char*) * writer->capacity);
    if (writer->sheet_names == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->sheet_count = 0;
    
    return writer;
}

void cxlsb_workbook_writer_destroy(cxlsb_workbook_writer_t* writer) {
    if (writer != NULL) {
        for (uint32_t i = 0; i < writer->sheet_count; i++) {
            if (writer->sheet_names[i] != NULL) {
                free(writer->sheet_names[i]);
            }
        }
        free(writer->sheet_names);
        free(writer);
    }
}

static cxlsb_error_t workbook_writer_expand(cxlsb_workbook_writer_t* writer) {
    uint32_t new_capacity = writer->capacity * 2;
    char** new_names = (char**)realloc(writer->sheet_names, sizeof(char*) * new_capacity);
    if (new_names == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    writer->sheet_names = new_names;
    writer->capacity = new_capacity;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_workbook_writer_add_sheet(cxlsb_workbook_writer_t* writer,
                                               const char* name) {
    if (writer == NULL || name == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (writer->sheet_count >= writer->capacity) {
        cxlsb_error_t err = workbook_writer_expand(writer);
        if (err != CXLSB_OK) {
            return err;
        }
    }
    
    writer->sheet_names[writer->sheet_count] = strdup(name);
    if (writer->sheet_names[writer->sheet_count] == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    writer->sheet_count++;
    return CXLSB_OK;
}

uint32_t cxlsb_workbook_writer_get_count(cxlsb_workbook_writer_t* writer) {
    return writer->sheet_count;
}

cxlsb_error_t cxlsb_workbook_writer_serialize(cxlsb_workbook_writer_t* writer,
                                               cxlsb_biff12_writer_t* biff12) {
    cxlsb_error_t err;
    
    err = cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BOOK);
    if (err != CXLSB_OK) return err;
    
    uint32_t file_version_size = 16;
    err = cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_FILE_VERSION, file_version_size);
    if (err != CXLSB_OK) return err;
    
    cxlsb_buffer_writer_write_short(biff12->buffer, 0);
    cxlsb_buffer_writer_write_short(biff12->buffer, 0);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0x00020010);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    
    err = cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_WB_PROP);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BOOK_VIEWS);
    if (err != CXLSB_OK) return err;
    
    uint32_t book_view_size = 12;
    err = cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BOOK_VIEW, book_view_size);
    if (err != CXLSB_OK) return err;
    
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    cxlsb_buffer_writer_write_int(biff12->buffer, 0);
    cxlsb_buffer_writer_write_short(biff12->buffer, 0);
    
    err = cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BOOK_VIEWS);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_BEGIN_BUNDLE_SHS);
    if (err != CXLSB_OK) return err;
    
    for (uint32_t i = 0; i < writer->sheet_count; i++) {
        const char* name = writer->sheet_names[i];
        size_t name_len = strlen(name);
        
        uint32_t bundle_sh_size = 8 + 4 + name_len * 2;
        err = cxlsb_biff12_writer_write_record_header(biff12, BIFF12_BRT_BUNDLE_SH, bundle_sh_size);
        if (err != CXLSB_OK) return err;
        
        cxlsb_buffer_writer_write_int(biff12->buffer, i);
        cxlsb_buffer_writer_write_int(biff12->buffer, 0);
        
        err = cxlsb_buffer_writer_write_int(biff12->buffer, name_len);
        if (err != CXLSB_OK) return err;
        
        size_t utf16_len = cxlsb_utf16le_byte_length(name);
        uint8_t* utf16_buf = (uint8_t*)malloc(utf16_len);
        if (utf16_buf == NULL) return CXLSB_ERR_NO_MEMORY;
        
        cxlsb_utf8_to_utf16le(name, utf16_buf, &utf16_len);
        cxlsb_buffer_writer_write_bytes(biff12->buffer, utf16_buf, utf16_len);
        free(utf16_buf);
    }
    
    err = cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BUNDLE_SHS);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_biff12_writer_write_empty_record(biff12, BIFF12_BRT_END_BOOK);
}

cxlsb_error_t cxlsb_workbook_writer_to_bytes(cxlsb_workbook_writer_t* writer,
                                              uint8_t** out_data,
                                              size_t* out_length) {
    cxlsb_biff12_writer_t* biff12 = cxlsb_biff12_writer_create(4096);
    if (biff12 == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    cxlsb_error_t err = cxlsb_workbook_writer_serialize(writer, biff12);
    if (err != CXLSB_OK) {
        cxlsb_biff12_writer_destroy(biff12);
        return err;
    }
    
    size_t len = cxlsb_biff12_writer_get_length(biff12);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) {
        cxlsb_biff12_writer_destroy(biff12);
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(data, cxlsb_biff12_writer_get_buffer(biff12), len);
    
    *out_data = data;
    *out_length = len;
    
    cxlsb_biff12_writer_destroy(biff12);
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/workbook_writer.c
git commit -m "feat(format): add workbook_writer.c implementation"
```

---

## Task 3: 创建 styles_registry 头文件

**Files:**
- Create: `src/format/styles_registry.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_STYLES_REGISTRY_H
#define CXLSB_STYLES_REGISTRY_H

#include <stdint.h>
#include "biff12_writer.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_styles_registry {
    cxlsb_xf_t* xfs;
    uint32_t xf_count;
    uint32_t xf_capacity;
    uint32_t default_xf_id;
} cxlsb_styles_registry_t;

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void);

void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry);

uint32_t cxlsb_styles_registry_add_xf(cxlsb_styles_registry_t* registry,
                                       const cxlsb_xf_t* xf);

cxlsb_error_t cxlsb_styles_registry_serialize(cxlsb_styles_registry_t* registry,
                                               cxlsb_biff12_writer_t* writer);

cxlsb_error_t cxlsb_styles_registry_to_bytes(cxlsb_styles_registry_t* registry,
                                              uint8_t** out_data,
                                              size_t* out_length);

#endif /* CXLSB_STYLES_REGISTRY_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/styles_registry.h
git commit -m "feat(format): add styles_registry.h header"
```

---

## Task 4: 实现 styles_registry

**Files:**
- Create: `src/format/styles_registry.c`

- [ ] **Step 1: 写入内容**

```c
#include "styles_registry.h"
#include "biff12_record_types.h"
#include <stdlib.h>
#include <string.h>

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void) {
    cxlsb_styles_registry_t* registry = (cxlsb_styles_registry_t*)malloc(sizeof(cxlsb_styles_registry_t));
    if (registry == NULL) {
        return NULL;
    }
    
    registry->xf_capacity = 16;
    registry->xfs = (cxlsb_xf_t*)malloc(sizeof(cxlsb_xf_t) * registry->xf_capacity);
    if (registry->xfs == NULL) {
        free(registry);
        return NULL;
    }
    
    registry->xf_count = 0;
    registry->default_xf_id = 0;
    
    cxlsb_xf_t default_xf = {0, 0, 0, 0, 0, 0};
    cxlsb_styles_registry_add_xf(registry, &default_xf);
    
    return registry;
}

void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry) {
    if (registry != NULL) {
        if (registry->xfs != NULL) {
            free(registry->xfs);
        }
        free(registry);
    }
}

uint32_t cxlsb_styles_registry_add_xf(cxlsb_styles_registry_t* registry,
                                       const cxlsb_xf_t* xf) {
    if (registry->xf_count >= registry->xf_capacity) {
        uint32_t new_capacity = registry->xf_capacity * 2;
        cxlsb_xf_t* new_xfs = (cxlsb_xf_t*)realloc(registry->xfs, 
                                                    sizeof(cxlsb_xf_t) * new_capacity);
        if (new_xfs == NULL) {
            return 0;
        }
        registry->xfs = new_xfs;
        registry->xf_capacity = new_capacity;
    }
    
    uint32_t xf_id = registry->xf_count;
    registry->xfs[xf_id] = *xf;
    registry->xf_count++;
    
    return xf_id;
}

cxlsb_error_t cxlsb_styles_registry_serialize(cxlsb_styles_registry_t* registry,
                                               cxlsb_biff12_writer_t* writer) {
    cxlsb_error_t err;
    
    err = cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_STYLE_SHEET);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_FILLS);
    if (err != CXLSB_OK) return err;
    
    uint32_t fill_size = 4;
    err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_FILL, fill_size);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_FILLS);
    
    err = cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_BORDERS);
    if (err != CXLSB_OK) return err;
    
    uint32_t border_size = 8;
    err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_BORDER, border_size);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    cxlsb_buffer_writer_write_int(writer->buffer, 0);
    
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_BORDERS);
    
    err = cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_BEGIN_CELL_XFS);
    if (err != CXLSB_OK) return err;
    
    for (uint32_t i = 0; i < registry->xf_count; i++) {
        cxlsb_xf_t* xf = &registry->xfs[i];
        
        uint32_t xf_size = 12;
        err = cxlsb_biff12_writer_write_record_header(writer, BIFF12_BRT_XF, xf_size);
        if (err != CXLSB_OK) return err;
        
        cxlsb_buffer_writer_write_int(writer->buffer, xf->font_id);
        cxlsb_buffer_writer_write_int(writer->buffer, xf->fill_id);
        cxlsb_buffer_writer_write_int(writer->buffer, xf->border_id);
    }
    
    cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_CELL_XFS);
    
    return cxlsb_biff12_writer_write_empty_record(writer, BIFF12_BRT_END_STYLE_SHEET);
}

cxlsb_error_t cxlsb_styles_registry_to_bytes(cxlsb_styles_registry_t* registry,
                                              uint8_t** out_data,
                                              size_t* out_length) {
    cxlsb_biff12_writer_t* writer = cxlsb_biff12_writer_create(4096);
    if (writer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    cxlsb_error_t err = cxlsb_styles_registry_serialize(registry, writer);
    if (err != CXLSB_OK) {
        cxlsb_biff12_writer_destroy(writer);
        return err;
    }
    
    size_t len = cxlsb_biff12_writer_get_length(writer);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) {
        cxlsb_biff12_writer_destroy(writer);
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(data, cxlsb_biff12_writer_get_buffer(writer), len);
    
    *out_data = data;
    *out_length = len;
    
    cxlsb_biff12_writer_destroy(writer);
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/styles_registry.c
git commit -m "feat(format): add styles_registry.c implementation"
```

---

## Task 5: 创建 sheet_writer 头文件

**Files:**
- Create: `src/format/sheet_writer.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_SHEET_WRITER_H
#define CXLSB_SHEET_WRITER_H

#include "sst_table.h"
#include "styles_registry.h"
#include "biff12_writer.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_sheet_writer {
    cxlsb_biff12_writer_t* biff12;
    cxlsb_sst_table_t* sst;
    cxlsb_styles_registry_t* styles;
    int col_count;
    int max_row;
    int max_col;
    bool streaming_mode;
} cxlsb_sheet_writer_t;

cxlsb_sheet_writer_t* cxlsb_sheet_writer_create(cxlsb_sst_table_t* sst,
                                                 cxlsb_styles_registry_t* styles);

void cxlsb_sheet_writer_destroy(cxlsb_sheet_writer_t* writer);

cxlsb_error_t cxlsb_sheet_writer_write_sheet(cxlsb_sheet_writer_t* writer,
                                              cxlsb_cell_supplier_fn supplier,
                                              void* user_data,
                                              int row_count,
                                              int col_count,
                                              uint8_t** out_data,
                                              size_t* out_length);

cxlsb_error_t cxlsb_sheet_writer_start_streaming(cxlsb_sheet_writer_t* writer,
                                                  int col_count);

cxlsb_error_t cxlsb_sheet_writer_append_rows(cxlsb_sheet_writer_t* writer,
                                              cxlsb_cell_supplier_fn supplier,
                                              void* user_data,
                                              int start_row,
                                              int row_count);

cxlsb_error_t cxlsb_sheet_writer_finalize_streaming(cxlsb_sheet_writer_t* writer,
                                                     int row_count,
                                                     int col_count,
                                                     uint8_t** out_data,
                                                     size_t* out_length);

#endif /* CXLSB_SHEET_WRITER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/format/sheet_writer.h
git commit -m "feat(format): add sheet_writer.h header"
```

---

## Task 6: 实现 sheet_writer

**Files:**
- Create: `src/format/sheet_writer.c`

- [ ] **Step 1: 写入内容**

```c
#include "sheet_writer.h"
#include "biff12_record_types.h"
#include "../util/varint.h"
#include "../util/utf16.h"
#include <stdlib.h>
#include <math.h>

cxlsb_sheet_writer_t* cxlsb_sheet_writer_create(cxlsb_sst_table_t* sst,
                                                 cxlsb_styles_registry_t* styles) {
    cxlsb_sheet_writer_t* writer = (cxlsb_sheet_writer_t*)malloc(sizeof(cxlsb_sheet_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    writer->biff12 = cxlsb_biff12_writer_create(64 * 1024);
    if (writer->biff12 == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->sst = sst;
    writer->styles = styles;
    writer->col_count = 0;
    writer->max_row = 0;
    writer->max_col = 0;
    writer->streaming_mode = false;
    
    return writer;
}

void cxlsb_sheet_writer_destroy(cxlsb_sheet_writer_t* writer) {
    if (writer != NULL) {
        if (writer->biff12 != NULL) {
            cxlsb_biff12_writer_destroy(writer->biff12);
        }
        free(writer);
    }
}

static cxlsb_error_t write_sheet_begin(cxlsb_sheet_writer_t* writer) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_empty_record(writer->biff12, 
                                                                BIFF12_BRT_BEGIN_SHEET);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_WS_PROP);
    if (err != CXLSB_OK) return err;
    
    return CXLSB_OK;
}

static cxlsb_error_t write_sheet_data(cxlsb_sheet_writer_t* writer,
                                       cxlsb_cell_supplier_fn supplier,
                                       void* user_data,
                                       int start_row,
                                       int row_count,
                                       int col_count) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_empty_record(writer->biff12,
                                                                BIFF12_BRT_BEGIN_SHEET_DATA);
    if (err != CXLSB_OK) return err;
    
    for (int row = start_row; row < start_row + row_count; row++) {
        int first_col = -1;
        int last_col = -1;
        
        for (int col = 0; col < col_count; col++) {
            cxlsb_cell_data_t cell = supplier(row, col, user_data);
            
            if (cell.type != CXLSB_CELL_BLANK) {
                if (first_col < 0) first_col = col;
                last_col = col;
            }
        }
        
        if (first_col >= 0) {
            err = cxlsb_biff12_writer_write_row_header(writer->biff12, row, first_col, last_col);
            if (err != CXLSB_OK) return err;
            
            for (int col = first_col; col <= last_col; col++) {
                cxlsb_cell_data_t cell = supplier(row, col, user_data);
                uint32_t xf_id = 0;
                
                switch (cell.type) {
                    case CXLSB_CELL_NUMBER:
                        err = cxlsb_biff12_writer_write_cell_real(writer->biff12,
                                                                  row, col,
                                                                  cell.value.number, xf_id);
                        break;
                        
                    case CXLSB_CELL_TEXT:
                        if (writer->sst != NULL && strlen(cell.value.text) > 3) {
                            uint32_t sst_idx = cxlsb_sst_table_add_string(writer->sst, cell.value.text);
                            err = cxlsb_biff12_writer_write_cell_isst(writer->biff12,
                                                                      row, col, sst_idx, xf_id);
                        } else {
                            err = cxlsb_biff12_writer_write_cell_st(writer->biff12,
                                                                   row, col,
                                                                   cell.value.text, xf_id);
                        }
                        break;
                        
                    case CXLSB_CELL_BOOL:
                        err = cxlsb_biff12_writer_write_cell_bool(writer->biff12,
                                                                  row, col,
                                                                  cell.value.boolean, xf_id);
                        break;
                        
                    case CXLSB_CELL_DATE:
                        err = cxlsb_biff12_writer_write_cell_real(writer->biff12,
                                                                  row, col,
                                                                  cell.value.number, xf_id);
                        break;
                        
                    case CXLSB_CELL_BLANK:
                        break;
                        
                    default:
                        err = cxlsb_biff12_writer_write_cell_blank(writer->biff12,
                                                                   row, col, xf_id);
                        break;
                }
                
                if (err != CXLSB_OK) return err;
            }
        }
        
        if (row > writer->max_row) writer->max_row = row;
        if (col_count > writer->max_col) writer->max_col = col_count - 1;
    }
    
    return cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET_DATA);
}

static cxlsb_error_t write_sheet_end(cxlsb_sheet_writer_t* writer,
                                       int row_count,
                                       int col_count) {
    cxlsb_error_t err = cxlsb_biff12_writer_write_ws_dim(writer->biff12,
                                                          0, 0,
                                                          row_count > 0 ? row_count - 1 : 0,
                                                          col_count > 0 ? col_count - 1 : 0);
    if (err != CXLSB_OK) return err;
    
    return cxlsb_biff12_writer_write_empty_record(writer->biff12, BIFF12_BRT_END_SHEET);
}

cxlsb_error_t cxlsb_sheet_writer_write_sheet(cxlsb_sheet_writer_t* writer,
                                              cxlsb_cell_supplier_fn supplier,
                                              void* user_data,
                                              int row_count,
                                              int col_count,
                                              uint8_t** out_data,
                                              size_t* out_length) {
    cxlsb_biff12_writer_reset(writer->biff12);
    
    cxlsb_error_t err = write_sheet_begin(writer);
    if (err != CXLSB_OK) return err;
    
    err = write_sheet_data(writer, supplier, user_data, 0, row_count, col_count);
    if (err != CXLSB_OK) return err;
    
    err = write_sheet_end(writer, row_count, col_count);
    if (err != CXLSB_OK) return err;
    
    size_t len = cxlsb_biff12_writer_get_length(writer->biff12);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(data, cxlsb_biff12_writer_get_buffer(writer->biff12), len);
    
    *out_data = data;
    *out_length = len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_sheet_writer_start_streaming(cxlsb_sheet_writer_t* writer,
                                                  int col_count) {
    writer->streaming_mode = true;
    writer->col_count = col_count;
    writer->max_row = 0;
    writer->max_col = 0;
    
    cxlsb_biff12_writer_reset(writer->biff12);
    
    return write_sheet_begin(writer);
}

cxlsb_error_t cxlsb_sheet_writer_append_rows(cxlsb_sheet_writer_t* writer,
                                              cxlsb_cell_supplier_fn supplier,
                                              void* user_data,
                                              int start_row,
                                              int row_count) {
    if (!writer->streaming_mode) {
        return CXLSB_ERR_INVALID_ARG;
    }
    
    return write_sheet_data(writer, supplier, user_data, start_row, row_count, writer->col_count);
}

cxlsb_error_t cxlsb_sheet_writer_finalize_streaming(cxlsb_sheet_writer_t* writer,
                                                     int row_count,
                                                     int col_count,
                                                     uint8_t** out_data,
                                                     size_t* out_length) {
    if (!writer->streaming_mode) {
        return CXLSB_ERR_INVALID_ARG;
    }
    
    cxlsb_error_t err = write_sheet_end(writer, row_count, col_count);
    if (err != CXLSB_OK) return err;
    
    writer->streaming_mode = false;
    
    size_t len = cxlsb_biff12_writer_get_length(writer->biff12);
    uint8_t* data = (uint8_t*)malloc(len);
    if (data == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(data, cxlsb_biff12_writer_get_buffer(writer->biff12), len);
    
    *out_data = data;
    *out_length = len;
    
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/format/sheet_writer.c
git commit -m "feat(format): add sheet_writer.c implementation"
```

---

## Phase 6 完成检查

- [ ] workbook_writer 已实现
- [ ] styles_registry 已实现
- [ ] sheet_writer 已实现
- [ ] Git 已提交

**下一步:** 进入 Phase 7 - API层写入器