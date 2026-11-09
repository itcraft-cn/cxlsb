[中文](CHANGELOG.md) | **English**

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Version numbers follow the Git tags `vX.Y.Z_YYYYMMDD`.

## [Unreleased]

### Changed

- Docs: synchronized README / MANUAL with the current API, adding paged and streaming reads plus number format and style notes; test count updated to 15
- Docs: reorganized version history to align with the Git tags `v0.1.0` and `v0.2.0`

## [0.2.0] - 2026-11-09

### Added

- LICENSE file
- **Row/column limit overflow policy** (aligned with rxlsb): `CXLSB_MAX_COLUMNS = 16384`,
  `CXLSB_MAX_ROWS = 1048576`; `cxlsb_set_column_overflow/set_row_overflow/set_overflow` with
  `CXLSB_OVERFLOW_ERROR/CUT`, returning `CXLSB_ERR_LIMIT_EXCEEDED` or truncating
- Configurable ZIP compression level: `cxlsb_writer_set_compression_level`, default **1**
- RK writing for integral numbers and `BrtCellRk` parsing in the reader
- Number formats and styles: `cxlsb_writer_add_format` / `cxlsb_writer_add_style`,
  per-cell `style_index` and `cxlsb_cell_*_with_style` constructors
- Paged reading via `cxlsb_reader_read_page` and streaming reads via
  `cxlsb_reader_start_streaming/read_batch/end_streaming`

### Fixed

- `BrtWsFmtInfo` record type corrected to **485** (old 229 made WPS show only the first row)
- `BrtWsDim` field order corrected; streaming writes use a prologue placeholder and backfill
- `BrtRowHdr` supports multi-span for >1024 columns; reader uses the last span
- `BrtCellSt` layout fixed (no extra flags byte); SST character count uses UTF-16 code units
- SST reading uses one bounds check, sequential decode, and handles UTF-16 surrogate pairs
- Blank cells no longer emit `BrtCellBlank`
- Template filling honors row/column limits
- Fixed numeric values being lost during template filling: `cxlsb_fill_result` now carries
  `cell_types`/`numbers`, so numbers, dates and booleans are no longer written as 0
- Free the previous fill buffers when filling the same sheet again
- RK encoding/decoding uses the standard integer form (bit1=fInt, bit0=fX100)

### Changed

- All text now goes to the SST by default; inline `BrtCellSt` is only a fallback when SST is unavailable
- Documentation layout unified as bilingual (README / CHANGELOG / MANUAL with cross-links)
- Documentation: added "Design Positioning" section
- Version: bumped from 0.1.0 to 0.2.0

## [0.1.0] - 2026-05-01

### Added

- Initial release of the cxlsb ANSI C library
- Writer API: batch writes (`cxlsb_writer_write_batch`) and streaming writes (`start_sheet` / `write_rows` / `end_sheet`)
- Reader API: row iteration (`cxlsb_reader_for_each_row`)
- Template filler API: `fill_marker` (`${data}` marker) and `fill_index` (explicit position)
- Shared String Table (SST) management with text deduplication, load/modify/save
- Support for cell types: Number, Text, Boolean, Date, Blank
- Dynamic buffer sizing based on data dimensions
- Row-level caching to optimize supplier calls
- Cross-platform support (Linux, Windows)
- 12 comprehensive tests with 100% pass rate

### Fixed

- malloc NULL checks in template_filler.c, xlsb_reader.c and workbook_writer.c
- Reader column mapping: `cells[col - first_col]` for relative index
- SST string length uses character count (UTF-16 code units), not byte count
- WsView block alignment with correct byte layout
- WsProp / WsView / WsFmtInfo aligned with the jxlsb reference
- Workbook.bin and Content_Types.xml structure
- BIFF12 cell references and record sizes
- Streaming mode sheet_writer implementation
- Template preservation: keep cells outside the fill region

### Changed

- Optimized supplier calls: single invocation per row with caching
- Dynamic sheet buffer sizing in template_filler
- Dynamic SST buffer sizing based on string content
- Row/column header visibility control

---

## Version History

| Version | Date | Description |
|---------|------|-------------|
| 0.2.0 | 2026-11-09 | Format and limit policy alignment, template fill fixes |
| 0.1.0 | 2026-05-01 | Initial release: basic read/write, streaming and template fill |

## Roadmap

### [0.3.0] - Planned
- Cell font/fill/border styles (number formats and XF style indices are already supported)
- Conditional formatting
- Formula support (read-only)
- Merged cells in streaming mode

### [0.4.0] - Planned
- Windows CI/CD pipeline
- Performance benchmark suite
- Memory leak detection with valgrind
- Code coverage reporting

### [1.0.0] - Future
- Multi-sheet concurrent processing
- Encryption support
- Compression optimization
- Async I/O support

## Related Documents

- [README.md](README.md) (Chinese) / [README_en.md](README_en.md) (English)
- [MANUAL.md](MANUAL.md) (Chinese) / [MANUAL_en.md](MANUAL_en.md) (English)
