#ifndef CXLSB_VARINT_H
#define CXLSB_VARINT_H

#include <stdint.h>
#include <stddef.h>

int cxlsb_varint_encode(uint32_t value, uint8_t* out);

int cxlsb_varint_decode(const uint8_t* buffer, size_t offset, uint32_t* out_value);

int cxlsb_varint_size(uint32_t value);

int cxlsb_varint_decode_at(const uint8_t* buffer, uint32_t* out_value, size_t* bytes_read);

#endif /* CXLSB_VARINT_H */