# cxlsb Phase 7: API层写入器实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现 xlsb_writer API，整合所有组件

**Architecture:** 组合 Container + Workbook + Sheet + SST + Styles

**Tech Stack:** C99

---

## Task 1: 创建 xlsb_writer 头文件

**Files:**
- Create: `include/cxlsb/xlsb_writer.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_XLSB_WRITER_H
#define CXLSB_XLSB_WRITER_H

#include "types.h"
#include "error_codes.h"

typedef struct cxlsb_writer cxlsb_writer_t;

cxlsb_writer_t* cxlsb_writer_create(const char* path);

cxlsb_writer_t* cxlsb_writer_create_with_template(const char* template_path,
                                                   const char* output_path);

void cxlsb_writer_destroy(cxlsb_writer_t* writer);

cxlsb_error_t cxlsb_writer_write_batch(cxlsb_writer_t* writer,
                                        const char* sheet_name,
                                        cxlsb_cell_supplier_fn supplier,
                                        void* user_data,
                                        int row_count,
                                        int col_count);

cxlsb_error_t cxlsb_writer_start_sheet(cxlsb_writer_t* writer,
                                        const char* sheet_name,
                                        int col_count);

cxlsb_error_t cxlsb_writer_write_rows(cxlsb_writer_t* writer,
                                       int start_row,
                                       cxlsb_cell_supplier_fn supplier,
                                       void* user_data,
                                       int row_count);

cxlsb_error_t cxlsb_writer_end_sheet(cxlsb_writer_t* writer);

int cxlsb_writer_get_sheet_count(cxlsb_writer_t* writer);

cxlsb_error_t cxlsb_writer_close(cxlsb_writer_t* writer);

#endif /* CXLSB_XLSB_WRITER_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/cxlsb/xlsb_writer.h
git commit -m "feat(api): add xlsb_writer.h header"
```

---

## Task 2: 实现 xlsb_writer

**Files:**
- Create: `src/api/xlsb_writer.c`

- [ ] **Step 1: 写入内容**

```c
#include "cxlsb/xlsb_writer.h"
#include "../container/xlsb_container.h"
#include "../format/sst_table.h"
#include "../format/styles_registry.h"
#include "../format/sheet_writer.h"
#include "../format/workbook_writer.h"
#include "../util/xml_gen.h"
#include "../util/rels_gen.h"
#include <stdlib.h>
#include <string.h>

struct cxlsb_writer {
    cxlsb_container_t* container;
    cxlsb_sst_table_t* sst;
    cxlsb_styles_registry_t* styles;
    cxlsb_workbook_writer_t* workbook;
    cxlsb_sheet_writer_t* sheet_writer;
    int sheet_count;
    bool is_template_mode;
    bool is_closed;
    
    char* current_sheet_name;
    int current_col_count;
    int current_row_count;
    bool streaming_active;
};

cxlsb_writer_t* cxlsb_writer_create(const char* path) {
    if (path == NULL) {
        return NULL;
    }
    
    cxlsb_writer_t* writer = (cxlsb_writer_t*)malloc(sizeof(cxlsb_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    writer->container = cxlsb_container_create(path);
    if (writer->container == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->sst = cxlsb_sst_table_create(1024);
    if (writer->sst == NULL) {
        cxlsb_container_destroy(writer->container);
        free(writer);
        return NULL;
    }
    
    writer->styles = cxlsb_styles_registry_create();
    if (writer->styles == NULL) {
        cxlsb_sst_table_destroy(writer->sst);
        cxlsb_container_destroy(writer->container);
        free(writer);
        return NULL;
    }
    
    writer->workbook = cxlsb_workbook_writer_create();
    if (writer->workbook == NULL) {
        cxlsb_styles_registry_destroy(writer->styles);
        cxlsb_sst_table_destroy(writer->sst);
        cxlsb_container_destroy(writer->container);
        free(writer);
        return NULL;
    }
    
    writer->sheet_writer = cxlsb_sheet_writer_create(writer->sst, writer->styles);
    if (writer->sheet_writer == NULL) {
        cxlsb_workbook_writer_destroy(writer->workbook);
        cxlsb_styles_registry_destroy(writer->styles);
        cxlsb_sst_table_destroy(writer->sst);
        cxlsb_container_destroy(writer->container);
        free(writer);
        return NULL;
    }
    
    writer->sheet_count = 0;
    writer->is_template_mode = false;
    writer->is_closed = false;
    writer->current_sheet_name = NULL;
    writer->current_col_count = 0;
    writer->current_row_count = 0;
    writer->streaming_active = false;
    
    return writer;
}

cxlsb_writer_t* cxlsb_writer_create_with_template(const char* template_path,
                                                   const char* output_path) {
    return NULL;
}

void cxlsb_writer_destroy(cxlsb_writer_t* writer) {
    if (writer != NULL) {
        if (!writer->is_closed) {
            cxlsb_writer_close(writer);
        }
        
        if (writer->sheet_writer != NULL) {
            cxlsb_sheet_writer_destroy(writer->sheet_writer);
        }
        if (writer->workbook != NULL) {
            cxlsb_workbook_writer_destroy(writer->workbook);
        }
        if (writer->styles != NULL) {
            cxlsb_styles_registry_destroy(writer->styles);
        }
        if (writer->sst != NULL) {
            cxlsb_sst_table_destroy(writer->sst);
        }
        if (writer->container != NULL) {
            cxlsb_container_destroy(writer->container);
        }
        if (writer->current_sheet_name != NULL) {
            free(writer->current_sheet_name);
        }
        free(writer);
    }
}

cxlsb_error_t cxlsb_writer_write_batch(cxlsb_writer_t* writer,
                                        const char* sheet_name,
                                        cxlsb_cell_supplier_fn supplier,
                                        void* user_data,
                                        int row_count,
                                        int col_count) {
    if (writer == NULL || sheet_name == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (writer->is_template_mode) {
        return CXLSB_ERR_IN_TEMPLATE_MODE;
    }
    
    cxlsb_error_t err = cxlsb_workbook_writer_add_sheet(writer->workbook, sheet_name);
    if (err != CXLSB_OK) return err;
    
    uint8_t* sheet_data;
    size_t sheet_length;
    
    err = cxlsb_sheet_writer_write_sheet(writer->sheet_writer,
                                          supplier, user_data,
                                          row_count, col_count,
                                          &sheet_data, &sheet_length);
    if (err != CXLSB_OK) return err;
    
    char entry_name[64];
    snprintf(entry_name, sizeof(entry_name), "xl/worksheets/sheet%d.bin", 
             writer->sheet_count + 1);
    
    err = cxlsb_container_add_entry(writer->container, entry_name,
                                     sheet_data, sheet_length);
    free(sheet_data);
    
    if (err != CXLSB_OK) return err;
    
    writer->sheet_count++;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_start_sheet(cxlsb_writer_t* writer,
                                        const char* sheet_name,
                                        int col_count) {
    if (writer == NULL || sheet_name == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (writer->streaming_active) {
        return CXLSB_ERR_SHEET_NOT_ENDED;
    }
    
    writer->current_sheet_name = strdup(sheet_name);
    if (writer->current_sheet_name == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    writer->current_col_count = col_count;
    writer->current_row_count = 0;
    writer->streaming_active = true;
    
    cxlsb_error_t err = cxlsb_sheet_writer_start_streaming(writer->sheet_writer, col_count);
    if (err != CXLSB_OK) {
        free(writer->current_sheet_name);
        writer->current_sheet_name = NULL;
        writer->streaming_active = false;
        return err;
    }
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_write_rows(cxlsb_writer_t* writer,
                                       int start_row,
                                       cxlsb_cell_supplier_fn supplier,
                                       void* user_data,
                                       int row_count) {
    if (writer == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (!writer->streaming_active) {
        return CXLSB_ERR_SHEET_NOT_STARTED;
    }
    
    cxlsb_error_t err = cxlsb_sheet_writer_append_rows(writer->sheet_writer,
                                                        supplier, user_data,
                                                        start_row, row_count);
    if (err != CXLSB_OK) return err;
    
    writer->current_row_count = start_row + row_count;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_end_sheet(cxlsb_writer_t* writer) {
    if (writer == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (!writer->streaming_active) {
        return CXLSB_ERR_SHEET_NOT_STARTED;
    }
    
    uint8_t* sheet_data;
    size_t sheet_length;
    
    cxlsb_error_t err = cxlsb_sheet_writer_finalize_streaming(writer->sheet_writer,
                                                               writer->current_row_count,
                                                               writer->current_col_count,
                                                               &sheet_data, &sheet_length);
    if (err != CXLSB_OK) return err;
    
    cxlsb_workbook_writer_add_sheet(writer->workbook, writer->current_sheet_name);
    
    char entry_name[64];
    snprintf(entry_name, sizeof(entry_name), "xl/worksheets/sheet%d.bin",
             writer->sheet_count + 1);
    
    err = cxlsb_container_add_entry(writer->container, entry_name,
                                     sheet_data, sheet_length);
    free(sheet_data);
    
    if (err != CXLSB_OK) return err;
    
    free(writer->current_sheet_name);
    writer->current_sheet_name = NULL;
    writer->current_col_count = 0;
    writer->current_row_count = 0;
    writer->streaming_active = false;
    writer->sheet_count++;
    
    return CXLSB_OK;
}

int cxlsb_writer_get_sheet_count(cxlsb_writer_t* writer) {
    if (writer == NULL) {
        return -1;
    }
    return writer->sheet_count;
}

static cxlsb_error_t write_container_structure(cxlsb_writer_t* writer) {
    cxlsb_error_t err;
    uint8_t* data;
    size_t length;
    
    err = cxlsb_xml_gen_content_types(writer->sheet_count,
                                        cxlsb_sst_table_get_count(writer->sst) > 0,
                                        &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "[Content_Types].xml", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_rels_gen_root(&data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "_rels/.rels", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_xml_gen_app(writer->sheet_count, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "docProps/app.xml", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_xml_gen_core(&data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "docProps/core.xml", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_workbook_writer_to_bytes(writer->workbook, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/workbook.bin", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_styles_registry_to_bytes(writer->styles, &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/styles.bin", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_xml_gen_theme(&data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/theme/theme1.xml", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    err = cxlsb_rels_gen_workbook(writer->sheet_count,
                                   cxlsb_sst_table_get_count(writer->sst) > 0,
                                   &data, &length);
    if (err != CXLSB_OK) return err;
    err = cxlsb_container_add_entry(writer->container, "xl/_rels/workbook.bin.rels", data, length);
    free(data);
    if (err != CXLSB_OK) return err;
    
    if (cxlsb_sst_table_get_count(writer->sst) > 0) {
        cxlsb_biff12_writer_t* biff12 = cxlsb_biff12_writer_create(4096);
        if (biff12 == NULL) return CXLSB_ERR_NO_MEMORY;
        
        err = cxlsb_sst_table_serialize(writer->sst, biff12);
        if (err != CXLSB_OK) {
            cxlsb_biff12_writer_destroy(biff12);
            return err;
        }
        
        length = cxlsb_biff12_writer_get_length(biff12);
        data = (uint8_t*)malloc(length);
        if (data == NULL) {
            cxlsb_biff12_writer_destroy(biff12);
            return CXLSB_ERR_NO_MEMORY;
        }
        memcpy(data, cxlsb_biff12_writer_get_buffer(biff12), length);
        
        err = cxlsb_container_add_entry(writer->container, "xl/sharedStrings.bin", data, length);
        free(data);
        cxlsb_biff12_writer_destroy(biff12);
        
        if (err != CXLSB_OK) return err;
    }
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_writer_close(cxlsb_writer_t* writer) {
    if (writer == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (writer->is_closed) {
        return CXLSB_OK;
    }
    
    if (writer->streaming_active) {
        cxlsb_writer_end_sheet(writer);
    }
    
    if (writer->sheet_count > 0) {
        cxlsb_error_t err = write_container_structure(writer);
        if (err != CXLSB_OK) return err;
    }
    
    cxlsb_error_t err = cxlsb_container_close(writer->container);
    if (err != CXLSB_OK) return err;
    
    writer->is_closed = true;
    
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/api/xlsb_writer.c
git commit -m "feat(api): add xlsb_writer.c implementation"
```

---

## Task 3: 创建写入器测试

**Files:**
- Create: `test/test_writer.c`

- [ ] **Step 1: 写入内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "cxlsb/xlsb.h"
#include "cxlsb/xlsb_writer.h"

cxlsb_cell_data_t simple_supplier(int row, int col, void* user_data) {
    if (col == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Row%d", row);
        return cxlsb_cell_text(buf);
    } else if (col == 1) {
        return cxlsb_cell_number(row * 100.0);
    }
    return cxlsb_cell_blank();
}

static void test_writer_create(void) {
    cxlsb_writer_t* writer = cxlsb_writer_create("test_simple.xlsb");
    assert(writer != NULL);
    assert(writer->sheet_count == 0);
    
    cxlsb_writer_destroy(writer);
    printf("test_writer_create: PASS\n");
}

static void test_writer_write_batch(void) {
    cxlsb_writer_t* writer = cxlsb_writer_create("test_batch.xlsb");
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Data",
                                                   simple_supplier, NULL,
                                                   100, 2);
    assert(err == CXLSB_OK);
    assert(writer->sheet_count == 1);
    
    cxlsb_writer_destroy(writer);
    
    assert(access("test_batch.xlsb", F_OK) == 0);
    
    printf("test_writer_write_batch: PASS\n");
}

static void test_writer_streaming(void) {
    cxlsb_writer_t* writer = cxlsb_writer_create("test_stream.xlsb");
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_writer_start_sheet(writer, "Stream", 2);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_write_rows(writer, 0, simple_supplier, NULL, 50);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_write_rows(writer, 50, simple_supplier, NULL, 50);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_end_sheet(writer);
    assert(err == CXLSB_OK);
    assert(writer->sheet_count == 1);
    
    cxlsb_writer_destroy(writer);
    
    assert(access("test_stream.xlsb", F_OK) == 0);
    
    printf("test_writer_streaming: PASS\n");
}

static void test_writer_multiple_sheets(void) {
    cxlsb_writer_t* writer = cxlsb_writer_create("test_multi.xlsb");
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1",
                                                   simple_supplier, NULL, 50, 2);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_write_batch(writer, "Sheet2",
                                   simple_supplier, NULL, 30, 2);
    assert(err == CXLSB_OK);
    
    assert(writer->sheet_count == 2);
    
    cxlsb_writer_destroy(writer);
    
    assert(access("test_multi.xlsb", F_OK) == 0);
    
    printf("test_writer_multiple_sheets: PASS\n");
}

int main(void) {
    test_writer_create();
    test_writer_write_batch();
    test_writer_streaming();
    test_writer_multiple_sheets();
    
    unlink("test_simple.xlsb");
    unlink("test_batch.xlsb");
    unlink("test_stream.xlsb");
    unlink("test_multi.xlsb");
    
    printf("\nAll writer tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add test/test_writer.c
git commit -m "test(api): add writer tests"
```

---

## Task 4: 构建并运行测试

- [ ] **Step 1: 重新构建**

```bash
cd build && cmake .. && make
```

- [ ] **Step 2: 运行测试**

```bash
./test_writer
```
Expected: 输出 "All writer tests passed!"

---

## Phase 7 完成检查

- [ ] xlsb_writer API 已实现
- [ ] write_batch 功能已实现
- [ ] 流式写入功能已实现
- [ ] 多 Sheet 支持已实现
- [ ] 测试已通过
- [ ] Git 已提交

**下一步:** 进入 Phase 8 - 读取器实现