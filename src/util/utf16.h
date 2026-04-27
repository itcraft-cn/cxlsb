#ifndef CXLSB_UTF16_H
#define CXLSB_UTF16_H

#include <stdint.h>
#include <stddef.h>
#include "cxlsb/error_codes.h"

cxlsb_error_t cxlsb_utf8_to_utf16le(const char* utf8, 
                                      uint8_t* out_utf16, 
                                      size_t* out_length);

char* cxlsb_utf16le_to_utf8(const uint8_t* utf16, size_t length);

size_t cxlsb_utf16le_byte_length(const char* utf8);

#endif /* CXLSB_UTF16_H */