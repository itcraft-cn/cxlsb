# cxlsb Phase 1: 项目骨架与目录结构 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 创建项目目录结构、CMake 配置、公共头文件定义

**Architecture:** 标准 C 项目结构，include/src 分离，CMake 构建

**Tech Stack:** C99, CMake 3.x

---

## Task 1: 创建目录结构

**Files:**
- Create: 目录结构

- [ ] **Step 1: 创建源码目录**

```bash
mkdir -p include/cxlsb
mkdir -p src/api src/container src/format src/io src/util
mkdir -p test examples
```

- [ ] **Step 2: 创建文档目录**

```bash
mkdir -p docs/superpowers/specs docs/superpowers/plans docs/analysis
```

- [ ] **Step 3: 验证目录结构**

```bash
ls -la include/cxlsb src
```
Expected: 显示创建的目录

---

## Task 2: 创建公共头文件 types.h

**Files:**
- Create: `include/cxlsb/types.h`

- [ ] **Step 1: 写入 types.h 内容**

```c
#ifndef CXLSB_TYPES_H
#define CXLSB_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum cxlsb_cell_type {
    CXLSB_CELL_BLANK = 0,
    CXLSB_CELL_NUMBER = 1,
    CXLSB_CELL_TEXT = 2,
    CXLSB_CELL_BOOL = 3,
    CXLSB_CELL_DATE = 4,
    CXLSB_CELL_ERROR = 5
} cxlsb_cell_type_t;

typedef struct cxlsb_cell_data {
    cxlsb_cell_type_t type;
    union {
        double number;
        const char* text;
        bool boolean;
        int error_code;
    } value;
} cxlsb_cell_data_t;

typedef cxlsb_cell_data_t (*cxlsb_cell_supplier_fn)(int row, int col, void* user_data);

typedef void (*cxlsb_row_handler_fn)(int row, int col_count, 
                                      cxlsb_cell_data_t* cells, void* user_data);

typedef void (*cxlsb_cell_handler_fn)(int row, int col, 
                                       cxlsb_cell_data_t* cell, void* user_data);

typedef struct cxlsb_sheet_info {
    int index;
    char* name;
    int row_count;
    int col_count;
} cxlsb_sheet_info_t;

typedef struct cxlsb_cell_style {
    int font_id;
    int fill_id;
    int border_id;
    int num_format_id;
    uint8_t horizontal_align;
    uint8_t vertical_align;
} cxlsb_cell_style_t;

typedef struct cxlsb_merge_cell {
    uint32_t first_row;
    uint32_t first_col;
    uint32_t last_row;
    uint32_t last_col;
} cxlsb_merge_cell_t;

#endif /* CXLSB_TYPES_H */
```

- [ ] **Step 2: 验证头文件**

```bash
cat include/cxlsb/types.h
```
Expected: 显示完整内容

- [ ] **Step 3: Commit**

```bash
git add include/cxlsb/types.h
git commit -m "feat: add types.h with common type definitions"
```

---

## Task 3: 创建错误码头文件 error_codes.h

**Files:**
- Create: `include/cxlsb/error_codes.h`

- [ ] **Step 1: 写入 error_codes.h 内容**

```c
#ifndef CXLSB_ERROR_CODES_H
#define CXLSB_ERROR_CODES_H

typedef enum cxlsb_error {
    CXLSB_OK = 0,
    
    CXLSB_ERR_NULL_PTR = -1,
    CXLSB_ERR_NO_MEMORY = -2,
    CXLSB_ERR_INVALID_ARG = -3,
    CXLSB_ERR_BUFFER_OVERFLOW = -4,
    
    CXLSB_ERR_FILE_NOT_FOUND = -10,
    CXLSB_ERR_FILE_OPEN_FAILED = -11,
    CXLSB_ERR_FILE_WRITE_FAILED = -12,
    CXLSB_ERR_FILE_READ_FAILED = -13,
    
    CXLSB_ERR_ZIP_OPEN_FAILED = -20,
    CXLSB_ERR_ZIP_ADD_FAILED = -21,
    CXLSB_ERR_ZIP_ENTRY_NOT_FOUND = -22,
    CXLSB_ERR_ZIP_READ_FAILED = -23,
    
    CXLSB_ERR_INVALID_RECORD = -30,
    CXLSB_ERR_INVALID_CELL_TYPE = -31,
    CXLSB_ERR_INVALID_SHEET_INDEX = -32,
    CXLSB_ERR_PARSE_FAILED = -33,
    
    CXLSB_ERR_NOT_IN_TEMPLATE_MODE = -40,
    CXLSB_ERR_IN_TEMPLATE_MODE = -41,
    CXLSB_ERR_SHEET_NOT_STARTED = -42,
    CXLSB_ERR_SHEET_NOT_ENDED = -43,
    CXLSB_ERR_FILL_NOT_STARTED = -44,
    
    CXLSB_ERR_MARKER_NOT_FOUND = -50,
    CXLSB_ERR_TEMPLATE_LOAD_FAILED = -51
} cxlsb_error_t;

const char* cxlsb_error_string(cxlsb_error_t error);

#endif /* CXLSB_ERROR_CODES_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/cxlsb/error_codes.h
git commit -m "feat: add error_codes.h with error definitions"
```

---

## Task 4: 创建错误码实现文件

**Files:**
- Create: `src/util/error_codes.c`

- [ ] **Step 1: 写入 error_codes.c 内容**

```c
#include "cxlsb/error_codes.h"

const char* cxlsb_error_string(cxlsb_error_t error) {
    switch (error) {
        case CXLSB_OK: return "Success";
        case CXLSB_ERR_NULL_PTR: return "Null pointer";
        case CXLSB_ERR_NO_MEMORY: return "No memory";
        case CXLSB_ERR_INVALID_ARG: return "Invalid argument";
        case CXLSB_ERR_BUFFER_OVERFLOW: return "Buffer overflow";
        case CXLSB_ERR_FILE_NOT_FOUND: return "File not found";
        case CXLSB_ERR_FILE_OPEN_FAILED: return "File open failed";
        case CXLSB_ERR_FILE_WRITE_FAILED: return "File write failed";
        case CXLSB_ERR_FILE_READ_FAILED: return "File read failed";
        case CXLSB_ERR_ZIP_OPEN_FAILED: return "ZIP open failed";
        case CXLSB_ERR_ZIP_ADD_FAILED: return "ZIP add entry failed";
        case CXLSB_ERR_ZIP_ENTRY_NOT_FOUND: return "ZIP entry not found";
        case CXLSB_ERR_ZIP_READ_FAILED: return "ZIP read failed";
        case CXLSB_ERR_INVALID_RECORD: return "Invalid BIFF12 record";
        case CXLSB_ERR_INVALID_CELL_TYPE: return "Invalid cell type";
        case CXLSB_ERR_INVALID_SHEET_INDEX: return "Invalid sheet index";
        case CXLSB_ERR_PARSE_FAILED: return "Parse failed";
        case CXLSB_ERR_NOT_IN_TEMPLATE_MODE: return "Not in template mode";
        case CXLSB_ERR_IN_TEMPLATE_MODE: return "In template mode";
        case CXLSB_ERR_SHEET_NOT_STARTED: return "Sheet not started";
        case CXLSB_ERR_SHEET_NOT_ENDED: return "Sheet not ended";
        case CXLSB_ERR_FILL_NOT_STARTED: return "Fill not started";
        case CXLSB_ERR_MARKER_NOT_FOUND: return "Marker not found";
        case CXLSB_ERR_TEMPLATE_LOAD_FAILED: return "Template load failed";
        default: return "Unknown error";
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add src/util/error_codes.c
git commit -m "feat: add error_codes.c implementation"
```

---

## Task 5: 创建单元格数据辅助函数头文件

**Files:**
- Create: `include/cxlsb/cell_data.h`

- [ ] **Step 1: 写入 cell_data.h 内容**

```c
#ifndef CXLSB_CELL_DATA_H
#define CXLSB_CELL_DATA_H

#include "types.h"

cxlsb_cell_data_t cxlsb_cell_blank(void);
cxlsb_cell_data_t cxlsb_cell_number(double value);
cxlsb_cell_data_t cxlsb_cell_text(const char* text);
cxlsb_cell_data_t cxlsb_cell_bool(bool value);
cxlsb_cell_data_t cxlsb_cell_date(double excel_date);
cxlsb_cell_data_t cxlsb_cell_error(int error_code);

double cxlsb_date_from_timestamp(uint64_t timestamp);
uint64_t cxlsb_date_to_timestamp(double excel_date);

#endif /* CXLSB_CELL_DATA_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/cxlsb/cell_data.h
git commit -m "feat: add cell_data.h header"
```

---

## Task 6: 创建单元格数据辅助函数实现

**Files:**
- Create: `src/api/cell_data.c`

- [ ] **Step 1: 写入 cell_data.c 内容**

```c
#include "cxlsb/cell_data.h"

cxlsb_cell_data_t cxlsb_cell_blank(void) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_BLANK;
    cell.value.number = 0.0;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_number(double value) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_NUMBER;
    cell.value.number = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_text(const char* text) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_TEXT;
    cell.value.text = text;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_bool(bool value) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_BOOL;
    cell.value.boolean = value;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_date(double excel_date) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_DATE;
    cell.value.number = excel_date;
    return cell;
}

cxlsb_cell_data_t cxlsb_cell_error(int error_code) {
    cxlsb_cell_data_t cell;
    cell.type = CXLSB_CELL_ERROR;
    cell.value.error_code = error_code;
    return cell;
}

double cxlsb_date_from_timestamp(uint64_t timestamp) {
    return (double)(timestamp / 86400000.0) + 25569.0;
}

uint64_t cxlsb_date_to_timestamp(double excel_date) {
    return (uint64_t)((excel_date - 25569.0) * 86400000.0);
}
```

- [ ] **Step 2: Commit**

```bash
git add src/api/cell_data.c
git commit -m "feat: add cell_data.c implementation"
```

---

## Task 7: 创建 CMakeLists.txt

**Files:**
- Create: `CMakeLists.txt`

- [ ] **Step 1: 写入 CMakeLists.txt 内容**

```cmake
cmake_minimum_required(VERSION 3.10)
project(cxlsb VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -g)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

find_package(ZLIB REQUIRED)
find_library(LIBZIP_LIBRARY zip)
find_path(LIBZIP_INCLUDE_DIR zip.h PATHS /usr/include)

set(ZLOG_ROOT "/home/helly/open_source/github/zlog")
set(ZLOG_INCLUDE_DIR "${ZLOG_ROOT}/src")
set(ZLOG_LIBRARY "${ZLOG_ROOT}/libzlog.so")

if(NOT EXISTS "${ZLOG_LIBRARY}")
    message(WARNING "zlog library not found at ${ZLOG_LIBRARY}, building without zlog")
    set(ZLOG_AVAILABLE OFF)
else()
    set(ZLOG_AVAILABLE ON)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${LIBZIP_INCLUDE_DIR}
)

if(ZLOG_AVAILABLE)
    include_directories(${ZLOG_INCLUDE_DIR})
endif()

file(GLOB_RECURSE CXLSB_SOURCES "src/*.c")

add_library(cxlsb SHARED ${CXLSB_SOURCES})
target_link_libraries(cxlsb ${LIBZIP_LIBRARY} ${ZLIB_LIBRARIES})

if(ZLOG_AVAILABLE)
    target_link_libraries(cxlsb ${ZLOG_LIBRARY})
endif()

set_target_properties(cxlsb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "cxlsb"
)

install(TARGETS cxlsb
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/cxlsb
    DESTINATION include
)

enable_testing()

file(GLOB TEST_SOURCES "test/*.c")
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} cxlsb)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

file(GLOB EXAMPLE_SOURCES "examples/*.c")
foreach(EXAMPLE_FILE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    target_link_libraries(${EXAMPLE_NAME} cxlsb)
endforeach()
```

- [ ] **Step 2: Commit**

```bash
git add CMakeLists.txt
git commit -m "feat: add CMakeLists.txt build configuration"
```

---

## Task 8: 创建主入口头文件

**Files:**
- Create: `include/cxlsb/xlsb.h`

- [ ] **Step 1: 写入 xlsb.h 内容**

```c
#ifndef CXLSB_XLSB_H
#define CXLSB_XLSB_H

#include "types.h"
#include "error_codes.h"
#include "cell_data.h"

#endif /* CXLSB_XLSB_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/cxlsb/xlsb.h
git commit -m "feat: add main header xlsb.h"
```

---

## Task 9: 创建 README.md

**Files:**
- Create: `README.md`

- [ ] **Step 1: 写入 README.md 内容**

```markdown
# cxlsb - C XLSB Library

A pure C implementation of XLSB (Excel Binary Workbook) format reader/writer library.

## Features

- **Zero Dependencies**: Only libzip and optional zlog
- **Cross-Platform**: Linux and Windows support
- **High Performance**: Dynamic buffer + streaming for large files
- **Full Functionality**: Aligned with Java version jxlsb
- **Functional API**: Pure functions + callback pointers

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

### Write Example

```c
#include "cxlsb/xlsb.h"

cxlsb_cell_data_t supplier(int row, int col, void* data) {
    if (col == 0) return cxlsb_cell_text("Name");
    return cxlsb_cell_number(row * 100.0);
}

int main() {
    cxlsb_writer_t* w = cxlsb_writer_create("output.xlsb");
    cxlsb_writer_write_batch(w, "Sheet1", supplier, NULL, 1000, 2);
    cxlsb_writer_destroy(w);
    return 0;
}
```

### Read Example

```c
void handler(int row, int col_count, cxlsb_cell_data_t* cells, void* data) {
    for (int i = 0; i < col_count; i++) {
        if (cells[i].type == CXLSB_CELL_TEXT) {
            printf("%s\n", cells[i].value.text);
        }
    }
}

int main() {
    cxlsb_reader_t* r = cxlsb_reader_create("data.xlsb");
    cxlsb_reader_for_each_row(r, 0, handler, NULL);
    cxlsb_reader_destroy(r);
    return 0;
}
```

## License

Apache License 2.0
```

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: add README.md"
```

---

## Task 10: 创建 .gitignore

**Files:**
- Create: `.gitignore`

- [ ] **Step 1: 写入 .gitignore 内容**

```
build/
*.o
*.so
*.a
*.xlsb
*.xlsx
test/output/
MEMORY.md
```

- [ ] **Step 2: Commit**

```bash
git add .gitignore
git commit -m "chore: add .gitignore"
```

---

## Task 11: 验证构建骨架

- [ ] **Step 1: 创建 build 目录并运行 cmake**

```bash
mkdir -p build && cd build && cmake ..
```
Expected: 显示配置成功

- [ ] **Step 2: 尝试构建（会因缺少源文件报错，这是预期）**

```bash
make
```
Expected: 部分失败，但 CMake 配置正确

---

## Phase 1 完成检查

- [ ] 目录结构已创建
- [ ] 公共头文件已创建
- [ ] CMake 配置已创建
- [ ] Git 已提交

**下一步:** 进入 Phase 2 - IO层实现