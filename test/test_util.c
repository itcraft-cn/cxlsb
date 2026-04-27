#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "src/util/varint.h"
#include "src/util/utf16.h"
#include "src/util/xml_gen.h"
#include "src/util/rels_gen.h"

static void test_varint_encode_decode(void) {
    uint8_t buf[10];
    
    int len = cxlsb_varint_encode(127, buf);
    assert(len == 1);
    assert(buf[0] == 127);
    
    uint32_t val;
    int dec_len = cxlsb_varint_decode(buf, 0, &val);
    assert(dec_len == 1);
    assert(val == 127);
    printf("test_varint_small: PASS\n");
    
    len = cxlsb_varint_encode(300, buf);
    assert(len == 2);
    
    dec_len = cxlsb_varint_decode(buf, 0, &val);
    assert(dec_len == 2);
    assert(val == 300);
    printf("test_varint_medium: PASS\n");
    
    len = cxlsb_varint_encode(100000, buf);
    assert(len >= 2);
    
    dec_len = cxlsb_varint_decode(buf, 0, &val);
    assert(val == 100000);
    printf("test_varint_large: PASS\n");
}

static void test_utf16_convert(void) {
    const char* text = "Hello";
    uint8_t utf16[20];
    size_t utf16_len;
    
    cxlsb_error_t err = cxlsb_utf8_to_utf16le(text, utf16, &utf16_len);
    assert(err == CXLSB_OK);
    assert(utf16_len == 10);
    
    assert(utf16[0] == 'H');
    assert(utf16[1] == 0);
    assert(utf16[2] == 'e');
    assert(utf16[3] == 0);
    
    char* back = cxlsb_utf16le_to_utf8(utf16, utf16_len);
    assert(back != NULL);
    assert(strcmp(back, text) == 0);
    free(back);
    
    printf("test_utf16_convert: PASS\n");
}

static void test_xml_gen(void) {
    uint8_t* theme;
    size_t theme_len;
    
    cxlsb_error_t err = cxlsb_xml_gen_theme(&theme, &theme_len);
    assert(err == CXLSB_OK);
    assert(theme_len > 0);
    assert(strncmp((char*)theme, "<?xml", 5) == 0);
    free(theme);
    
    uint8_t* app;
    size_t app_len;
    err = cxlsb_xml_gen_app(3, &app, &app_len);
    assert(err == CXLSB_OK);
    free(app);
    
    uint8_t* core;
    size_t core_len;
    err = cxlsb_xml_gen_core(&core, &core_len);
    assert(err == CXLSB_OK);
    free(core);
    
    printf("test_xml_gen: PASS\n");
}

static void test_rels_gen(void) {
    uint8_t* root;
    size_t root_len;
    
    cxlsb_error_t err = cxlsb_rels_gen_root(&root, &root_len);
    assert(err == CXLSB_OK);
    assert(root_len > 0);
    free(root);
    
    uint8_t* wb;
    size_t wb_len;
    err = cxlsb_rels_gen_workbook(3, true, &wb, &wb_len);
    assert(err == CXLSB_OK);
    assert(wb_len > 0);
    free(wb);
    
    printf("test_rels_gen: PASS\n");
}

int main(void) {
    test_varint_encode_decode();
    test_utf16_convert();
    test_xml_gen();
    test_rels_gen();
    
    printf("\nAll util tests passed!\n");
    return 0;
}