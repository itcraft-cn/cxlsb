#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/cxlsb/xlsb_writer.h"
#include "../include/cxlsb/xlsb_reader.h"
#include "../include/cxlsb/cell_data.h"

#define TOTAL_ROWS 1000000
#define COLUMN_COUNT 10
#define PAGE_SIZE 1000

static char name_pool[10000][32];
static char cat_pool[100][32];
static char r_pool[10][32];
static int pool_initialized = 0;

static void init_string_pool(void) {
    if (!pool_initialized) {
        for (int i = 0; i < 10000; i++) sprintf(name_pool[i], "Name-%d", i);
        for (int i = 0; i < 100; i++) sprintf(cat_pool[i], "Cat-%d", i);
        for (int i = 0; i < 10; i++) sprintf(r_pool[i], "R-%d", i);
        pool_initialized = 1;
    }
}

static const char* BATCH_FILE = "/tmp/cxlsb_batch_write.xlsb";
static const char* STREAM_FILE = "/tmp/cxlsb_stream_write.xlsb";

typedef struct {
    int offset;
} batch_data_t;

typedef struct {
    int count;
} read_ctx_t;

static cxlsb_cell_data_t create_cell(int row, int col) {
    switch (col) {
        case 0:
            return cxlsb_cell_text(name_pool[row % 10000]);
        case 1:
            return cxlsb_cell_text(cat_pool[row % 100]);
        case 2:
            return cxlsb_cell_text(r_pool[row % 10]);
        case 3:
            return cxlsb_cell_number(row);
        case 4:
            return cxlsb_cell_number(row * 10);
        case 5:
            return cxlsb_cell_number(row * 100);
        case 6:
            return cxlsb_cell_number(row % 1000);
        case 7:
            return cxlsb_cell_number(row * 1.5);
        case 8:
            return cxlsb_cell_number(row * 2.345);
        case 9:
            return cxlsb_cell_number(row / 7.0);
        default:
            return cxlsb_cell_blank();
    }
}

static cxlsb_cell_data_t batch_supplier(int row, int col, void* ud) {
    batch_data_t* data = (batch_data_t*)ud;
    int actual_row = data->offset + row;
    return create_cell(actual_row, col);
}

static cxlsb_cell_data_t stream_supplier(int row, int col, void* ud) {
    (void)ud;
    return create_cell(row, col);
}

static void row_counter_handler(int row, int cols, cxlsb_cell_data_t* cells, void* ud) {
    read_ctx_t* ctx = (read_ctx_t*)ud;
    ctx->count++;
}

static long get_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

static double get_time_diff_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

int main(void) {
    init_string_pool();
    
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         cxlsb 性能基准测试 (100万行 × 10字段)                 ║\n");
    printf("║         字段类型: 3字符串 + 4整数 + 3浮点数                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    struct timespec start_time, end_time;
    double elapsed_ms;
    long file_size;
    
    printf("═════════════════════════════════════════════════════════════\n");
    printf("1. 普通分页写 (startSheet + writeRows + endSheet)\n");
    printf("═════════════════════════════════════════════════════════════\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    cxlsb_writer_t* batch_writer = cxlsb_writer_create(BATCH_FILE);
    cxlsb_writer_start_sheet(batch_writer, "Data", COLUMN_COUNT);
    
    batch_data_t batch_data = {0};
    int offset = 0;
    while (offset < TOTAL_ROWS) {
        int batch_size = (offset + PAGE_SIZE > TOTAL_ROWS) ? (TOTAL_ROWS - offset) : PAGE_SIZE;
        batch_data.offset = offset;
        cxlsb_writer_write_rows(batch_writer, offset, batch_supplier, &batch_data, batch_size);
        offset += batch_size;
    }
    
    cxlsb_writer_end_sheet(batch_writer);
    cxlsb_writer_destroy(batch_writer);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_ms = get_time_diff_ms(start_time, end_time);
    file_size = get_file_size(BATCH_FILE);
    
    printf("写入耗时: %.0f ms\n", elapsed_ms);
    printf("文件大小: %.2f MB\n", file_size / 1000000.0);
    printf("写入速度: %.0f 行/秒\n", TOTAL_ROWS / (elapsed_ms / 1000.0));
    
    printf("\n═════════════════════════════════════════════════════════════\n");
    printf("2. 普通分页读 (for_each_row)\n");
    printf("═════════════════════════════════════════════════════════════\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    cxlsb_reader_t* batch_reader = cxlsb_reader_create(BATCH_FILE);
    read_ctx_t ctx = {0};
    cxlsb_reader_for_each_row(batch_reader, 0, row_counter_handler, &ctx);
    cxlsb_reader_destroy(batch_reader);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_ms = get_time_diff_ms(start_time, end_time);
    
    printf("读取耗时: %.0f ms\n", elapsed_ms);
    printf("读取速度: %.0f 行/秒\n", TOTAL_ROWS / (elapsed_ms / 1000.0));
    
    printf("\n═════════════════════════════════════════════════════════════\n");
    printf("3. 普通流式写 (write_batch)\n");
    printf("═════════════════════════════════════════════════════════════\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    cxlsb_writer_t* stream_writer = cxlsb_writer_create(STREAM_FILE);
    cxlsb_writer_write_batch(stream_writer, "Data", stream_supplier, NULL, TOTAL_ROWS, COLUMN_COUNT);
    cxlsb_writer_destroy(stream_writer);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_ms = get_time_diff_ms(start_time, end_time);
    file_size = get_file_size(STREAM_FILE);
    
    printf("写入耗时: %.0f ms\n", elapsed_ms);
    printf("文件大小: %.2f MB\n", file_size / 1000000.0);
    printf("写入速度: %.0f 行/秒\n", TOTAL_ROWS / (elapsed_ms / 1000.0));
    
    printf("\n═════════════════════════════════════════════════════════════\n");
    printf("4. 普通流式读 (for_each_row)\n");
    printf("═════════════════════════════════════════════════════════════\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    cxlsb_reader_t* stream_reader = cxlsb_reader_create(STREAM_FILE);
    read_ctx_t stream_ctx = {0};
    cxlsb_reader_for_each_row(stream_reader, 0, row_counter_handler, &stream_ctx);
    cxlsb_reader_destroy(stream_reader);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_ms = get_time_diff_ms(start_time, end_time);
    
    printf("读取耗时: %.0f ms\n", elapsed_ms);
    printf("读取速度: %.0f 行/秒\n", TOTAL_ROWS / (elapsed_ms / 1000.0));
    
    
    
    return 0;
}