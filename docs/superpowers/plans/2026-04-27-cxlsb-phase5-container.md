# cxlsb Phase 5: Container层（ZIP容器）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现 ZIP 容器写入器和读取器，使用 libzip

**Architecture:** libzip 封装，256KB 缓冲区优化

**Tech Stack:** C99, libzip

---

## Task 1: 创建 xlsb_container 头文件

**Files:**
- Create: `src/container/xlsb_container.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_XLSB_CONTAINER_H
#define CXLSB_XLSB_CONTAINER_H

#include <stdint.h>
#include <stddef.h>
#include "cxlsb/error_codes.h"

typedef struct cxlsb_container {
    void* zip_handle;
    char* path;
    bool is_open;
} cxlsb_container_t;

cxlsb_container_t* cxlsb_container_create(const char* path);

void cxlsb_container_destroy(cxlsb_container_t* container);

cxlsb_error_t cxlsb_container_add_entry(cxlsb_container_t* container,
                                          const char* name,
                                          const uint8_t* data,
                                          size_t length);

cxlsb_error_t cxlsb_container_close(cxlsb_container_t* container);

#endif /* CXLSB_XLSB_CONTAINER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/container/xlsb_container.h
git commit -m "feat(container): add xlsb_container.h header"
```

---

## Task 2: 实现 xlsb_container

**Files:**
- Create: `src/container/xlsb_container.c`

- [ ] **Step 1: 写入内容**

```c
#include "xlsb_container.h"
#include <zip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cxlsb_container_t* cxlsb_container_create(const char* path) {
    if (path == NULL) {
        return NULL;
    }
    
    cxlsb_container_t* container = (cxlsb_container_t*)malloc(sizeof(cxlsb_container_t));
    if (container == NULL) {
        return NULL;
    }
    
    container->path = strdup(path);
    if (container->path == NULL) {
        free(container);
        return NULL;
    }
    
    int error = 0;
    container->zip_handle = zip_open(path, ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (container->zip_handle == NULL) {
        free(container->path);
        free(container);
        return NULL;
    }
    
    container->is_open = true;
    
    return container;
}

void cxlsb_container_destroy(cxlsb_container_t* container) {
    if (container != NULL) {
        if (container->is_open && container->zip_handle != NULL) {
            zip_close(container->zip_handle);
        }
        if (container->path != NULL) {
            free(container->path);
        }
        free(container);
    }
}

cxlsb_error_t cxlsb_container_add_entry(cxlsb_container_t* container,
                                          const char* name,
                                          const uint8_t* data,
                                          size_t length) {
    if (container == NULL || name == NULL || data == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (!container->is_open || container->zip_handle == NULL) {
        return CXLSB_ERR_ZIP_OPEN_FAILED;
    }
    
    zip_source_t* source = zip_source_buffer(container->zip_handle, data, length, 0);
    if (source == NULL) {
        return CXLSB_ERR_ZIP_ADD_FAILED;
    }
    
    int64_t index = zip_file_add(container->zip_handle, name, source, ZIP_FL_OVERWRITE);
    if (index < 0) {
        zip_source_free(source);
        return CXLSB_ERR_ZIP_ADD_FAILED;
    }
    
    if (zip_set_file_compression(container->zip_handle, index, ZIP_CM_DEFLATE, 6) != 0) {
        return CXLSB_ERR_ZIP_ADD_FAILED;
    }
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_container_close(cxlsb_container_t* container) {
    if (container == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (!container->is_open || container->zip_handle == NULL) {
        return CXLSB_ERR_ZIP_OPEN_FAILED;
    }
    
    int result = zip_close(container->zip_handle);
    if (result != 0) {
        return CXLSB_ERR_ZIP_ADD_FAILED;
    }
    
    container->zip_handle = NULL;
    container->is_open = false;
    
    return CXLSB_OK;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/container/xlsb_container.c
git commit -m "feat(container): add xlsb_container.c implementation"
```

---

## Task 3: 创建 xlsb_container_reader 头文件

**Files:**
- Create: `src/container/xlsb_container_reader.h`

- [ ] **Step 1: 写入内容**

```c
#ifndef CXLSB_XLSB_CONTAINER_READER_H
#define CXLSB_XLSB_CONTAINER_READER_H

#include <stdint.h>
#include <stdbool.h>
#include "../io/buffer_reader.h"
#include "cxlsb/error_codes.h"
#include "cxlsb/types.h"

typedef struct cxlsb_container_reader {
    void* zip_handle;
    char* path;
    bool is_open;
    cxlsb_sheet_info_t* sheet_infos;
    uint32_t sheet_count;
} cxlsb_container_reader_t;

cxlsb_container_reader_t* cxlsb_container_reader_create(const char* path);

void cxlsb_container_reader_destroy(cxlsb_container_reader_t* reader);

bool cxlsb_container_reader_has_entry(cxlsb_container_reader_t* reader,
                                       const char* name);

cxlsb_error_t cxlsb_container_reader_get_entry(cxlsb_container_reader_t* reader,
                                                const char* name,
                                                uint8_t** out_data,
                                                size_t* out_length);

uint8_t* cxlsb_container_reader_get_entry_data(cxlsb_container_reader_t* reader,
                                                const char* name,
                                                size_t* out_length);

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sheet_stream(
    cxlsb_container_reader_t* reader,
    int sheet_index);

bool cxlsb_container_reader_has_sst(cxlsb_container_reader_t* reader);

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sst_stream(
    cxlsb_container_reader_t* reader);

uint32_t cxlsb_container_reader_get_sheet_count(cxlsb_container_reader_t* reader);

cxlsb_error_t cxlsb_container_reader_get_sheet_info(cxlsb_container_reader_t* reader,
                                                     int sheet_index,
                                                     cxlsb_sheet_info_t* info);

#endif /* CXLSB_XLSB_CONTAINER_READER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/container/xlsb_container_reader.h
git commit -m "feat(container): add xlsb_container_reader.h header"
```

---

## Task 4: 实现 xlsb_container_reader

**Files:**
- Create: `src/container/xlsb_container_reader.c`

- [ ] **Step 1: 写入内容**

```c
#include "xlsb_container_reader.h"
#include "../format/biff12_record_types.h"
#include "../io/buffer_reader.h"
#include "../util/varint.h"
#include <zip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cxlsb_container_reader_t* cxlsb_container_reader_create(const char* path) {
    if (path == NULL) {
        return NULL;
    }
    
    cxlsb_container_reader_t* reader = (cxlsb_container_reader_t*)malloc(sizeof(cxlsb_container_reader_t));
    if (reader == NULL) {
        return NULL;
    }
    
    reader->path = strdup(path);
    if (reader->path == NULL) {
        free(reader);
        return NULL;
    }
    
    int error = 0;
    reader->zip_handle = zip_open(path, ZIP_RDONLY, &error);
    if (reader->zip_handle == NULL) {
        free(reader->path);
        free(reader);
        return NULL;
    }
    
    reader->is_open = true;
    reader->sheet_infos = NULL;
    reader->sheet_count = 0;
    
    return reader;
}

void cxlsb_container_reader_destroy(cxlsb_container_reader_t* reader) {
    if (reader != NULL) {
        if (reader->sheet_infos != NULL) {
            for (uint32_t i = 0; i < reader->sheet_count; i++) {
                if (reader->sheet_infos[i].name != NULL) {
                    free(reader->sheet_infos[i].name);
                }
            }
            free(reader->sheet_infos);
        }
        if (reader->is_open && reader->zip_handle != NULL) {
            zip_close(reader->zip_handle);
        }
        if (reader->path != NULL) {
            free(reader->path);
        }
        free(reader);
    }
}

bool cxlsb_container_reader_has_entry(cxlsb_container_reader_t* reader,
                                       const char* name) {
    if (reader == NULL || name == NULL || !reader->is_open) {
        return false;
    }
    
    int64_t index = zip_name_locate(reader->zip_handle, name, 0);
    return index >= 0;
}

cxlsb_error_t cxlsb_container_reader_get_entry(cxlsb_container_reader_t* reader,
                                                const char* name,
                                                uint8_t** out_data,
                                                size_t* out_length) {
    if (reader == NULL || name == NULL || out_data == NULL || out_length == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (!reader->is_open || reader->zip_handle == NULL) {
        return CXLSB_ERR_ZIP_OPEN_FAILED;
    }
    
    int64_t index = zip_name_locate(reader->zip_handle, name, 0);
    if (index < 0) {
        return CXLSB_ERR_ZIP_ENTRY_NOT_FOUND;
    }
    
    struct zip_stat st;
    if (zip_stat_index(reader->zip_handle, index, 0, &st) != 0) {
        return CXLSB_ERR_ZIP_READ_FAILED;
    }
    
    *out_length = st.size;
    *out_data = (uint8_t*)malloc(st.size);
    if (*out_data == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    zip_file_t* file = zip_fopen_index(reader->zip_handle, index, 0);
    if (file == NULL) {
        free(*out_data);
        *out_data = NULL;
        return CXLSB_ERR_ZIP_READ_FAILED;
    }
    
    zip_int64_t bytes_read = zip_fread(file, *out_data, st.size);
    zip_fclose(file);
    
    if (bytes_read != (zip_int64_t)st.size) {
        free(*out_data);
        *out_data = NULL;
        return CXLSB_ERR_ZIP_READ_FAILED;
    }
    
    return CXLSB_OK;
}

uint8_t* cxlsb_container_reader_get_entry_data(cxlsb_container_reader_t* reader,
                                                const char* name,
                                                size_t* out_length) {
    uint8_t* data = NULL;
    cxlsb_error_t err = cxlsb_container_reader_get_entry(reader, name, &data, out_length);
    if (err != CXLSB_OK) {
        return NULL;
    }
    return data;
}

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sheet_stream(
    cxlsb_container_reader_t* reader,
    int sheet_index) {
    if (reader == NULL || sheet_index < 0) {
        return NULL;
    }
    
    char name[64];
    snprintf(name, sizeof(name), "xl/worksheets/sheet%d.bin", sheet_index + 1);
    
    size_t length;
    uint8_t* data = cxlsb_container_reader_get_entry_data(reader, name, &length);
    if (data == NULL) {
        return NULL;
    }
    
    return cxlsb_buffer_reader_create(data, length);
}

bool cxlsb_container_reader_has_sst(cxlsb_container_reader_t* reader) {
    return cxlsb_container_reader_has_entry(reader, "xl/sharedStrings.bin");
}

cxlsb_buffer_reader_t* cxlsb_container_reader_get_sst_stream(
    cxlsb_container_reader_t* reader) {
    if (reader == NULL) {
        return NULL;
    }
    
    size_t length;
    uint8_t* data = cxlsb_container_reader_get_entry_data(reader, 
                                                           "xl/sharedStrings.bin", 
                                                           &length);
    if (data == NULL) {
        return NULL;
    }
    
    return cxlsb_buffer_reader_create(data, length);
}

uint32_t cxlsb_container_reader_get_sheet_count(cxlsb_container_reader_t* reader) {
    if (reader == NULL || !reader->is_open) {
        return 0;
    }
    
    int64_t num_entries = zip_get_num_entries(reader->zip_handle, 0);
    
    uint32_t count = 0;
    for (int64_t i = 0; i < num_entries; i++) {
        const char* name = zip_get_name(reader->zip_handle, i, 0);
        if (name != NULL && strncmp(name, "xl/worksheets/sheet", 19) == 0) {
            const char* ext = strstr(name, ".bin");
            if (ext != NULL && ext[4] == '\0') {
                count++;
            }
        }
    }
    
    return count;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/container/xlsb_container_reader.c
git commit -m "feat(container): add xlsb_container_reader.c implementation"
```

---

## Task 5: 创建 Container 层测试

**Files:**
- Create: `test/test_container.c`

- [ ] **Step 1: 写入测试内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "src/container/xlsb_container.h"
#include "src/container/xlsb_container_reader.h"
#include "src/util/xml_gen.h"
#include "src/util/rels_gen.h"

static void test_container_create_and_add(void) {
    cxlsb_container_t* container = cxlsb_container_create("test_output.zip");
    assert(container != NULL);
    assert(container->is_open);
    
    uint8_t* theme;
    size_t theme_len;
    cxlsb_error_t err = cxlsb_xml_gen_theme(&theme, &theme_len);
    assert(err == CXLSB_OK);
    
    err = cxlsb_container_add_entry(container, "xl/theme/theme1.xml", theme, theme_len);
    assert(err == CXLSB_OK);
    free(theme);
    
    const char* test_data = "Hello World";
    err = cxlsb_container_add_entry(container, "test.txt", 
                                     (const uint8_t*)test_data, strlen(test_data));
    assert(err == CXLSB_OK);
    
    err = cxlsb_container_close(container);
    assert(err == CXLSB_OK);
    
    cxlsb_container_destroy(container);
    
    assert(access("test_output.zip", F_OK) == 0);
    
    printf("test_container_create_and_add: PASS\n");
}

static void test_container_reader_open(void) {
    cxlsb_container_t* container = cxlsb_container_create("test_reader.zip");
    
    uint8_t* app;
    size_t app_len;
    cxlsb_xml_gen_app(1, &app, &app_len);
    cxlsb_container_add_entry(container, "docProps/app.xml", app, app_len);
    free(app);
    
    cxlsb_container_close(container);
    cxlsb_container_destroy(container);
    
    cxlsb_container_reader_t* reader = cxlsb_container_reader_create("test_reader.zip");
    assert(reader != NULL);
    assert(reader->is_open);
    
    bool has = cxlsb_container_reader_has_entry(reader, "docProps/app.xml");
    assert(has);
    
    uint8_t* data;
    size_t length;
    cxlsb_error_t err = cxlsb_container_reader_get_entry(reader, "docProps/app.xml", 
                                                          &data, &length);
    assert(err == CXLSB_OK);
    assert(length > 0);
    assert(strncmp((char*)data, "<?xml", 5) == 0);
    free(data);
    
    cxlsb_container_reader_destroy(reader);
    
    printf("test_container_reader_open: PASS\n");
}

static void test_container_missing_entry(void) {
    cxlsb_container_reader_t* reader = cxlsb_container_reader_create("test_reader.zip");
    assert(reader != NULL);
    
    bool has = cxlsb_container_reader_has_entry(reader, "nonexistent.txt");
    assert(!has);
    
    cxlsb_container_reader_destroy(reader);
    
    printf("test_container_missing_entry: PASS\n");
}

int main(void) {
    test_container_create_and_add();
    test_container_reader_open();
    test_container_missing_entry();
    
    unlink("test_output.zip");
    unlink("test_reader.zip");
    
    printf("\nAll container tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add test/test_container.c
git commit -m "test(container): add container tests"
```

---

## Task 6: 构建并运行测试

- [ ] **Step 1: 重新构建**

```bash
cd build && cmake .. && make
```

- [ ] **Step 2: 运行测试**

```bash
./test_container
```
Expected: 输出 "All container tests passed!"

---

## Phase 5 完成检查

- [ ] ZIP 容器写入器已实现
- [ ] ZIP 容器读取器已实现
- [ ] 条目添加和读取已实现
- [ ] 测试已通过
- [ ] Git 已提交

**下一步:** 进入 Phase 6 - Format层高级（Sheet/Workbook写入器）