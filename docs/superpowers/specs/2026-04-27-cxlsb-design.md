# cxlsb 设计文档

## 一、项目概述

### 1.1 项目目标

构建一个纯 C 实现的 XLSB（Excel Binary Workbook）格式读写库，具备以下特性：

- **零第三方依赖**：仅依赖系统安装的 libzip 和用户指定的 zlog
- **跨平台支持**：支持 Linux 和 Windows 平台
- **高性能**：动态缓冲区 + 流式处理，支持大文件
- **完整功能**：与 Java 版本 jxlsb 功能对齐
- **函数式 API**：纯函数式 + 函数指针回调，符合 C 传统风格

### 1.2 功能范围

**基础读写功能：**
- 单元格数据类型：文本、数值、日期、布尔、空白、错误
- 行/列操作：读取、写入、批量操作
- 多 Sheet 支持：Workbook 包含多个 Sheet
- 流式 API：write_batch、write_rows、read_rows、for_each_row

**高级功能：**
- 模板填充：fill_batch、fill_at_marker、start_fill/fill_rows/end_fill
- 样式支持：字体、边框、填充、对齐、数字格式
- 合并单元格：读取和写入合并单元格
- 共享字符串表（SST）：文本优化存储

### 1.3 技术约束

- 包名：`cxlsb`
- C 标准：ANSI C (C99)
- 构建：CMake 3.x
- 日志：zlog（用户指定路径 `/home/helly/open_source/github/zlog`）
- ZIP 处理：libzip（系统安装）
- 错误处理：返回值 + 错误码枚举

---

## 二、整体架构

### 2.1 架构分层

```
┌──────────────────────────────────────────────────────────────┐
│                      API Layer                                │
│  xlsb_writer_create/write_batch/write_rows/close             │
│  xlsb_reader_create/read_rows/for_each_row/close             │
├──────────────────────────────────────────────────────────────┤
│                    Container Layer                            │
│  xlsb_container_create/add_entry/close                       │
│  xlsb_container_reader_open/get_entry/close                  │
│  [依赖: libzip]                                               │
├──────────────────────────────────────────────────────────────┤
│                     Format Layer                              │
│  biff12_writer/write_record/write_cell                       │
│  biff12_reader/parse_record/parse_cell                       │
│  sst_table/add_string/get_string                             │
│  styles_registry/add_style/get_xf_id                         │
│  sheet_writer/write_sheet_data                               │
│  sheet_reader/read_sheet_data                                │
│  workbook_writer/serialize                                    │
├──────────────────────────────────────────────────────────────┤
│                       IO Layer                                │
│  buffer_writer_create/write_int/write_bytes/flush            │
│  buffer_reader_create/read_int/read_bytes                    │
├──────────────────────────────────────────────────────────────┤
│                      Util Layer                               │
│  varint_encode/varint_decode                                 │
│  utf16_encode/utf16_decode                                   │
│  xml_gen_theme/xml_gen_rels                                  │
│  [依赖: zlog]                                                 │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 核心设计原则

1. **动态缓冲区优先**：小文件使用动态扩展缓冲区，大文件（>=10w条）使用固定大小缓冲区
2. **流式处理**：避免全量加载到内存，支持 GB 级文件
3. **函数式 API**：纯函数 + 函数指针回调，不使用 OO 模拟
4. **返回值错误处理**：0 成功，负数错误码失败
5. **跨平台兼容**：使用标准 C 和 POSIX API

---

## 三、目录结构

```
cxlsb/
├── CMakeLists.txt                    # CMake 构建配置
├── README.md                          # 项目说明
├── LICENSE                            # Apache 2.0
├── include/
│   └── cxlsb/
│       ├── xlsb.h                     # 主入口头文件
│       ├── xlsb_writer.h              # 写入器 API
│       ├── xlsb_reader.h              # 读取器 API
│       ├── cell_data.h                # 单元格数据结构
│       ├── error_codes.h              # 错误码定义
│       └── types.h                    # 公共类型定义
├── src/
│   ├── api/
│   │   ├── xlsb_writer.c              # 写入器实现
│   │   └── xlsb_reader.c              # 读取器实现
│   ├── container/
│   │   ├── xlsb_container.c           # ZIP 容器（libzip）
│   │   └── xlsb_container_reader.c    # ZIP 容器读取
│   ├── format/
│   │   ├── biff12_writer.c            # BIFF12 写入器
│   │   ├── biff12_reader.c            # BIFF12 读取器
│   │   ├── biff12_record_types.h      # 记录类型常量
│   │   ├── sst_table.c                # 共享字符串表
│   │   ├── styles_registry.c          # 样式注册表
│   │   ├── sheet_writer.c             # Sheet 写入器
│   │   ├── sheet_reader.c             # Sheet 读取器
│   │   └── workbook_writer.c          # Workbook 写入器
│   ├── io/
│   │   ├── buffer_writer.c            # 动态缓冲区写入
│   │   └── buffer_reader.c            # 动态缓冲区读取
│   └── util/
│       ├── varint.c                    # VarInt 编解码
│       ├── utf16.c                     # UTF-16LE 处理
│       ├── xml_gen.c                   # XML 生成（theme等）
│       └── rels_gen.c                  # 关系文件生成
├── test/
│   ├── test_writer.c                   # 写入测试
│   ├── test_reader.c                   # 读取测试
│   ├── test_sst.c                      # SST 测试
│   ├── test_biff12.c                   # BIFF12 测试
│   └── test_template.c                 # 模板填充测试
└── examples/
    ├── example_write.c                 # 写入示例
    ├── example_read.c                  # 读取示例
    └── example_template.c              # 模板示例
```

---

## 四、核心数据结构

### 4.1 公共类型 (`types.h`)

```c
/* 单元格类型枚举 */
typedef enum cxlsb_cell_type {
    CXLSB_CELL_BLANK = 0,
    CXLSB_CELL_NUMBER = 1,
    CXLSB_CELL_TEXT = 2,
    CXLSB_CELL_BOOL = 3,
    CXLSB_CELL_DATE = 4,
    CXLSB_CELL_ERROR = 5
} cxlsb_cell_type_t;

/* 单元格数据结构 */
typedef struct cxlsb_cell_data {
    cxlsb_cell_type_t type;
    union {
        double number;          /* 数值/日期 */
        const char* text;       /* 文本 */
        bool boolean;           /* 布尔值 */
        int error_code;         /* 错误码 */
    } value;
} cxlsb_cell_data_t;

/* 单元格数据供应回调（写入时使用） */
typedef cxlsb_cell_data_t (*cxlsb_cell_supplier_fn)(int row, int col, void* user_data);

/* 行数据处理回调（读取时使用） */
typedef void (*cxlsb_row_handler_fn)(int row, int col_count, 
                                      cxlsb_cell_data_t* cells, void* user_data);

/* Sheet 信息 */
typedef struct cxlsb_sheet_info {
    int index;
    char* name;
    int row_count;
    int col_count;
} cxlsb_sheet_info_t;
```

### 4.2 错误码定义 (`error_codes.h`)

```c
typedef enum cxlsb_error {
    CXLSB_OK = 0,
    
    /* 通用错误 */
    CXLSB_ERR_NULL_PTR = -1,
    CXLSB_ERR_NO_MEMORY = -2,
    CXLSB_ERR_INVALID_ARG = -3,
    CXLSB_ERR_BUFFER_OVERFLOW = -4,
    
    /* 文件错误 */
    CXLSB_ERR_FILE_NOT_FOUND = -10,
    CXLSB_ERR_FILE_OPEN_FAILED = -11,
    CXLSB_ERR_FILE_WRITE_FAILED = -12,
    
    /* ZIP 容器错误 */
    CXLSB_ERR_ZIP_OPEN_FAILED = -20,
    CXLSB_ERR_ZIP_ADD_ENTRY_FAILED = -21,
    
    /* 格式错误 */
    CXLSB_ERR_INVALID_RECORD = -30,
    CXLSB_ERR_INVALID_CELL_TYPE = -31,
    CXLSB_ERR_INVALID_SHEET_INDEX = -32,
    
    /* 状态错误 */
    CXLSB_ERR_NOT_IN_TEMPLATE_MODE = -40,
    CXLSB_ERR_IN_TEMPLATE_MODE = -41,
    CXLSB_ERR_SHEET_NOT_STARTED = -42,
    
    /* 模板错误 */
    CXLSB_ERR_MARKER_NOT_FOUND = -50
} cxlsb_error_t;

const char* cxlsb_error_string(cxlsb_error_t error);
```

---

## 五、API层详细设计

### 5.1 写入器 API (`xlsb_writer.h`)

**创建与销毁：**
```c
cxlsb_writer_t* cxlsb_writer_create(const char* path);
cxlsb_writer_t* cxlsb_writer_create_with_template(const char* template_path, 
                                                   const char* output_path);
void cxlsb_writer_destroy(cxlsb_writer_t* writer);
```

**批量写入：**
```c
cxlsb_error_t cxlsb_writer_write_batch(cxlsb_writer_t* writer,
                                        const char* sheet_name,
                                        cxlsb_cell_supplier_fn supplier,
                                        void* user_data,
                                        int row_count,
                                        int col_count);
```

**流式写入：**
```c
cxlsb_error_t cxlsb_writer_start_sheet(cxlsb_writer_t* writer,
                                        const char* sheet_name,
                                        int col_count);
cxlsb_error_t cxlsb_writer_write_rows(cxlsb_writer_t* writer,
                                       int start_row,
                                       cxlsb_cell_supplier_fn supplier,
                                       void* user_data,
                                       int row_count);
cxlsb_error_t cxlsb_writer_end_sheet(cxlsb_writer_t* writer);
```

**模板填充：**
```c
cxlsb_error_t cxlsb_writer_fill_batch(cxlsb_writer_t* writer,
                                       int sheet_index,
                                       cxlsb_cell_data_t** data_list,
                                       int data_row_count,
                                       int data_col_count,
                                       int start_row,
                                       int start_col);
cxlsb_error_t cxlsb_writer_fill_at_marker(cxlsb_writer_t* writer,
                                           int sheet_index,
                                           const char* marker,
                                           cxlsb_cell_data_t** data_list,
                                           int data_row_count,
                                           int data_col_count);
cxlsb_error_t cxlsb_writer_start_fill(cxlsb_writer_t* writer,
                                       int sheet_index,
                                       int start_row,
                                       int start_col);
cxlsb_error_t cxlsb_writer_fill_rows(cxlsb_writer_t* writer,
                                      cxlsb_cell_data_t* cells,
                                      int col_count);
cxlsb_error_t cxlsb_writer_end_fill(cxlsb_writer_t* writer);
```

### 5.2 读取器 API (`xlsb_reader.h`)

**创建与销毁：**
```c
cxlsb_reader_t* cxlsb_reader_create(const char* path);
void cxlsb_reader_destroy(cxlsb_reader_t* reader);
```

**Sheet 信息：**
```c
int cxlsb_reader_get_sheet_count(cxlsb_reader_t* reader);
cxlsb_error_t cxlsb_reader_get_sheet_info(cxlsb_reader_t* reader,
                                           int sheet_index,
                                           cxlsb_sheet_info_t* info);
```

**流式读取：**
```c
cxlsb_error_t cxlsb_reader_for_each_row(cxlsb_reader_t* reader,
                                         int sheet_index,
                                         cxlsb_row_handler_fn handler,
                                         void* user_data);
cxlsb_error_t cxlsb_reader_for_each_cell(cxlsb_reader_t* reader,
                                          int sheet_index,
                                          cxlsb_cell_handler_fn handler,
                                          void* user_data);
```

**分页读取：**
```c
int cxlsb_reader_read_rows(cxlsb_reader_t* reader,
                            int sheet_index,
                            int start_row,
                            int row_count,
                            cxlsb_cell_data_t** out_cells,
                            int* out_col_count);
```

---

## 六、IO层详细设计

### 6.1 动态缓冲区写入器 (`buffer_writer.h`)

```c
typedef struct cxlsb_buffer_writer {
    uint8_t* buffer;            /* 数据缓冲区 */
    size_t capacity;            /* 总容量 */
    size_t position;            /* 当前写入位置 */
    size_t initial_capacity;    /* 初始容量 */
} cxlsb_buffer_writer_t;

cxlsb_buffer_writer_t* cxlsb_buffer_writer_create(size_t initial_capacity);
void cxlsb_buffer_writer_destroy(cxlsb_buffer_writer_t* writer);

int cxlsb_buffer_writer_write_byte(cxlsb_buffer_writer_t* writer, uint8_t value);
int cxlsb_buffer_writer_write_short(cxlsb_buffer_writer_t* writer, uint16_t value);
int cxlsb_buffer_writer_write_int(cxlsb_buffer_writer_t* writer, uint32_t value);
int cxlsb_buffer_writer_write_long(cxlsb_buffer_writer_t* writer, uint64_t value);
int cxlsb_buffer_writer_write_double(cxlsb_buffer_writer_t* writer, double value);
int cxlsb_buffer_writer_write_bytes(cxlsb_buffer_writer_t* writer, 
                                     const uint8_t* data, size_t length);

size_t cxlsb_buffer_writer_get_position(cxlsb_buffer_writer_t* writer);
uint8_t* cxlsb_buffer_writer_get_buffer(cxlsb_buffer_writer_t* writer);
size_t cxlsb_buffer_writer_get_length(cxlsb_buffer_writer_t* writer);
int cxlsb_buffer_writer_ensure_capacity(cxlsb_buffer_writer_t* writer, size_t required);
```

**内存管理策略：**
- 初始容量：4KB
- 扩展策略：容量翻倍（realloc）
- 大文件（>=10w条）：预分配 16MB 固定缓冲区

### 6.2 动态缓冲区读取器 (`buffer_reader.h`)

```c
typedef struct cxlsb_buffer_reader {
    const uint8_t* buffer;      /* 数据缓冲区 */
    size_t length;              /* 数据长度 */
    size_t position;            /* 当前读取位置 */
} cxlsb_buffer_reader_t;

cxlsb_buffer_reader_t* cxlsb_buffer_reader_create(const uint8_t* buffer, size_t length);
void cxlsb_buffer_reader_destroy(cxlsb_buffer_reader_t* reader);

uint8_t cxlsb_buffer_reader_read_byte(cxlsb_buffer_reader_t* reader);
uint16_t cxlsb_buffer_reader_read_short(cxlsb_buffer_reader_t* reader);
uint32_t cxlsb_buffer_reader_read_int(cxlsb_buffer_reader_t* reader);
uint64_t cxlsb_buffer_reader_read_long(cxlsb_buffer_reader_t* reader);
double cxlsb_buffer_reader_read_double(cxlsb_buffer_reader_t* reader);

size_t cxlsb_buffer_reader_get_position(cxlsb_buffer_reader_t* reader);
size_t cxlsb_buffer_reader_get_remaining(cxlsb_buffer_reader_t* reader);
bool cxlsb_buffer_reader_has_remaining(cxlsb_buffer_reader_t* reader);
```

---

## 七、Format层详细设计

### 7.1 BIFF12 记录类型常量 (`biff12_record_types.h`)

```c
/* Cell records (0-11) */
#define BIFF12_BRT_ROW_HDR           0
#define BIFF12_BRT_CELL_BLANK        1
#define BIFF12_BRT_CELL_RK           2
#define BIFF12_BRT_CELL_ERROR        3
#define BIFF12_BRT_CELL_BOOL         4
#define BIFF12_BRT_CELL_REAL         5
#define BIFF12_BRT_CELL_ST           6
#define BIFF12_BRT_CELL_ISST         7

/* SST */
#define BIFF12_BRT_SST_ITEM          19

/* Workbook records (128+) */
#define BIFF12_BRT_BEGIN_SHEET       129
#define BIFF12_BRT_END_SHEET         130
#define BIFF12_BRT_BEGIN_BOOK        131
#define BIFF12_BRT_END_BOOK          132
#define BIFF12_BRT_BEGIN_SHEET_DATA  145
#define BIFF12_BRT_END_SHEET_DATA    146
#define BIFF12_BRT_WS_DIM            148
#define BIFF12_BRT_BEGIN_SST         159
#define BIFF12_BRT_END_SST           160

/* Styles */
#define BIFF12_BRT_BEGIN_STYLE_SHEET 370
#define BIFF12_BRT_END_STYLE_SHEET   371

/* Merge cells */
#define BIFF12_BRT_MERGE_CELL        176
```

### 7.2 BIFF12 写入器 (`biff12_writer.h`)

```c
typedef struct cxlsb_biff12_writer {
    cxlsb_buffer_writer_t* buffer;
} cxlsb_biff12_writer_t;

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create(size_t initial_capacity);
void cxlsb_biff12_writer_destroy(cxlsb_biff12_writer_t* writer);

int cxlsb_biff12_writer_write_record_header(cxlsb_biff12_writer_t* writer,
                                             uint32_t record_type,
                                             uint32_t record_size);
int cxlsb_biff12_writer_write_empty_record(cxlsb_biff12_writer_t* writer,
                                            uint32_t record_type);
int cxlsb_biff12_writer_write_varint(cxlsb_biff12_writer_t* writer, uint32_t value);
int cxlsb_biff12_writer_write_wide_string(cxlsb_biff12_writer_t* writer,
                                           const char* str);

int cxlsb_biff12_writer_write_cell_real(cxlsb_biff12_writer_t* writer,
                                         uint32_t row, uint32_t col,
                                         double value, uint32_t xf_id);
int cxlsb_biff12_writer_write_cell_rk(cxlsb_biff12_writer_t* writer,
                                       uint32_t row, uint32_t col,
                                       double value, uint32_t xf_id);
int cxlsb_biff12_writer_write_cell_st(cxlsb_biff12_writer_t* writer,
                                       uint32_t row, uint32_t col,
                                       const char* str, uint32_t xf_id);
int cxlsb_biff12_writer_write_cell_isst(cxlsb_biff12_writer_t* writer,
                                         uint32_t row, uint32_t col,
                                         uint32_t sst_index, uint32_t xf_id);
int cxlsb_biff12_writer_write_cell_bool(cxlsb_biff12_writer_t* writer,
                                         uint32_t row, uint32_t col,
                                         bool value, uint32_t xf_id);
int cxlsb_biff12_writer_write_cell_blank(cxlsb_biff12_writer_t* writer,
                                          uint32_t row, uint32_t col,
                                          uint32_t xf_id);
int cxlsb_biff12_writer_write_row_header(cxlsb_biff12_writer_t* writer,
                                          uint32_t row, uint32_t first_col,
                                          uint32_t last_col);
```

### 7.3 共享字符串表 (`sst_table.h`)

```c
typedef struct cxlsb_sst_table {
    char** strings;             /* 字符串数组 */
    uint32_t* hash_table;       /* 哈希表用于快速查找 */
    uint32_t capacity;          /* 数组容量 */
    uint32_t count;             /* 字符串数量 */
    uint32_t total_count;       /* 总引用次数 */
} cxlsb_sst_table_t;

cxlsb_sst_table_t* cxlsb_sst_table_create(uint32_t initial_capacity);
void cxlsb_sst_table_destroy(cxlsb_sst_table_t* table);

uint32_t cxlsb_sst_table_add_string(cxlsb_sst_table_t* table, const char* str);
int cxlsb_sst_table_find_string(cxlsb_sst_table_t* table, const char* str);
const char* cxlsb_sst_table_get_string(cxlsb_sst_table_t* table, uint32_t index);

int cxlsb_sst_table_serialize(cxlsb_sst_table_t* table,
                               cxlsb_biff12_writer_t* writer);
```

### 7.4 样式注册表 (`styles_registry.h`)

```c
typedef struct cxlsb_font {
    char* name;
    double size;
    bool bold;
    bool italic;
    uint32_t color;
} cxlsb_font_t;

typedef struct cxlsb_fill {
    uint32_t pattern_type;
    uint32_t fg_color;
    uint32_t bg_color;
} cxlsb_fill_t;

typedef struct cxlsb_border {
    uint32_t left_style;
    uint32_t right_style;
    uint32_t top_style;
    uint32_t bottom_style;
} cxlsb_border_t;

typedef struct cxlsb_xf {
    uint32_t font_id;
    uint32_t fill_id;
    uint32_t border_id;
    uint32_t num_format_id;
    uint8_t horizontal_align;
    uint8_t vertical_align;
} cxlsb_xf_t;

typedef struct cxlsb_styles_registry {
    char** num_formats;
    cxlsb_font_t* fonts;
    cxlsb_fill_t* fills;
    cxlsb_border_t* borders;
    cxlsb_xf_t* xfs;
    uint32_t num_format_count;
    uint32_t font_count;
    uint32_t fill_count;
    uint32_t border_count;
    uint32_t xf_count;
} cxlsb_styles_registry_t;

cxlsb_styles_registry_t* cxlsb_styles_registry_create(void);
void cxlsb_styles_registry_destroy(cxlsb_styles_registry_t* registry);

uint32_t cxlsb_styles_registry_add_font(cxlsb_styles_registry_t* registry,
                                         const cxlsb_font_t* font);
uint32_t cxlsb_styles_registry_add_xf(cxlsb_styles_registry_t* registry,
                                       const cxlsb_xf_t* xf);
int cxlsb_styles_registry_serialize(cxlsb_styles_registry_t* registry,
                                     cxlsb_biff12_writer_t* writer);
```

---

## 八、Container层详细设计

### 8.1 ZIP 容器写入器 (`xlsb_container.h`)

```c
typedef struct cxlsb_container cxlsb_container_t;

cxlsb_container_t* cxlsb_container_create(const char* path);
void cxlsb_container_destroy(cxlsb_container_t* container);

int cxlsb_container_add_entry(cxlsb_container_t* container,
                               const char* name,
                               const uint8_t* data,
                               size_t length);
int cxlsb_container_close(cxlsb_container_t* container);
```

**实现细节：**
- 使用 libzip 库
- 缓冲区大小：256KB（减少磁盘 IO）
- 条目添加顺序：
  1. `[Content_Types].xml`
  2. `_rels/.rels`
  3. `docProps/app.xml`
  4. `docProps/core.xml`
  5. `xl/workbook.bin`
  6. `xl/styles.bin`
  7. `xl/theme/theme1.xml`
  8. `xl/sharedStrings.bin`（如有）
  9. `xl/worksheets/sheetN.bin`
  10. `xl/_rels/workbook.bin.rels`

### 8.2 ZIP 容器读取器 (`xlsb_container_reader.h`)

```c
typedef struct cxlsb_container_reader cxlsb_container_reader_t;

cxlsb_container_reader_t* cxlsb_container_reader_create(const char* path);
void cxlsb_container_reader_destroy(cxlsb_container_reader_t* reader);

bool cxlsb_container_reader_has_entry(cxlsb_container_reader_t* reader,
                                       const char* name);
int cxlsb_container_reader_get_entry(cxlsb_container_reader_t* reader,
                                      const char* name,
                                      uint8_t** out_data,
                                      size_t* out_length);
cxlsb_buffer_reader_t* cxlsb_container_reader_get_sheet_stream(
    cxlsb_container_reader_t* reader, int sheet_index);
cxlsb_buffer_reader_t* cxlsb_container_reader_get_sst_stream(
    cxlsb_container_reader_t* reader);
bool cxlsb_container_reader_has_sst(cxlsb_container_reader_t* reader);
```

---

## 九、Util层详细设计

### 9.1 VarInt 编解码 (`varint.h`)

```c
int cxlsb_varint_encode(uint32_t value, uint8_t* out);
int cxlsb_varint_decode(const uint8_t* buffer, size_t offset, uint32_t* out_value);
int cxlsb_varint_size(uint32_t value);
int cxlsb_varsize_encode(uint32_t value, uint8_t* out);
int cxlsb_varsize_decode(const uint8_t* buffer, size_t offset, uint32_t* out_value);
```

**VarInt 编码规则：**
- 每字节最高位表示是否继续
- 低 7 位存储数据
- 小端序编码

### 9.2 UTF-16LE 处理 (`utf16.h`)

```c
int cxlsb_utf8_to_utf16le(const char* utf8, uint8_t* out_utf16, size_t* out_length);
char* cxlsb_utf16le_to_utf8(const uint8_t* utf16, size_t length);
size_t cxlsb_utf16le_length(const char* utf8);
```

### 9.3 XML 生成 (`xml_gen.h`)

```c
int cxlsb_xml_gen_theme(uint8_t** out, size_t* out_length);
int cxlsb_xml_gen_app(int sheet_count, uint8_t** out, size_t* out_length);
int cxlsb_xml_gen_core(uint8_t** out, size_t* out_length);
int cxlsb_xml_gen_content_types(int sheet_count, bool has_sst,
                                 uint8_t** out, size_t* out_length);
```

### 9.4 关系文件生成 (`rels_gen.h`)

```c
int cxlsb_rels_gen_root(uint8_t** out, size_t* out_length);
int cxlsb_rels_gen_workbook(int sheet_count, bool has_sst,
                             uint8_t** out, size_t* out_length);
```

---

## 十、Sheet写入器与Workbook结构

### 10.1 Sheet 写入器 (`sheet_writer.h`)

```c
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

int cxlsb_sheet_writer_write_sheet(cxlsb_sheet_writer_t* writer,
                                    cxlsb_cell_supplier_fn supplier,
                                    void* user_data,
                                    int row_count,
                                    int col_count,
                                    uint8_t** out_data,
                                    size_t* out_length);

int cxlsb_sheet_writer_start_streaming(cxlsb_sheet_writer_t* writer, int col_count);
int cxlsb_sheet_writer_append_rows(cxlsb_sheet_writer_t* writer,
                                    cxlsb_cell_supplier_fn supplier,
                                    void* user_data,
                                    int start_row,
                                    int row_count);
int cxlsb_sheet_writer_finalize_streaming(cxlsb_sheet_writer_t* writer,
                                           int row_count,
                                           int col_count,
                                           uint8_t** out_data,
                                           size_t* out_length);
```

### 10.2 Workbook 写入器 (`workbook_writer.h`)

```c
typedef struct cxlsb_workbook_writer {
    char** sheet_names;
    uint32_t sheet_count;
    uint32_t capacity;
} cxlsb_workbook_writer_t;

cxlsb_workbook_writer_t* cxlsb_workbook_writer_create(void);
void cxlsb_workbook_writer_destroy(cxlsb_workbook_writer_t* writer);

int cxlsb_workbook_writer_add_sheet(cxlsb_workbook_writer_t* writer,
                                     const char* name);
int cxlsb_workbook_writer_serialize(cxlsb_workbook_writer_t* writer,
                                     uint8_t** out_data,
                                     size_t* out_length);
```

---

## 十一、单元格数据辅助函数

```c
/* 创建空白单元格 */
cxlsb_cell_data_t cxlsb_cell_blank(void);

/* 创建数值单元格 */
cxlsb_cell_data_t cxlsb_cell_number(double value);

/* 创建文本单元格 */
cxlsb_cell_data_t cxlsb_cell_text(const char* text);

/* 创建布尔单元格 */
cxlsb_cell_data_t cxlsb_cell_bool(bool value);

/* 创建日期单元格 */
cxlsb_cell_data_t cxlsb_cell_date(double excel_date);

/* 创建带百分号格式的数值 */
cxlsb_cell_data_t cxlsb_cell_percentage(double value);

/* 创建带逗号分隔的数值 */
cxlsb_cell_data_t cxlsb_cell_number_comma(double value);
```

---

## 十二、使用示例

### 12.1 批量写入示例

```c
#include "cxlsb/xlsb.h"

/* 单元格数据供应回调 */
cxlsb_cell_data_t cell_supplier(int row, int col, void* user_data) {
    if (col == 0) {
        return cxlsb_cell_text("Product");
    } else if (col == 1) {
        return cxlsb_cell_number(row * 100.0);
    }
    return cxlsb_cell_blank();
}

int main() {
    /* 创建写入器 */
    cxlsb_writer_t* writer = cxlsb_writer_create("output.xlsb");
    if (writer == NULL) {
        fprintf(stderr, "Failed to create writer\n");
        return -1;
    }
    
    /* 批量写入 */
    cxlsb_error_t err = cxlsb_writer_write_batch(writer, "Sheet1",
                                                  cell_supplier, NULL,
                                                  1000, 2);
    if (err != CXLSB_OK) {
        fprintf(stderr, "Error: %s\n", cxlsb_error_string(err));
        cxlsb_writer_destroy(writer);
        return -1;
    }
    
    /* 销毁写入器 */
    cxlsb_writer_destroy(writer);
    return 0;
}
```

### 12.2 流式写入示例

```c
int main() {
    cxlsb_writer_t* writer = cxlsb_writer_create("large_output.xlsb");
    
    /* 开始 Sheet */
    cxlsb_writer_start_sheet(writer, "Data", 4);
    
    /* 流式写入多批数据 */
    int offset = 0;
    while (has_more_data()) {
        cxlsb_writer_write_rows(writer, offset, row_supplier, NULL, 1000);
        offset += 1000;
    }
    
    /* 结束 Sheet */
    cxlsb_writer_end_sheet(writer);
    
    cxlsb_writer_destroy(writer);
    return 0;
}
```

### 12.3 模板填充示例

```c
int main() {
    /* 创建模板模式写入器 */
    cxlsb_writer_t* writer = cxlsb_writer_create_with_template(
        "template.xlsb", "output.xlsb");
    
    /* 准备数据 */
    cxlsb_cell_data_t** data = prepare_data(100, 5);
    
    /* 在标记位置填充 */
    cxlsb_error_t err = cxlsb_writer_fill_at_marker(writer, 0, "${data}",
                                                     data, 100, 5);
    
    cxlsb_writer_destroy(writer);
    return 0;
}
```

### 12.4 读取示例

```c
/* 行处理回调 */
void row_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* user_data) {
    for (int i = 0; i < col_count; i++) {
        if (cells[i].type == CXLSB_CELL_TEXT) {
            printf("Row %d, Col %d: %s\n", row, i, cells[i].value.text);
        }
    }
}

int main() {
    cxlsb_reader_t* reader = cxlsb_reader_create("data.xlsb");
    
    /* 流式读取 */
    cxlsb_reader_for_each_row(reader, 0, row_handler, NULL);
    
    cxlsb_reader_destroy(reader);
    return 0;
}
```

---

## 十三、CMake 配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(cxlsb VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找依赖
find_package(ZLIB REQUIRED)
find_library(LIBZIP_LIBRARY zip)
find_path(LIBZIP_INCLUDE_DIR zip.h)

# zlog 路径
set(ZLOG_INCLUDE_DIR "/home/helly/open_source/github/zlog/src")
set(ZLOG_LIBRARY "/home/helly/open_source/github/zlog/build/libzlog.so")

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${LIBZIP_INCLUDE_DIR}
    ${ZLOG_INCLUDE_DIR}
)

# 源文件
file(GLOB_RECURSE SOURCES "src/*.c")

# 构建库
add_library(cxlsb SHARED ${SOURCES})
target_link_libraries(cxlsb ${LIBZIP_LIBRARY} ${ZLOG_LIBRARY} ${ZLIB_LIBRARIES})

# 测试
enable_testing()
file(GLOB TEST_SOURCES "test/*.c")
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} cxlsb)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
```

---

## 十四、性能目标

| 指标 | 目标值 |
|------|--------|
| 读取速度 | >50MB/s |
| 写入速度 | >30MB/s |
| 内存占用（小文件） | 动态扩展 |
| 内存占用（大文件） | 固定16MB缓冲区 |
| SST 优化 | 文本复用，减少50%存储 |

---

## 十五、开发计划

| Phase | 内容 | 预估工作量 |
|-------|------|------------|
| Phase 1 | 项目骨架 + CMake + 目录结构 | 0.5h |
| Phase 2 | IO层：buffer_writer, buffer_reader | 1h |
| Phase 3 | Util层：varint, utf16, xml_gen, rels_gen | 1h |
| Phase 4 | Format层：biff12_writer, sst_table | 2h |
| Phase 5 | Container层：xlsb_container | 1h |
| Phase 6 | Sheet/Workbook写入器 | 2h |
| Phase 7 | API层：xlsb_writer | 1h |
| Phase 8 | 读取器：biff12_reader, sheet_reader, xlsb_reader | 2h |
| Phase 9 | 模板填充功能 | 1h |
| Phase 10 | 测试 + 示例 | 1h |

**总计预估：约 12.5 小时**

---

**设计完成，待用户审批后进入 writing-plans 阶段生成详细实施计划。**