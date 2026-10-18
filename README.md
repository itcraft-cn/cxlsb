# cxlsb - ANSI C XLSB 库

**中文** | [English](README_en.md)

纯 ANSI C 实现的 XLSB (Excel 二进制工作簿) 格式读写库，移植自 Java [jxlsb](https://github.com/itcraft-cn/jxlsb) 项目。

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C Standard](https://img.shields.io/badge/C-C99-green.svg)](https://en.wikipedia.org/wiki/C99)

## 设计定位

本库的出发点始终明确：**做高性能的数据输出（写入）**。读取最初只是为了读模板；但读一旦做了，高性能读也就顺理成章地成为同一内核的自然延伸。

站在这个窄应用视角看，XLSX 其实并不需要——XLSB 已经非常能打。

## 特性

- **零依赖**: 仅需要 libzip 处理 ZIP 容器
- **跨平台**: 支持 Linux 和 Windows
- **高性能**:
  - 行级缓存避免重复 supplier 调用
  - 动态缓冲区大小按数据维度计算
  - 流式模式支持大数据集 (>100万行)
- **功能完整**: 与 Java jxlsb 实现对齐
- **函数式 API**: 纯函数 + 回调指针，灵活易用
- **内存安全**: 完整的 malloc NULL 检查，create/destroy 模式

## 支持操作

| 功能 | API | 描述 |
|------|-----|------|
| 批量写入 | `cxlsb_writer_write_batch` | 单次调用写入整张工作表 |
| 流式写入 | `cxlsb_writer_start_sheet` + `write_rows` + `end_sheet` | 逐批追加行 |
| 行迭代 | `cxlsb_reader_for_each_row` | 回调处理每行数据 |
| 模板填充 (标记) | `cxlsb_template_filler_fill_marker` | 在 `${data}` 标记位置填充 |
| 模板填充 (索引) | `cxlsb_template_filler_fill_index` | 在指定行列位置填充 |
| SST 管理 | 共享字符串表 | 文本单元格自动去重 |

## 单元格类型

| 类型 | 常量 | BIFF12 记录 |
|------|------|-------------|
| 数字 | `CXLSB_CELL_NUMBER` | CellReal (类型=4) |
| 文本 | `CXLSB_CELL_TEXT` | CellIsst (类型=7) 用SST，CellSt (类型=6) 内联 |
| 布尔 | `CXLSB_CELL_BOOL` | CellBool (类型=9) |
| 日期 | `CXLSB_CELL_DATE` | CellReal 加日期格式 |
| 空白 | `CXLSB_CELL_BLANK` | CellBlank (类型=0) |

## 构建

### 前置条件

- GCC 或兼容 C 编译器 (支持 C99)
- libzip 开发包

```bash
# Ubuntu/Debian
sudo apt-get install libzip-dev

# Fedora/RHEL
sudo dnf install libzip-devel
```

### 编译

```bash
# 编译单个测试
gcc tests/test_simple_write.c \
  src/api/*.c src/format/*.c src/container/*.c src/io/*.c src/util/*.c \
  -Iinclude -Isrc $(pkg-config --libs libzip) -o /tmp/test_simple_write

# 运行全部测试
tests/run_tests.sh
```

## 使用示例

### 写入 XLSB

```c
#include "cxlsb/xlsb_writer.h"
#include "cxlsb/cell_data.h"

cxlsb_cell_data_t supplier(int row, int col, void* data) {
    switch (col) {
        case 0: return cxlsb_cell_text("姓名");
        case 1: return cxlsb_cell_number(row * 100.0);
        case 2: return cxlsb_cell_bool(row % 2 == 0);
        default: return cxlsb_cell_blank();
    }
}

int main() {
    cxlsb_writer_t* w = cxlsb_writer_create("output.xlsb");
    
    cxlsb_error_t err = cxlsb_writer_write_batch(w, "Sheet1", supplier, NULL, 1000, 3);
    if (err != CXLSB_OK) {
        printf("错误: %s\n", cxlsb_error_string(err));
    }
    
    cxlsb_writer_destroy(w);
    return 0;
}
```

### 流式写入 (大文件)

```c
cxlsb_writer_t* w = cxlsb_writer_create("large.xlsb");

cxlsb_writer_start_sheet(w, "数据", 3);

for (int batch = 0; batch < 10; batch++) {
    cxlsb_writer_write_rows(w, batch * 10000, supplier, NULL, 10000);
}

cxlsb_writer_end_sheet(w);
cxlsb_writer_close(w);
cxlsb_writer_destroy(w);
```

### 读取 XLSB

```c
#include "cxlsb/xlsb_reader.h"

void row_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* data) {
    printf("行 %d: ", row);
    for (int i = 0; i < col_count; i++) {
        switch (cells[i].type) {
            case CXLSB_CELL_TEXT:
                printf("%s ", cells[i].value.text);
                break;
            case CXLSB_CELL_NUMBER:
                printf("%.2f ", cells[i].value.number);
                break;
            default:
                printf("(空) ");
        }
    }
    printf("\n");
}

int main() {
    cxlsb_reader_t* r = cxlsb_reader_create("data.xlsb");
    
    cxlsb_error_t err = cxlsb_reader_for_each_row(r, 0, row_handler, NULL);
    if (err != CXLSB_OK) {
        printf("错误: %s\n", cxlsb_error_string(err));
    }
    
    cxlsb_reader_destroy(r);
    return 0;
}
```

### 模板填充

```c
#include "cxlsb/template_filler.h"

cxlsb_cell_data_t data_supplier(int row, int col, void* data) {
    return cxlsb_cell_text("填充数据");
}

int main() {
    cxlsb_template_filler_t* filler = cxlsb_template_filler_create("template.xlsb");
    
    cxlsb_error_t err = cxlsb_template_filler_fill_marker(
        filler, 0, "${data}", data_supplier, NULL, 10, 5);
    
    if (err == CXLSB_OK) {
        cxlsb_template_filler_save(filler, "output.xlsb");
    } else {
        printf("错误: %s\n", cxlsb_error_string(err));
    }
    
    cxlsb_template_filler_destroy(filler);
    return 0;
}
```

## 错误处理

所有 API 函数返回 `cxlsb_error_t` 枚举:

```c
typedef enum {
    CXLSB_OK = 0,
    CXLSB_ERR_NULL_PTR,
    CXLSB_ERR_NO_MEMORY,
    CXLSB_ERR_FILE_OPEN_FAILED,
    CXLSB_ERR_INVALID_ARG,
    CXLSB_ERR_MARKER_NOT_FOUND,
    ...
} cxlsb_error_t;
```

使用 `cxlsb_error_string(err)` 获取可读错误描述。

## 项目结构

```
cxlsb/
├── include/cxlsb/       # 公共 API 头文件
│   ├── types.h          # 核心类型定义
│   ├── error_codes.h    # 错误码
│   ├── cell_data.h      # 单元格数据辅助函数
│   ├── xlsb_writer.h    # 写入器 API
│   ├── xlsb_reader.h    # 读取器 API
│   └── template_filler.h # 模板填充 API
├── src/
│   ├── api/             # API 实现
│   ├── format/          # BIFF12 格式处理
│   ├── container/       # ZIP 容器处理
│   ├── io/              # 缓冲读写
│   └── util/            # 工具函数 (varint, UTF-16)
├── tests/               # 测试套件 (12个测试)
│   └── resources/       # 测试模板
└── tools/               # 工具程序
```

## 测试覆盖

| 测试 | 描述 |
|------|------|
| test_simple_write | 基本写入操作 |
| test_reader_simple | 基本读取操作 |
| test_read_write | 读写往返测试 |
| test_stream_simple | 流式模式 |
| test_stream_multi_batch | 多批次流式 |
| test_stream_large | 大数据集流式 |
| test_pagination | 分页读取 |
| test_mixed_types | 多单元格类型 |
| test_mixed_mode | 批量 + 流式混合 |
| template_fill_marker | 标记填充 |
| template_fill_index | 索引填充 |
| test_template_filler | 模板填充 API |

12个测试全部通过。

## 性能优化

- **Supplier 优化**: 每行单次调用 (缓存)
- **动态缓冲区**: 按维度计算大小，无溢出风险
- **SST 阈值**: >3字符用SST，短字符串内联
- **流式写入**: 支持超出内存限制的数据集

## BIFF12 格式

本库实现 MS-XLSB 规范:
- Varint 编码记录类型和大小
- UTF-16LE 字符串内容
- 共享字符串表 (SST) 文本去重
- ROW_HDR 记录带 span 段

## 相关文档

- [使用手册](MANUAL.md) — 安装、构建、API 用法与示例
- [变更日志](CHANGELOG.md) — 版本历史

## 许可证

Apache License 2.0

## 致谢

移植自 [jxlsb](https://github.com/itcraft-cn/jxlsb) Java 实现。