[中文](MANUAL.md) | **English**

# cxlsb Manual

> This manual is for developers using cxlsb. It covers requirements, build, API usage and examples.
> For a feature overview see [README_en.md](README_en.md); for version history see [CHANGELOG_en.md](CHANGELOG_en.md).

## 1. Introduction

cxlsb is a pure ANSI C (C99) XLSB (Excel Binary Workbook) reader/writer library ported from the Java jxlsb. Its fixed starting point is high-performance data output (writing); reading was initially meant only for consuming templates, and high-performance reading followed naturally once reading was complete. It uses a procedural + function-pointer style and depends only on libzip, making it embedded-friendly.

## 2. Requirements

| Item | Requirement |
|------|-------------|
| Compiler | GCC or a compatible C compiler (C99) |
| Dependencies | libzip (ZIP container), zlib; optional zlog |

Install dependencies:

```bash
# Ubuntu/Debian
sudo apt-get install libzip-dev
# Fedora/RHEL
sudo dnf install libzip-devel
```

## 3. Build & Test

```bash
# Compile a single test
gcc tests/test_simple_write.c \
  src/api/*.c src/format/*.c src/container/*.c src/io/*.c src/util/*.c \
  -Iinclude -Isrc $(pkg-config --libs libzip) -o /tmp/test_simple_write

# Run all tests
tests/run_tests.sh
```

For a CMake build see `CMakeLists.txt` in the repository root.

## 4. API Usage

### 4.1 Writing

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

### 4.2 Streaming Write (Large Files)

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

### 4.3 Reading

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

### 4.4 Paged and Streaming Read

```c
#include "cxlsb/xlsb_reader.h"

/* Read 100 rows starting from row 1000 */
cxlsb_reader_read_page(reader, 0, 1000, 100, row_handler, NULL);

/* Or read in batches while streaming */
cxlsb_reader_start_streaming(reader, 0);
while (cxlsb_reader_has_more_rows(reader)) {
    cxlsb_reader_read_batch(reader, 5000, row_handler, NULL);
}
cxlsb_reader_end_streaming(reader);
```

### 4.5 Template Fill

```c
#include "cxlsb/template_filler.h"

cxlsb_cell_data_t data_supplier(int row, int col, void* data) {
    return cxlsb_cell_text("fill data");
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

Filled values keep their type: text uses SST, numbers/dates use RK/Real, booleans use Bool; blank cells emit no record.

### 4.6 Row/Column Limits and Compression

```c
#include "cxlsb/limits.h"

/* Cut on column overflow, error on row overflow (global, snapshotted per write) */
cxlsb_set_overflow(CXLSB_OVERFLOW_CUT, CXLSB_OVERFLOW_ERROR);

cxlsb_writer_t* writer = cxlsb_writer_create("output.xlsb");
cxlsb_writer_set_compression_level(writer, 6);  /* 0=store, 1-9=deflate; default 1 */
```

- Limits: `CXLSB_MAX_COLUMNS = 16384`, `CXLSB_MAX_ROWS = 1048576`
- `CXLSB_OVERFLOW_ERROR` returns `CXLSB_ERR_LIMIT_EXCEEDED`; `CXLSB_OVERFLOW_CUT` truncates
- Configuration is global and snapshotted when each write operation starts
- Batch writes, streaming writes and template filling share the same policy

### 4.7 Number Formats and Styles

```c
/* Register a number format (returns format ID), then a style (returns XF index) */
int fmt = cxlsb_writer_add_format(writer, "#,##0.00");
int style = cxlsb_writer_add_style(writer, fmt);

/* Bind the XF index via the styled cell constructors in the supplier */
return cxlsb_cell_number_with_style(row * 1.5, (uint32_t)style);
```

- Identical format strings and identical styles are deduplicated and return the existing ID
- Use `cxlsb_cell_number_with_style` / `cxlsb_cell_text_with_style` / `cxlsb_cell_date_with_style`
- You can also set the `style_index` field of `cxlsb_cell_data_t` directly

## 5. Supported Operations

| Feature | API | Description |
|---------|-----|-------------|
| Batch write | `cxlsb_writer_write_batch` | Write an entire sheet in one call |
| Streaming write | `cxlsb_writer_start_sheet` + `write_rows` + `end_sheet` | Append rows incrementally |
| Row iteration | `cxlsb_reader_for_each_row` | Process each row with a callback |
| Paged read | `cxlsb_reader_read_page` | Read a start row and row count |
| Streaming read | `cxlsb_reader_start_streaming` + `read_batch` + `end_streaming` | Batch reading for large files |
| Template fill (marker) | `cxlsb_template_filler_fill_marker` | Fill at the `${data}` marker position |
| Template fill (index) | `cxlsb_template_filler_fill_index` | Fill at a specified row/column |
| Number format & style | `cxlsb_writer_add_format` + `cxlsb_writer_add_style` | Custom number formats and XF style indices |
| SST management | Shared String Table | Automatic deduplication for text cells |

## 6. Cell Types

| Type | Constant | BIFF12 Record |
|------|----------|---------------|
| Number | `CXLSB_CELL_NUMBER` | CellReal (type=4) |
| Text | `CXLSB_CELL_TEXT` | CellIsst (type=7, SST by default); falls back to CellSt (type=6) when SST is unavailable |
| Boolean | `CXLSB_CELL_BOOL` | CellBool (type=9) |
| Date | `CXLSB_CELL_DATE` | CellReal with date format |
| Blank | `CXLSB_CELL_BLANK` | CellBlank (type=0) |

## 7. Error Handling

All API functions return a `cxlsb_error_t` enum; success is `CXLSB_OK` (0). Use `cxlsb_error_string(err)` for a human-readable description.

## 8. Memory Management

- Uses a create/destroy pattern
- All allocations check for NULL
- Use valgrind to detect memory leaks

## 9. Further Docs

- [README_en.md](README_en.md) — feature overview
- [CHANGELOG_en.md](CHANGELOG_en.md) — version history
- `docs/row_column_header_visibility.md` — row/column header visibility
- `tests/` — test suite
