# cxlsb Phase 9: 示例和集成测试实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 创建完整示例程序、集成测试、端到端测试

**Architecture:** 写入 → 读取 → 验证数据一致性

**Tech Stack:** C99

---

## Task 1: 创建写入示例

**Files:**
- Create: `examples/example_write.c`

- [ ] **Step 1: 写入内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cxlsb/xlsb.h"
#include "cxlsb/xlsb_writer.h"

cxlsb_cell_data_t product_supplier(int row, int col, void* user_data) {
    switch (col) {
        case 0:
            return cxlsb_cell_number(row + 1);
        case 1: {
            char name[32];
            snprintf(name, sizeof(name), "Product-%d", row + 1);
            return cxlsb_cell_text(name);
        }
        case 2:
            return cxlsb_cell_number((row + 1) * 100.0 + 50.0);
        case 3:
            return cxlsb_cell_number((rand() % 50) + 1);
        default:
            return cxlsb_cell_blank();
    }
}

int main(int argc, char** argv) {
    const char* filename = "example_products.xlsb";
    int row_count = 100;
    int col_count = 4;
    
    if (argc > 1) {
        filename = argv[1];
    }
    if (argc > 2) {
        row_count = atoi(argv[2]);
    }
    
    printf("Creating XLSB file: %s\n", filename);
    printf("Row count: %d, Column count: %d\n", row_count, col_count);
    
    cxlsb_writer_t* writer = cxlsb_writer_create(filename);
    if (writer == NULL) {
        fprintf(stderr, "Failed to create writer\n");
        return 1;
    }
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Products",
                                                  product_supplier, NULL,
                                                  row_count, col_count);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Write error: %s\n", cxlsb_error_string(err));
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    printf("Sheet 1 created with %d rows\n", row_count);
    
    cxlsb_writer_destroy(writer);
    
    printf("File created successfully!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add examples/example_write.c
git commit -m "docs: add example_write.c"
```

---

## Task 2: 创建读取示例

**Files:**
- Create: `examples/example_read.c`

- [ ] **Step 1: 写入内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include "cxlsb/xlsb.h"
#include "cxlsb/xlsb_reader.h"

static int total_rows = 0;
static int total_cells = 0;

void row_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    total_rows++;
    
    printf("Row %d: ", row);
    for (int i = 0; i < col_count; i++) {
        total_cells++;
        
        switch (cells[i].type) {
            case CXLSB_CELL_NUMBER:
                printf("%.2f ", cells[i].value.number);
                break;
            case CXLSB_CELL_TEXT:
                printf("%s ", cells[i].value.text);
                break;
            case CXLSB_CELL_BOOL:
                printf("%s ", cells[i].value.boolean ? "TRUE" : "FALSE");
                break;
            default:
                printf("(blank) ");
                break;
        }
    }
    printf("\n");
    
    for (int i = 0; i < col_count; i++) {
        if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
            free(cells[i].value.text);
        }
    }
}

int main(int argc, char** argv) {
    const char* filename = "example_products.xlsb";
    
    if (argc > 1) {
        filename = argv[1];
    }
    
    printf("Reading XLSB file: %s\n", filename);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(filename);
    if (reader == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return 1;
    }
    
    int sheet_count = cxlsb_reader_get_sheet_count(reader);
    printf("Sheet count: %d\n", sheet_count);
    
    for (int i = 0; i < sheet_count; i++) {
        total_rows = 0;
        total_cells = 0;
        
        printf("\n=== Sheet %d ===\n", i + 1);
        
        cxlsb_error_t err = cxlsb_reader_for_each_row(reader, i, row_handler, NULL);
        if (err != CXLSB_OK) {
            fprintf(stderr, "Read error: %s\n", cxlsb_error_string(err));
        }
        
        printf("\nTotal rows: %d, Total cells: %d\n", total_rows, total_cells);
    }
    
    cxlsb_reader_destroy(reader);
    
    printf("File read successfully!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add examples/example_read.c
git commit -m "docs: add example_read.c"
```

---

## Task 3: 创建流式写入示例

**Files:**
- Create: `examples/example_stream.c`

- [ ] **Step 1: 写入内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include "cxlsb/xlsb.h"
#include "cxlsb/xlsb_writer.h"

cxlsb_cell_data_t stream_supplier(int row, int col, void* user_data) {
    switch (col) {
        case 0:
            return cxlsb_cell_number(row);
        case 1: {
            char buf[32];
            snprintf(buf, sizeof(buf), "Item-%d", row);
            return cxlsb_cell_text(buf);
        }
        case 2:
            return cxlsb_cell_number(row * 10.0);
        default:
            return cxlsb_cell_blank();
    }
}

int main(int argc, char** argv) {
    const char* filename = "example_stream.xlsb";
    int total_rows = 10000;
    int batch_size = 1000;
    int col_count = 3;
    
    if (argc > 1) filename = argv[1];
    if (argc > 2) total_rows = atoi(argv[2]);
    if (argc > 3) batch_size = atoi(argv[3]);
    
    printf("Streaming write to: %s\n", filename);
    printf("Total rows: %d, Batch size: %d\n", total_rows, batch_size);
    
    cxlsb_writer_t* writer = cxlsb_writer_create(filename);
    if (writer == NULL) {
        fprintf(stderr, "Failed to create writer\n");
        return 1;
    }
    
    cxlsb_error_t err = cxlsb_writer_start_sheet(writer, "LargeData", col_count);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Start sheet error: %s\n", cxlsb_error_string(err));
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    int batches = total_rows / batch_size;
    for (int b = 0; b < batches; b++) {
        int start_row = b * batch_size;
        
        err = cxlsb_writer_write_rows(writer, start_row, 
                                       stream_supplier, NULL, batch_size);
        if (err != CXLSB_OK) {
            fprintf(stderr, "Write rows error: %s\n", cxlsb_error_string(err));
            cxlsb_writer_destroy(writer);
            return 1;
        }
        
        printf("Batch %d/%d written (%d rows)\n", b + 1, batches, batch_size);
    }
    
    int remaining = total_rows % batch_size;
    if (remaining > 0) {
        int start_row = batches * batch_size;
        err = cxlsb_writer_write_rows(writer, start_row,
                                       stream_supplier, NULL, remaining);
        if (err != CXLSB_OK) {
            fprintf(stderr, "Write remaining error: %s\n", cxlsb_error_string(err));
        }
        printf("Remaining %d rows written\n", remaining);
    }
    
    err = cxlsb_writer_end_sheet(writer);
    if (err != CXLSB_OK) {
        fprintf(stderr, "End sheet error: %s\n", cxlsb_error_string(err));
        cxlsb_writer_destroy(writer);
        return 1;
    }
    
    printf("Sheet completed with %d rows\n", total_rows);
    
    cxlsb_writer_destroy(writer);
    
    printf("File created successfully!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add examples/example_stream.c
git commit -m "docs: add example_stream.c"
```

---

## Task 4: 创建集成测试

**Files:**
- Create: `test/test_integration.c`

- [ ] **Step 1: 写入内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "cxlsb/xlsb.h"
#include "cxlsb/xlsb_writer.h"
#include "cxlsb/xlsb_reader.h"

static int verify_row_count = 0;
static int verify_col_count = 0;
static double expected_values[100][4];

cxlsb_cell_data_t verify_supplier(int row, int col, void* user_data) {
    switch (col) {
        case 0:
            expected_values[row][col] = row + 1.0;
            return cxlsb_cell_number(row + 1);
        case 1:
            expected_values[row][col] = 100.0 + row;
            return cxlsb_cell_number(100.0 + row);
        case 2:
            expected_values[row][col] = row % 2 == 0 ? 1.0 : 0.0;
            return cxlsb_cell_bool(row % 2 == 0);
        case 3: {
            char buf[32];
            snprintf(buf, sizeof(buf), "Text-%d", row);
            return cxlsb_cell_text(buf);
        }
        default:
            return cxlsb_cell_blank();
    }
}

void verify_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    verify_row_count++;
    verify_col_count = col_count;
    
    if (row < 100) {
        assert(col_count == 4);
        
        assert(cells[0].type == CXLSB_CELL_NUMBER);
        assert(cells[0].value.number == expected_values[row][0]);
        
        assert(cells[1].type == CXLSB_CELL_NUMBER);
        assert(cells[1].value.number == expected_values[row][1]);
        
        assert(cells[2].type == CXLSB_CELL_BOOL);
        bool expected_bool = expected_values[row][2] == 1.0;
        assert(cells[2].value.boolean == expected_bool);
        
        assert(cells[3].type == CXLSB_CELL_TEXT);
    }
    
    for (int i = 0; i < col_count; i++) {
        if (cells[i].type == CXLSB_CELL_TEXT && cells[i].value.text != NULL) {
            free(cells[i].value.text);
        }
    }
}

static void test_write_read_cycle(void) {
    const char* filename = "test_cycle.xlsb";
    
    printf("Test: Write-Read Cycle\n");
    
    cxlsb_writer_t* writer = cxlsb_writer_create(filename);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "TestSheet",
                                                   verify_supplier, NULL, 100, 4);
    assert(err == CXLSB_OK);
    
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(filename);
    assert(reader != NULL);
    
    verify_row_count = 0;
    verify_col_count = 0;
    
    err = cxlsb_reader_for_each_row(reader, 0, verify_handler, NULL);
    assert(err == CXLSB_OK);
    
    assert(verify_row_count == 100);
    assert(verify_col_count == 4);
    
    cxlsb_reader_destroy(reader);
    
    unlink(filename);
    
    printf("  PASS\n");
}

static void test_large_file(void) {
    const char* filename = "test_large.xlsb";
    int row_count = 1000;
    
    printf("Test: Large File (1000 rows)\n");
    
    cxlsb_writer_t* writer = cxlsb_writer_create(filename);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_writer_start_sheet(writer, "Large", 5);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_write_rows(writer, 0, verify_supplier, NULL, 500);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_write_rows(writer, 500, verify_supplier, NULL, 500);
    assert(err == CXLSB_OK);
    
    err = cxlsb_writer_end_sheet(writer);
    assert(err == CXLSB_OK);
    
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(filename);
    assert(reader != NULL);
    
    verify_row_count = 0;
    err = cxlsb_reader_for_each_row(reader, 0, verify_handler, NULL);
    assert(err == CXLSB_OK);
    
    assert(verify_row_count == 1000);
    
    cxlsb_reader_destroy(reader);
    
    unlink(filename);
    
    printf("  PASS\n");
}

static void test_multiple_sheets(void) {
    const char* filename = "test_multi_sheet.xlsb";
    
    printf("Test: Multiple Sheets\n");
    
    cxlsb_writer_t* writer = cxlsb_writer_create(filename);
    assert(writer != NULL);
    
    cxlsb_writer_write_batch(writer, "Sheet1", verify_supplier, NULL, 50, 4);
    cxlsb_writer_write_batch(writer, "Sheet2", verify_supplier, NULL, 30, 4);
    cxlsb_writer_write_batch(writer, "Sheet3", verify_supplier, NULL, 20, 4);
    
    cxlsb_writer_destroy(writer);
    
    cxlsb_reader_t* reader = cxlsb_reader_create(filename);
    assert(reader != NULL);
    
    int sheet_count = cxlsb_reader_get_sheet_count(reader);
    assert(sheet_count == 3);
    
    cxlsb_reader_destroy(reader);
    
    unlink(filename);
    
    printf("  PASS\n");
}

int main(void) {
    test_write_read_cycle();
    test_large_file();
    test_multiple_sheets();
    
    printf("\nAll integration tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add test/test_integration.c
git commit -m "test: add integration tests"
```

---

## Task 5: 构建、运行测试和示例

- [ ] **Step 1: 重新构建**

```bash
cd build && cmake .. && make
```

- [ ] **Step 2: 运行集成测试**

```bash
./test_integration
```
Expected: 输出 "All integration tests passed!"

- [ ] **Step 3: 运行写入示例**

```bash
./example_write output.xlsb 100
```
Expected: 输出 "File created successfully!"

- [ ] **Step 4: 运行读取示例**

```bash
./example_read output.xlsb
```
Expected: 输出读取的数据

- [ ] **Step 5: 清理测试文件**

```bash
rm -f output.xlsb
```

---

## Phase 9 完成检查

- [ ] 写入示例已创建
- [ ] 读取示例已创建
- [ ] 流式写入示例已创建
- [ ] 集成测试已创建
- [ ] 所有测试已通过
- [ ] Git 已提交

**下一步:** 最终提交和总结