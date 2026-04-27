#include "utf16.h"
#include <stdlib.h>
#include <string.h>

cxlsb_error_t cxlsb_utf8_to_utf16le(const char* utf8, 
                                      uint8_t* out_utf16, 
                                      size_t* out_length) {
    if (utf8 == NULL || out_utf16 == NULL || out_length == NULL) {
        return CXLSB_ERR_NULL_PTR;
    }
    
    size_t utf8_len = strlen(utf8);
    size_t utf16_pos = 0;
    
    for (size_t i = 0; i < utf8_len; i++) {
        uint8_t c = (uint8_t)utf8[i];
        
        if (c < 0x80) {
            out_utf16[utf16_pos++] = c;
            out_utf16[utf16_pos++] = 0;
        } else if (c < 0xC0) {
            continue;
        } else if (c < 0xE0) {
            if (i + 1 < utf8_len) {
                uint16_t wc = ((c & 0x1F) << 6) | (utf8[i+1] & 0x3F);
                out_utf16[utf16_pos++] = (uint8_t)(wc & 0xFF);
                out_utf16[utf16_pos++] = (uint8_t)(wc >> 8);
                i++;
            }
        } else if (c < 0xF0) {
            if (i + 2 < utf8_len) {
                uint16_t wc = ((c & 0x0F) << 12) | 
                              ((utf8[i+1] & 0x3F) << 6) | 
                              (utf8[i+2] & 0x3F);
                out_utf16[utf16_pos++] = (uint8_t)(wc & 0xFF);
                out_utf16[utf16_pos++] = (uint8_t)(wc >> 8);
                i += 2;
            }
        } else {
            uint32_t wc = ((c & 0x07) << 18) | 
                          ((utf8[i+1] & 0x3F) << 12) | 
                          ((utf8[i+2] & 0x3F) << 6) | 
                          (utf8[i+3] & 0x3F);
            wc -= 0x10000;
            uint16_t high = (uint16_t)(0xD800 + (wc >> 10));
            uint16_t low = (uint16_t)(0xDC00 + (wc & 0x3FF));
            out_utf16[utf16_pos++] = (uint8_t)(high & 0xFF);
            out_utf16[utf16_pos++] = (uint8_t)(high >> 8);
            out_utf16[utf16_pos++] = (uint8_t)(low & 0xFF);
            out_utf16[utf16_pos++] = (uint8_t)(low >> 8);
            i += 3;
        }
    }
    
    *out_length = utf16_pos;
    return CXLSB_OK;
}

char* cxlsb_utf16le_to_utf8(const uint8_t* utf16, size_t length) {
    if (utf16 == NULL || length == 0) {
        return NULL;
    }
    
    size_t char_count = length / 2;
    char* utf8 = (char*)malloc(char_count * 3 + 1);
    if (utf8 == NULL) {
        return NULL;
    }
    
    size_t utf8_pos = 0;
    
    for (size_t i = 0; i < length; i += 2) {
        uint16_t wc = utf16[i] | (utf16[i+1] << 8);
        
        if (wc < 0x80) {
            utf8[utf8_pos++] = (char)wc;
        } else if (wc < 0x800) {
            utf8[utf8_pos++] = (char)(0xC0 | (wc >> 6));
            utf8[utf8_pos++] = (char)(0x80 | (wc & 0x3F));
        } else if (wc >= 0xD800 && wc <= 0xDBFF) {
            if (i + 3 < length) {
                uint16_t low = utf16[i+2] | (utf16[i+3] << 8);
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    uint32_t cp = 0x10000 + 
                                  ((wc - 0xD800) << 10) + 
                                  (low - 0xDC00);
                    utf8[utf8_pos++] = (char)(0xF0 | (cp >> 18));
                    utf8[utf8_pos++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                    utf8[utf8_pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    utf8[utf8_pos++] = (char)(0x80 | (cp & 0x3F));
                    i += 2;
                }
            }
        } else {
            utf8[utf8_pos++] = (char)(0xE0 | (wc >> 12));
            utf8[utf8_pos++] = (char)(0x80 | ((wc >> 6) & 0x3F));
            utf8[utf8_pos++] = (char)(0x80 | (wc & 0x3F));
        }
    }
    
    utf8[utf8_pos] = '\0';
    return utf8;
}

size_t cxlsb_utf16le_byte_length(const char* utf8) {
    if (utf8 == NULL) {
        return 0;
    }
    
    size_t len = strlen(utf8);
    size_t byte_count = 0;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t c = (uint8_t)utf8[i];
        
        if (c < 0x80) {
            byte_count += 2;
        } else if (c < 0xC0) {
            continue;
        } else if (c < 0xE0) {
            byte_count += 2;
            i++;
        } else if (c < 0xF0) {
            byte_count += 2;
            i += 2;
        } else {
            byte_count += 4;
            i += 3;
        }
    }
    
    return byte_count;
}