**中文** | [English](MANUAL_en.md)

# cxlsb 使用手册

> 本手册面向使用 cxlsb 的开发者，覆盖环境要求、构建、API 用法与示例。
> 特性概览见 [README.md](README.md)，版本历史见 [CHANGELOG.md](CHANGELOG.md)。

## 1. 简介

cxlsb 是纯 ANSI C（C99）实现的 XLSB（Excel 二进制工作簿）读写库，移植自 Java jxlsb。设计出发点为高性能数据输出（写入）；读取最初用于读模板，读能力完备后自然延伸出高性能读。采用过程式 + 函数指针风格，只依赖 libzip，嵌入式友好。

## 2. 环境要求

| 项 | 要求 |
|----|------|
| 编译器 | GCC 或兼容的 C 编译器（支持 C99） |
| 依赖 | libzip（ZIP 容器）、zlib；可选 zlog |

安装依赖：

```bash
# Ubuntu/Debian
sudo apt-get install libzip-dev
# Fedora/RHEL
sudo dnf install libzip-devel
```

## 3. 构建与测试

```bash
# 编译单个测试
gcc tests/test_simple_write.c \
  src/api/*.c src/format/*.c src/container/*.c src/io/*.c src/util/*.c \
  -Iinclude -Isrc $(pkg-config --libs libzip) -o /tmp/test_simple_write

# 运行全部测试
tests/run_tests.sh
```

CMake 构建见仓库根目录 `CMakeLists.txt`。

## 4. API 用法

### 4.1 写入

```c
#include "cxlsb/xlsb_writer.h"
#include "cxlsb/cell_data.h"

cxlsb_cell_data_t supplier(int row, int col, void* data) {
    switch (col) {
        case 0: return cxlsb_cell_text("Name");
        case 1: return cxlsb_cell_number(row * 100.0);
        case 2: return cxlsb_cell_bool(row % 2 == 0);
        default: return cxlsb_cell_blank();
    }
}

int main() {
    cxlsb_writer_t* w = cxlsb_writer_create("output.xlsb");

    cxlsb_error_t err = cxlsb_writer_write_batch(w, "Sheet1", supplier, NULL, 1000, 3);
    if (err != CXLSB_OK) {
        printf("Error: %s\n", cxlsb_error_string(err));
    }

    cxlsb_writer_destroy(w);
    return 0;
}
```

### 4.2 流式写入（大文件）

```c
cxlsb_writer_t* w = cxlsb_writer_create("large.xlsb");
cxlsb_writer_start_sheet(w, "Data", 3);

for (int batch = 0; batch < 10; batch++) {
    cxlsb_writer_write_rows(w, batch * 10000, supplier, NULL, 10000);
}

cxlsb_writer_end_sheet(w);
cxlsb_writer_close(w);
cxlsb_writer_destroy(w);
```

### 4.3 读取

```c
#include "cxlsb/xlsb_reader.h"

void row_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* data) {
    printf("Row %d: ", row);
    for (int i = 0; i < col_count; i++) {
        switch (cells[i].type) {
            case CXLSB_CELL_TEXT:   printf("%s ", cells[i].value.text); break;
            case CXLSB_CELL_NUMBER: printf("%.2f ", cells[i].value.number); break;
            default:                printf("(blank) ");
        }
    }
    printf("\n");
}

int main() {
    cxlsb_reader_t* r = cxlsb_reader_create("data.xlsb");
    cxlsb_error_t err = cxlsb_reader_for_each_row(r, 0, row_handler, NULL);
    if (err != CXLSB_OK) {
        printf("Error: %s\n", cxlsb_error_string(err));
    }
    cxlsb_reader_destroy(r);
    return 0;
}
```

### 4.4 模板填充

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
        printf("Error: %s\n", cxlsb_error_string(err));
    }

    cxlsb_template_filler_destroy(filler);
    return 0;
}
```

## 5. 支持的操作

| 功能 | API | 描述 |
|------|-----|------|
| 批量写入 | `cxlsb_writer_write_batch` | 单次调用写入整张工作表 |
| 流式写入 | `cxlsb_writer_start_sheet` + `write_rows` + `end_sheet` | 逐批追加行 |
| 行迭代 | `cxlsb_reader_for_each_row` | 回调处理每行数据 |
| 模板填充（标记） | `cxlsb_template_filler_fill_marker` | 在 `${data}` 标记位置填充 |
| 模板填充（索引） | `cxlsb_template_filler_fill_index` | 在指定行列位置填充 |
| SST 管理 | 共享字符串表 | 文本单元格自动去重 |

## 6. 单元格类型

| 类型 | 常量 | BIFF12 记录 |
|------|------|-------------|
| 数字 | `CXLSB_CELL_NUMBER` | CellReal（type=4） |
| 文本 | `CXLSB_CELL_TEXT` | CellIsst（type=7，SST）/ CellSt（type=6，内联） |
| 布尔 | `CXLSB_CELL_BOOL` | CellBool（type=9） |
| 日期 | `CXLSB_CELL_DATE` | 带日期格式的 CellReal |
| 空白 | `CXLSB_CELL_BLANK` | CellBlank（type=0） |

## 7. 错误处理

所有 API 返回 `cxlsb_error_t` 枚举，成功为 `CXLSB_OK`(0)。使用 `cxlsb_error_string(err)` 获取可读描述。

## 8. 内存管理

- 采用 create/destroy 模式
- 所有分配均检查 NULL
- 建议使用 valgrind 检测内存泄漏

## 9. 更多文档

- [README.md](README.md) — 特性概览
- [CHANGELOG.md](CHANGELOG.md) — 版本历史
- `docs/row_column_header_visibility.md` — 行列表头可见性
- `tests/` — 测试套件
