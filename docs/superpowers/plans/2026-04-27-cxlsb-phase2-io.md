# cxlsb Phase 2: IO层（动态缓冲区）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现动态缓冲区写入器和读取器，支持小端序读写

**Architecture:** 动态扩展缓冲区，初始4KB，按需翻倍扩展；大文件使用固定16MB缓冲区

**Tech Stack:** C99, realloc 动态内存管理

---

## Task 1: 创建 buffer_writer 头文件

**Files:**
- Create: `src/io/buffer_writer.h`

- [ ] **Step 1: 写入 buffer_writer.h 内容**

```c
#ifndef CXLSB_BUFFER_WRITER_H
#define CXLSB_BUFFER_WRITER_H

#include <stdint.h>
#include <stddef.h>
#include "cxlsb/error_codes.h"

typedef struct cxlsb_buffer_writer {
    uint8_t* buffer;
    size_t capacity;
    size_t position;
    size_t initial_capacity;
    bool fixed_mode;
} cxlsb_buffer_writer_t;

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create(size_t initial_capacity);

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create_fixed(size_t capacity);

void cxlsb_buffer_writer_destroy(cxlsb_buffer_writer_t* writer);

cxlsb_error_t cxlsb_buffer_writer_write_byte(cxlsb_buffer_writer_t* writer, uint8_t value);

cxlsb_error_t cxlsb_buffer_writer_write_short(cxlsb_buffer_writer_t* writer, uint16_t value);

cxlsb_error_t cxlsb_buffer_writer_write_int(cxlsb_buffer_writer_t* writer, uint32_t value);

cxlsb_error_t cxlsb_buffer_writer_write_long(cxlsb_buffer_writer_t* writer, uint64_t value);

cxlsb_error_t cxlsb_buffer_writer_write_double(cxlsb_buffer_writer_t* writer, double value);

cxlsb_error_t cxlsb_buffer_writer_write_bytes(cxlsb_buffer_writer_t* writer, 
                                               const uint8_t* data, 
                                               size_t length);

size_t cxlsb_buffer_writer_get_position(cxlsb_buffer_writer_t* writer);

uint8_t* cxlsb_buffer_writer_get_buffer(cxlsb_buffer_writer_t* writer);

size_t cxlsb_buffer_writer_get_length(cxlsb_buffer_writer_t* writer);

void cxlsb_buffer_writer_reset(cxlsb_buffer_writer_t* writer);

cxlsb_error_t cxlsb_buffer_writer_ensure_capacity(cxlsb_buffer_writer_t* writer, 
                                                   size_t required);

#endif /* CXLSB_BUFFER_WRITER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/io/buffer_writer.h
git commit -m "feat(io): add buffer_writer.h header"
```

---

## Task 2: 实现 buffer_writer

**Files:**
- Create: `src/io/buffer_writer.c`

- [ ] **Step 1: 写入 buffer_writer.c 内容**

```c
#include "buffer_writer.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_INITIAL_CAPACITY 4096
#define LARGE_FILE_THRESHOLD 100000
#define LARGE_FILE_BUFFER_SIZE (16 * 1024 * 1024)

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create(size_t initial_capacity) {
    cxlsb_buffer_writer_t* writer = (cxlsb_buffer_writer_t*)malloc(sizeof(cxlsb_buffer_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_INITIAL_CAPACITY;
    }
    
    writer->buffer = (uint8_t*)malloc(initial_capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->capacity = initial_capacity;
    writer->initial_capacity = initial_capacity;
    writer->position = 0;
    writer->fixed_mode = false;
    
    return writer;
}

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create_fixed(size_t capacity) {
    cxlsb_buffer_writer_t* writer = (cxlsb_buffer_writer_t*)malloc(sizeof(cxlsb_buffer_writer_t));
    if (writer == NULL) {
        return NULL;
    }
    
    if (capacity == 0) {
        capacity = LARGE_FILE_BUFFER_SIZE;
    }
    
    writer->buffer = (uint8_t*)malloc(capacity);
    if (writer->buffer == NULL) {
        free(writer);
        return NULL;
    }
    
    writer->capacity = capacity;
    writer->initial_capacity = capacity;
    writer->position = 0;
    writer->fixed_mode = true;
    
    return writer;
}

void cxlsb_buffer_writer_destroy(cxlsb_buffer_writer_t* writer) {
    if (writer != NULL) {
        if (writer->buffer != NULL) {
            free(writer->buffer);
        }
        free(writer);
    }
}

cxlsb_error_t cxlsb_buffer_writer_ensure_capacity(cxlsb_buffer_writer_t* writer, 
                                                   size_t required) {
    size_t needed = writer->position + required;
    
    if (needed <= writer->capacity) {
        return CXLSB_OK;
    }
    
    if (writer->fixed_mode) {
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    size_t new_capacity = writer->capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    
    uint8_t* new_buffer = (uint8_t*)realloc(writer->buffer, new_capacity);
    if (new_buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    writer->buffer = new_buffer;
    writer->capacity = new_capacity;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_byte(cxlsb_buffer_writer_t* writer, uint8_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 1);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = value;
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_short(cxlsb_buffer_writer_t* writer, uint16_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 2);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = (uint8_t)(value & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 8) & 0xFF);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_int(cxlsb_buffer_writer_t* writer, uint32_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 4);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = (uint8_t)(value & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 8) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 16) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 24) & 0xFF);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_long(cxlsb_buffer_writer_t* writer, uint64_t value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 8);
    if (err != CXLSB_OK) {
        return err;
    }
    
    writer->buffer[writer->position++] = (uint8_t)(value & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 8) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 16) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 24) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 32) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 40) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 48) & 0xFF);
    writer->buffer[writer->position++] = (uint8_t)((value >> 56) & 0xFF);
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_buffer_writer_write_double(cxlsb_buffer_writer_t* writer, double value) {
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, 8);
    if (err != CXLSB_OK) {
        return err;
    }
    
    uint64_t bits;
    memcpy(&bits, &value, sizeof(double));
    
    return cxlsb_buffer_writer_write_long(writer, bits);
}

cxlsb_error_t cxlsb_buffer_writer_write_bytes(cxlsb_buffer_writer_t* writer, 
                                               const uint8_t* data, 
                                               size_t length) {
    if (data == NULL || length == 0) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    cxlsb_error_t err = cxlsb_buffer_writer_ensure_capacity(writer, length);
    if (err != CXLSB_OK) {
        return err;
    }
    
    memcpy(writer->buffer + writer->position, data, length);
    writer->position += length;
    
    return CXLSB_OK;
}

size_t cxlsb_buffer_writer_get_position(cxlsb_buffer_writer_t* writer) {
    return writer->position;
}

uint8_t* cxlsb_buffer_writer_get_buffer(cxlsb_buffer_writer_t* writer) {
    return writer->buffer;
}

size_t cxlsb_buffer_writer_get_length(cxlsb_buffer_writer_t* writer) {
    return writer->position;
}

void cxlsb_buffer_writer_reset(cxlsb_buffer_writer_t* writer) {
    writer->position = 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/io/buffer_writer.c
git commit -m "feat(io): add buffer_writer.c implementation"
```

---

## Task 3: 创建 buffer_reader 头文件

**Files:**
- Create: `src/io/buffer_reader.h`

- [ ] **Step 1: 写入 buffer_reader.h 内容**

```c
#ifndef CXLSB_BUFFER_READER_H
#define CXLSB_BUFFER_READER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

typedef struct cxlsb_buffer_reader {
    const uint8_t* buffer;
    size_t length;
    size_t position;
} cxlsb_buffer_reader_t;

cxlsb_buffer_reader_t* cxlsb_buffer_reader_create(const uint8_t* buffer, size_t length);

void cxlsb_buffer_reader_destroy(cxlsb_buffer_reader_t* reader);

uint8_t cxlsb_buffer_reader_read_byte(cxlsb_buffer_reader_t* reader);

uint16_t cxlsb_buffer_reader_read_short(cxlsb_buffer_reader_t* reader);

uint32_t cxlsb_buffer_reader_read_int(cxlsb_buffer_reader_t* reader);

uint64_t cxlsb_buffer_reader_read_long(cxlsb_buffer_reader_t* reader);

double cxlsb_buffer_reader_read_double(cxlsb_buffer_reader_t* reader);

cxlsb_error_t cxlsb_buffer_reader_read_bytes(cxlsb_buffer_reader_t* reader, 
                                              uint8_t* out, 
                                              size_t length);

size_t cxlsb_buffer_reader_get_position(cxlsb_buffer_reader_t* reader);

size_t cxlsb_buffer_reader_get_remaining(cxlsb_buffer_reader_t* reader);

bool cxlsb_buffer_reader_has_remaining(cxlsb_buffer_reader_t* reader, size_t required);

void cxlsb_buffer_reader_set_position(cxlsb_buffer_reader_t* reader, size_t position);

bool cxlsb_buffer_reader_is_eof(cxlsb_buffer_reader_t* reader);

#endif /* CXLSB_BUFFER_READER_H */
```

- [ ] **Step 2: Commit**

```bash
git add src/io/buffer_reader.h
git commit -m "feat(io): add buffer_reader.h header"
```

---

## Task 4: 实现 buffer_reader

**Files:**
- Create: `src/io/buffer_reader.c`

- [ ] **Step 1: 写入 buffer_reader.c 内容**

```c
#include "buffer_reader.h"
#include <stdlib.h>
#include <string.h>

cxlsb_buffer_reader_t* cxlsb_buffer_reader_create(const uint8_t* buffer, size_t length) {
    if (buffer == NULL) {
        return NULL;
    }
    
    cxlsb_buffer_reader_t* reader = (cxlsb_buffer_reader_t*)malloc(sizeof(cxlsb_buffer_reader_t));
    if (reader == NULL) {
        return NULL;
    }
    
    reader->buffer = buffer;
    reader->length = length;
    reader->position = 0;
    
    return reader;
}

void cxlsb_buffer_reader_destroy(cxlsb_buffer_reader_t* reader) {
    if (reader != NULL) {
        free(reader);
    }
}

uint8_t cxlsb_buffer_reader_read_byte(cxlsb_buffer_reader_t* reader) {
    if (reader->position >= reader->length) {
        return 0;
    }
    return reader->buffer[reader->position++];
}

uint16_t cxlsb_buffer_reader_read_short(cxlsb_buffer_reader_t* reader) {
    if (reader->position + 2 > reader->length) {
        reader->position = reader->length;
        return 0;
    }
    
    uint16_t value = (uint16_t)reader->buffer[reader->position];
    value |= (uint16_t)reader->buffer[reader->position + 1] << 8;
    reader->position += 2;
    
    return value;
}

uint32_t cxlsb_buffer_reader_read_int(cxlsb_buffer_reader_t* reader) {
    if (reader->position + 4 > reader->length) {
        reader->position = reader->length;
        return 0;
    }
    
    uint32_t value = (uint32_t)reader->buffer[reader->position];
    value |= (uint32_t)reader->buffer[reader->position + 1] << 8;
    value |= (uint32_t)reader->buffer[reader->position + 2] << 16;
    value |= (uint32_t)reader->buffer[reader->position + 3] << 24;
    reader->position += 4;
    
    return value;
}

uint64_t cxlsb_buffer_reader_read_long(cxlsb_buffer_reader_t* reader) {
    if (reader->position + 8 > reader->length) {
        reader->position = reader->length;
        return 0;
    }
    
    uint64_t value = (uint64_t)reader->buffer[reader->position];
    value |= (uint64_t)reader->buffer[reader->position + 1] << 8;
    value |= (uint64_t)reader->buffer[reader->position + 2] << 16;
    value |= (uint64_t)reader->buffer[reader->position + 3] << 24;
    value |= (uint64_t)reader->buffer[reader->position + 4] << 32;
    value |= (uint64_t)reader->buffer[reader->position + 5] << 40;
    value |= (uint64_t)reader->buffer[reader->position + 6] << 48;
    value |= (uint64_t)reader->buffer[reader->position + 7] << 56;
    reader->position += 8;
    
    return value;
}

double cxlsb_buffer_reader_read_double(cxlsb_buffer_reader_t* reader) {
    uint64_t bits = cxlsb_buffer_reader_read_long(reader);
    double value;
    memcpy(&value, &bits, sizeof(double));
    return value;
}

cxlsb_error_t cxlsb_buffer_reader_read_bytes(cxlsb_buffer_reader_t* reader, 
                                              uint8_t* out, 
                                              size_t length) {
    if (out == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    if (reader->position + length > reader->length) {
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    memcpy(out, reader->buffer + reader->position, length);
    reader->position += length;
    
    return CXLSB_OK;
}

size_t cxlsb_buffer_reader_get_position(cxlsb_buffer_reader_t* reader) {
    return reader->position;
}

size_t cxlsb_buffer_reader_get_remaining(cxlsb_buffer_reader_t* reader) {
    return reader->length - reader->position;
}

bool cxlsb_buffer_reader_has_remaining(cxlsb_buffer_reader_t* reader, size_t required) {
    return reader->position + required <= reader->length;
}

void cxlsb_buffer_reader_set_position(cxlsb_buffer_reader_t* reader, size_t position) {
    if (position <= reader->length) {
        reader->position = position;
    } else {
        reader->position = reader->length;
    }
}

bool cxlsb_buffer_reader_is_eof(cxlsb_buffer_reader_t* reader) {
    return reader->position >= reader->length;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/io/buffer_reader.c
git commit -m "feat(io): add buffer_reader.c implementation"
```

---

## Task 5: 创建 IO层测试文件

**Files:**
- Create: `test/test_buffer.c`

- [ ] **Step 1: 写入测试文件内容**

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "src/io/buffer_writer.h"
#include "src/io/buffer_reader.h"

static void test_buffer_writer_create(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(1024);
    assert(writer != NULL);
    assert(writer->capacity == 1024);
    assert(writer->position == 0);
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_writer_create: PASS\n");
}

static void test_buffer_writer_write_int(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(1024);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_buffer_writer_write_int(writer, 0x12345678);
    assert(err == CXLSB_OK);
    assert(writer->position == 4);
    
    uint8_t* buf = cxlsb_buffer_writer_get_buffer(writer);
    assert(buf[0] == 0x78);
    assert(buf[1] == 0x56);
    assert(buf[2] == 0x34);
    assert(buf[3] == 0x12);
    
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_writer_write_int: PASS\n");
}

static void test_buffer_writer_expand(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(16);
    assert(writer != NULL);
    assert(writer->capacity == 16);
    
    for (int i = 0; i < 10; i++) {
        cxlsb_error_t err = cxlsb_buffer_writer_write_int(writer, i);
        assert(err == CXLSB_OK);
    }
    
    assert(writer->position == 40);
    assert(writer->capacity >= 40);
    
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_writer_expand: PASS\n");
}

static void test_buffer_reader_read_int(void) {
    uint8_t data[] = {0x78, 0x56, 0x34, 0x12, 0x01, 0x00, 0x00, 0x00};
    
    cxlsb_buffer_reader_t* reader = cxlsb_buffer_reader_create(data, 8);
    assert(reader != NULL);
    
    uint32_t val1 = cxlsb_buffer_reader_read_int(reader);
    assert(val1 == 0x12345678);
    assert(reader->position == 4);
    
    uint32_t val2 = cxlsb_buffer_reader_read_int(reader);
    assert(val2 == 1);
    assert(reader->position == 8);
    
    cxlsb_buffer_reader_destroy(reader);
    printf("test_buffer_reader_read_int: PASS\n");
}

static void test_buffer_reader_eof(void) {
    uint8_t data[] = {0x01, 0x02};
    
    cxlsb_buffer_reader_t* reader = cxlsb_buffer_reader_create(data, 2);
    assert(reader != NULL);
    
    cxlsb_buffer_reader_read_int(reader);
    assert(cxlsb_buffer_reader_is_eof(reader));
    
    cxlsb_buffer_reader_destroy(reader);
    printf("test_buffer_reader_eof: PASS\n");
}

static void test_buffer_write_read_cycle(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(1024);
    
    cxlsb_buffer_writer_write_int(writer, 100);
    cxlsb_buffer_writer_write_double(writer, 3.14159);
    cxlsb_buffer_writer_write_byte(writer, 42);
    
    uint8_t* buf = cxlsb_buffer_writer_get_buffer(writer);
    size_t len = cxlsb_buffer_writer_get_length(writer);
    
    cxlsb_buffer_reader_t* reader = cxlsb_buffer_reader_create(buf, len);
    
    uint32_t int_val = cxlsb_buffer_reader_read_int(reader);
    assert(int_val == 100);
    
    double double_val = cxlsb_buffer_reader_read_double(reader);
    assert(double_val > 3.14158 && double_val < 3.14160);
    
    uint8_t byte_val = cxlsb_buffer_reader_read_byte(reader);
    assert(byte_val == 42);
    
    cxlsb_buffer_reader_destroy(reader);
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_write_read_cycle: PASS\n");
}

int main(void) {
    test_buffer_writer_create();
    test_buffer_writer_write_int();
    test_buffer_writer_expand();
    test_buffer_reader_read_int();
    test_buffer_reader_eof();
    test_buffer_write_read_cycle();
    
    printf("\nAll buffer tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add test/test_buffer.c
git commit -m "test(io): add buffer tests"
```

---

## Task 6: 构建并运行测试

- [ ] **Step 1: 重新配置 CMake**

```bash
cd build && cmake ..
```

- [ ] **Step 2: 构建项目**

```bash
make
```
Expected: 编译成功

- [ ] **Step 3: 运行测试**

```bash
./test_buffer
```
Expected: 输出 "All buffer tests passed!"

---

## Phase 2 完成检查

- [ ] buffer_writer.h/c 已创建
- [ ] buffer_reader.h/c 已创建
- [ ] 小端序读写已实现
- [ ] 动态扩展功能已实现
- [ ] 测试已通过
- [ ] Git 已提交

**下一步:** 进入 Phase 3 - Util层实现