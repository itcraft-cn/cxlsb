[中文](CHANGELOG.md) | **English**

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- LICENSE file
- **Row/column limit overflow policy** (aligned with rxlsb): `CXLSB_MAX_COLUMNS = 16384`,
  `CXLSB_MAX_ROWS = 1048576`; `cxlsb_set_column_overflow/set_row_overflow/set_overflow` with
  `CXLSB_OVERFLOW_ERROR/CUT`, returning `CXLSB_ERR_LIMIT_EXCEEDED` or truncating
- Configurable ZIP compression level: `cxlsb_writer_set_compression_level`, default **1**
- RK writing for integral numbers and `BrtCellRk` parsing in the reader

### Fixed

- `BrtWsFmtInfo` record type corrected to **485** (old 229 made WPS show only the first row)
- `BrtWsDim` field order corrected; streaming writes use a prologue placeholder and backfill
- `BrtRowHdr` supports multi-span for >1024 columns; reader uses the last span
- `BrtCellSt` layout fixed (no extra flags byte); SST character count uses UTF-16 code units
- SST reading uses one bounds check, sequential decode, and handles UTF-16 surrogate pairs
- Blank cells no longer emit `BrtCellBlank`
- Template filling honors row/column limits
- RK encoding/decoding uses the standard integer form (bit1=fInt, bit0=fX100)

### Changed

- Documentation: added "Design Positioning" section
- Version: moved to 0.2.0

## [1.0.0] - 2026-04-28

### Added
- Initial release of cxlsb ANSI C library
- Writer API with batch and streaming modes
- Reader API with row iteration
- Template filler API with marker and index-based fill
- Shared String Table (SST) management
- Support for cell types: Number, Text, Boolean, Date, Blank
- Dynamic buffer sizing based on data dimensions
- Row-level caching to optimize supplier calls
- Cross-platform support (Linux, Windows)
- 12 comprehensive tests with 100% pass rate

### Fixed
- malloc NULL checks in template_filler.c
- malloc NULL checks in xlsb_reader.c
- malloc NULL checks in workbook_writer.c
- Reader column mapping: cells[col - first_col] for relative index
- SST string length: utf16_len / 2 (char count, not byte count)
- WsView block alignment with correct byte layout
- Template preservation: keep cells outside fill region

### Changed
- Optimized supplier calls: single invocation per row with caching
- Defined CXLSB_SST_INLINE_THRESHOLD constant (replaced magic number)
- Dynamic sheet buffer sizing in template_filler
- Dynamic SST buffer sizing based on string content

## [0.9.0] - 2026-04-27

### Added
- Streaming write mode for large datasets
- Template fill with ${data} marker detection
- Read/write roundtrip tests
- Mixed mode test (batch + streaming)

### Fixed
- Streaming mode sheet_writer implementation
- Cell reference and record sizes
- Workbook.bin and Content_Types.xml structure

## [0.8.0] - 2026-04-26

### Added
- Basic XLSB writer functionality
- Basic XLSB reader functionality
- BIFF12 format handling (varint encoding, UTF-16LE)
- ZIP container handling with libzip
- SST table with hash-based deduplication

### Fixed
- WsProp/WsView/WsFmtInfo to match jxlsb reference
- Row/column header visibility control

## [0.7.0] - 2026-04-25

### Added
- Project structure setup
- Core type definitions (types.h)
- Error codes definition (error_codes.h)
- Cell data helpers (cell_data.h)
- Buffer writer/reader implementation
- Varint encoding/decoding
- UTF-8 to UTF-16LE conversion

---

## Version History

| Version | Date | Description |
|---------|------|-------------|
| 1.0.0 | 2026-04-28 | Production release, all tests passing |
| 0.9.0 | 2026-04-27 | Streaming and template fill |
| 0.8.0 | 2026-04-26 | Basic read/write functionality |
| 0.7.0 | 2026-04-25 | Project initialization |

## Roadmap

### [1.1.0] - Planned
- Pagination API with offset/limit parameters
- Cell style support (fonts, fills, borders)
- Conditional formatting
- Formula support (read-only)
- Merged cells in streaming mode

### [1.2.0] - Planned
- Windows CI/CD pipeline
- Performance benchmark suite
- Memory leak detection with valgrind
- Code coverage reporting

### [2.0.0] - Future
- Multi-sheet concurrent processing
- Encryption support
- Compression optimization
- Async I/O support