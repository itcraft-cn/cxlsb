#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/io/buffer_writer.h"
#include "../src/io/buffer_reader.h"

static void test_buffer_writer_create(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(1024);
    assert(writer != NULL);
    assert(writer->capacity == 1024);
    assert(writer->position == 0);
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_writer_create: PASS\n");
}

static void test_buffer_writer_write_int(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(1024);
    assert(writer != NULL);
    
    cxlsb_error_t err = cxlsb_buffer_writer_write_int(writer, 0x12345678);
    assert(err == CXLSB_OK);
    assert(writer->position == 4);
    
    uint8_t* buf = cxlsb_buffer_writer_get_buffer(writer);
    assert(buf[0] == 0x78);
    assert(buf[1] == 0x56);
    assert(buf[2] == 0x34);
    assert(buf[3] == 0x12);
    
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_writer_write_int: PASS\n");
}

static void test_buffer_writer_expand(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(16);
    assert(writer != NULL);
    assert(writer->capacity == 16);
    
    for (int i = 0; i < 10; i++) {
        cxlsb_error_t err = cxlsb_buffer_writer_write_int(writer, i);
        assert(err == CXLSB_OK);
    }
    
    assert(writer->position == 40);
    assert(writer->capacity >= 40);
    
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_writer_expand: PASS\n");
}

static void test_buffer_reader_read_int(void) {
    uint8_t data[] = {0x78, 0x56, 0x34, 0x12, 0x01, 0x00, 0x00, 0x00};
    
    cxlsb_buffer_reader_t* reader = cxlsb_buffer_reader_create(data, 8);
    assert(reader != NULL);
    
    uint32_t val1 = cxlsb_buffer_reader_read_int(reader);
    assert(val1 == 0x12345678);
    assert(reader->position == 4);
    
    uint32_t val2 = cxlsb_buffer_reader_read_int(reader);
    assert(val2 == 1);
    assert(reader->position == 8);
    
    cxlsb_buffer_reader_destroy(reader);
    printf("test_buffer_reader_read_int: PASS\n");
}

static void test_buffer_reader_eof(void) {
    uint8_t data[] = {0x01, 0x02};
    
    cxlsb_buffer_reader_t* reader = cxlsb_buffer_reader_create(data, 2);
    assert(reader != NULL);
    
    cxlsb_buffer_reader_read_int(reader);
    assert(cxlsb_buffer_reader_is_eof(reader));
    
    cxlsb_buffer_reader_destroy(reader);
    printf("test_buffer_reader_eof: PASS\n");
}

static void test_buffer_write_read_cycle(void) {
    cxlsb_buffer_writer_t* writer = cxlsb_buffer_writer_create(1024);
    
    cxlsb_buffer_writer_write_int(writer, 100);
    cxlsb_buffer_writer_write_double(writer, 3.14159);
    cxlsb_buffer_writer_write_byte(writer, 42);
    
    uint8_t* buf = cxlsb_buffer_writer_get_buffer(writer);
    size_t len = cxlsb_buffer_writer_get_length(writer);
    
    cxlsb_buffer_reader_t* reader = cxlsb_buffer_reader_create(buf, len);
    
    uint32_t int_val = cxlsb_buffer_reader_read_int(reader);
    assert(int_val == 100);
    
    double double_val = cxlsb_buffer_reader_read_double(reader);
    assert(double_val > 3.14158 && double_val < 3.14160);
    
    uint8_t byte_val = cxlsb_buffer_reader_read_byte(reader);
    assert(byte_val == 42);
    
    cxlsb_buffer_reader_destroy(reader);
    cxlsb_buffer_writer_destroy(writer);
    printf("test_buffer_write_read_cycle: PASS\n");
}

int main(void) {
    test_buffer_writer_create();
    test_buffer_writer_write_int();
    test_buffer_writer_expand();
    test_buffer_reader_read_int();
    test_buffer_reader_eof();
    test_buffer_write_read_cycle();
    
    printf("\nAll buffer tests passed!\n");
    return 0;
}