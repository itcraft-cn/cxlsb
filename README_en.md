# cxlsb - ANSI C XLSB Library

[中文](README.md) | **English**

A pure ANSI C implementation of XLSB (Excel Binary Workbook) format reader/writer library, ported from the Java [jxlsb](https://github.com/itcraft-cn/jxlsb) project.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C Standard](https://img.shields.io/badge/C-C99-green.svg)](https://en.wikipedia.org/wiki/C99)

## Design Positioning

This library has a fixed starting point: **high-performance data output (writing)**. Reading was originally meant only for consuming templates. But once reading was in place, high-performance reading became a natural extension of the same core.

Viewed from this narrow, output-oriented angle, XLSX is simply not needed — XLSB is more than capable.

## Features

- **Zero Dependencies**: Only requires libzip for ZIP container handling
- **Cross-Platform**: Linux and Windows support
- **High Performance**: 
  - Row-level caching to avoid redundant supplier calls
  - Dynamic buffer sizing based on data dimensions
  - Streaming mode for large datasets (>1M rows)
- **Full Functionality**: Feature-aligned with Java jxlsb implementation
- **Functional API**: Pure functions with callback pointers for flexibility
- **Memory Safe**: Complete malloc NULL checks, create/destroy pattern
- **Row/Column Limit Policy**: 16384 columns × 1048576 rows, `Error` or `Cut`
- **Configurable Compression**: ZIP level 0-9, default 1 (speed first)

## Supported Operations

| Feature | API | Description |
|---------|-----|-------------|
| Batch Write | `cxlsb_writer_write_batch` | Write entire sheet in one call |
| Streaming Write | `cxlsb_writer_start_sheet` + `write_rows` + `end_sheet` | Append rows incrementally |
| Row Iteration | `cxlsb_reader_for_each_row` | Process each row with callback |
| Paged Read | `cxlsb_reader_read_page` | Read a start row and row count |
| Streaming Read | `cxlsb_reader_start_streaming` + `read_batch` + `end_streaming` | Batch reading for large files |
| Template Fill (Marker) | `cxlsb_template_filler_fill_marker` | Fill at `${data}` marker position |
| Template Fill (Index) | `cxlsb_template_filler_fill_index` | Fill at specified row/column |
| Number Format & Style | `cxlsb_writer_add_format` + `cxlsb_writer_add_style` | Custom number formats and XF style indices |
| SST Management | Shared String Table | Automatic deduplication for text cells |

## Cell Types

| Type | Constant | BIFF12 Record |
|------|----------|---------------|
| Number | `CXLSB_CELL_NUMBER` | CellReal (type=4) |
| Text | `CXLSB_CELL_TEXT` | CellIsst (type=7, SST by default); falls back to CellSt (type=6) when SST is unavailable |
| Boolean | `CXLSB_CELL_BOOL` | CellBool (type=9) |
| Date | `CXLSB_CELL_DATE` | CellReal with date format |
| Blank | `CXLSB_CELL_BLANK` | CellBlank (type=0) |

## Build

### Prerequisites

- GCC or compatible C compiler (C99 support)
- libzip development package

```bash
# Ubuntu/Debian
sudo apt-get install libzip-dev

# Fedora/RHEL
sudo dnf install libzip-devel
```

### Compile

```bash
# Compile single test
gcc tests/test_simple_write.c \
  src/api/*.c src/format/*.c src/container/*.c src/io/*.c src/util/*.c \
  -Iinclude -Isrc $(pkg-config --libs libzip) -o /tmp/test_simple_write

# Run all tests
tests/run_tests.sh
```

## Usage

### Write XLSB

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

### Streaming Write (Large Files)

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

### Read XLSB

```c
#include "cxlsb/xlsb_reader.h"

void row_handler(int row, int col_count, cxlsb_cell_data_t* cells, void* data) {
    printf("Row %d: ", row);
    for (int i = 0; i < col_count; i++) {
        switch (cells[i].type) {
            case CXLSB_CELL_TEXT:
                printf("%s ", cells[i].value.text);
                break;
            case CXLSB_CELL_NUMBER:
                printf("%.2f ", cells[i].value.number);
                break;
            default:
                printf("(blank) ");
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

### Template Fill

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

## Error Handling

All API functions return `cxlsb_error_t` enum:

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

Use `cxlsb_error_string(err)` to get human-readable error description.

## Row/Column Limits and Compression

```c
#include "cxlsb/limits.h"

/* Cut on column overflow, error on row overflow (global, snapshotted per write) */
cxlsb_set_overflow(CXLSB_OVERFLOW_CUT, CXLSB_OVERFLOW_ERROR);

cxlsb_writer_t* writer = cxlsb_writer_create("output.xlsb");
cxlsb_writer_set_compression_level(writer, 6);  /* 0=store, 1-9=deflate; default 1 */
```

- Limits: `CXLSB_MAX_COLUMNS = 16384`, `CXLSB_MAX_ROWS = 1048576`
- `CXLSB_OVERFLOW_ERROR` returns `CXLSB_ERR_LIMIT_EXCEEDED`; `CXLSB_OVERFLOW_CUT` truncates
- Batch writes, streaming writes and template filling share the same policy

## Project Structure

```
cxlsb/
├── include/cxlsb/       # Public API headers
│   ├── types.h          # Core type definitions
│   ├── error_codes.h    # Error codes
│   ├── cell_data.h      # Cell data helpers
│   ├── xlsb_writer.h    # Writer API
│   ├── xlsb_reader.h    # Reader API
│   ├── template_filler.h # Template fill API
│   └── limits.h         # Row/column limit policy
├── src/
│   ├── api/             # API implementation
│   ├── format/          # BIFF12 format handling
│   ├── container/       # ZIP container handling
│   ├── io/              # Buffered I/O
│   └── util/            # Utilities (varint, UTF-16)
├── tests/               # Test suite (15 tests)
│   └── resources/       # Test templates
└── tools/               # Utilities
```

## Test Coverage

| Test | Description |
|------|-------------|
| test_simple_write | Basic write operation |
| test_reader_simple | Basic read operation |
| test_read_write | Read/write roundtrip |
| test_stream_simple | Streaming mode |
| test_stream_multi_batch | Multiple streaming batches |
| test_stream_large | Large dataset streaming |
| test_pagination | Page-based reading |
| test_mixed_types | Multiple cell types |
| test_mixed_mode | Batch + streaming combined |
| template_fill_marker | Marker-based fill |
| template_fill_index | Index-based fill |
| test_template_filler | Template filler API |
| test_paged_read | Paged reading |
| test_stream_read | Streaming reading |
| test_limits_rk | Row/column limits and RK codec |

`tests/run_tests.sh` runs all 15 tests, all passing; the `test_styles` test must be compiled and run separately.

## Performance

- **Supplier optimization**: Single call per row (cached)
- **Dynamic buffers**: Size calculated from dimensions, no overflow risk
- **SST unified**: all text goes to the Shared String Table; inline CellSt only when SST is unavailable
- **RK encoding**: eligible integers use RK records to reduce file size
- **Streaming**: Supports datasets exceeding memory limits

## BIFF12 Format

This library implements MS-XLSB specification:
- Varint encoding for record type and size
- UTF-16LE for string content
- Shared String Table (SST) for text deduplication
- ROW_HDR record with span segments

## Related Documents

- [Manual](MANUAL_en.md) — Installation, build, API usage and examples
- [Changelog](CHANGELOG_en.md) — Version history

## License

Apache License 2.0

## Acknowledgments

Ported from [jxlsb](https://github.com/itcraft-cn/jxlsb) Java implementation.