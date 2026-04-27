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