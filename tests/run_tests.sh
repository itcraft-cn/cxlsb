#!/bin/bash
# cxlsb Test Runner
# Runs all tests and reports results

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "=== cxlsb Test Suite ==="
echo ""

# Common compilation flags
CFLAGS="-Iinclude -Isrc"
LIBS="src/api/cell_data.c src/api/xlsb_reader.c src/api/xlsb_writer.c \
     src/container/xlsb_container.c src/container/xlsb_container_reader.c \
     src/format/biff12_writer.c src/format/sheet_writer.c src/format/sst_table.c \
     src/format/styles_registry.c src/format/workbook_writer.c \
     src/io/buffer_reader.c src/io/buffer_writer.c \
     src/util/error_codes.c src/util/rels_gen.c src/util/utf16.c src/util/varint.c src/util/xml_gen.c"
ZIP_LIBS="$(pkg-config --libs libzip)"

TOTAL=0
PASSED=0
FAILED=0

run_test() {
    local test_name=$1
    local test_file=$2
    local extra_flags=$3
    
    TOTAL=$((TOTAL + 1))
    echo "[$TOTAL] Running: $test_name"
    
    if gcc $CFLAGS tests/$test_file $LIBS -o /tmp/cxlsb_test $ZIP_LIBS -lm $extra_flags 2>/dev/null && \
       /tmp/cxlsb_test >/dev/null 2>&1; then
        echo "  ✓ PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAILED"
        FAILED=$((FAILED + 1))
    fi
}

# Test 1: Simple write
run_test "test_simple_write" "test_simple_write.c"

# Test 2: Reader simple
run_test "test_reader_simple" "test_reader_simple.c"

# Test 3: Read/Write roundtrip
run_test "test_read_write" "test_read_write.c"

# Test 4: Streaming simple
run_test "test_stream_simple" "test_stream_simple.c"

# Test 5: Multi-batch streaming
run_test "test_stream_multi_batch" "test_stream_multi_batch.c"

# Test 6: Pagination
run_test "test_pagination" "test_pagination.c"

# Test 7: Mixed types
run_test "test_mixed_types" "test_mixed_types.c"

# Test 8: Template fill (marker-based)
echo "[8] Running: template_fill_marker (special)"
TOTAL=$((TOTAL + 1))
if gcc $CFLAGS tests/template_fill_jxlsb.c \
   src/format/sheet_parser.c $LIBS -o /tmp/cxlsb_test $ZIP_LIBS 2>/dev/null && \
   /tmp/cxlsb_test >/dev/null 2>&1; then
    echo "  ✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAILED"
    FAILED=$((FAILED + 1))
fi

# Test 9: Template fill (index-based)
echo "[9] Running: template_fill_index (special)"
TOTAL=$((TOTAL + 1))
if gcc tests/template_fill_index.c \
   src/format/sheet_parser.c src/format/sst_table.c \
   src/format/biff12_writer.c src/io/buffer_writer.c \
   src/io/buffer_reader.c \
   src/util/error_codes.c src/util/utf16.c src/util/varint.c \
   -Iinclude -Isrc -o /tmp/cxlsb_test $ZIP_LIBS 2>/dev/null && \
   /tmp/cxlsb_test >/dev/null 2>&1; then
    echo "  ✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAILED"
    FAILED=$((FAILED + 1))
fi

# Test 10: Large streaming
echo "[10] Running: test_stream_large"
TOTAL=$((TOTAL + 1))
if gcc tests/test_stream_large.c $LIBS -Iinclude -Isrc -o /tmp/cxlsb_test $ZIP_LIBS 2>/dev/null && \
   /tmp/cxlsb_test >/dev/null 2>&1; then
    echo "  ✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAILED"
    FAILED=$((FAILED + 1))
fi

# Test 11: Mixed mode (batch + streaming)
echo "[11] Running: test_mixed_mode"
TOTAL=$((TOTAL + 1))
if gcc tests/test_mixed_mode.c $LIBS -Iinclude -Isrc -o /tmp/cxlsb_test $ZIP_LIBS 2>/dev/null && \
   /tmp/cxlsb_test >/dev/null 2>&1; then
    echo "  ✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAILED"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Summary ==="
echo "Total:  $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

rm -f /tmp/cxlsb_test

if [ $FAILED -eq 0 ]; then
    echo ""
    echo "All tests passed! ✓"
    exit 0
else
    echo ""
    echo "Some tests failed. ✗"
    exit 1
fi