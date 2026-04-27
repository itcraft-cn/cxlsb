#include "varint.h"

int cxlsb_varint_encode(uint32_t value, uint8_t* out) {
    int count = 0;
    
    while (value >= 0x80) {
        out[count++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    out[count++] = (uint8_t)value;
    
    return count;
}

int cxlsb_varint_decode(const uint8_t* buffer, size_t offset, uint32_t* out_value) {
    uint32_t result = 0;
    int shift = 0;
    size_t pos = offset;
    int count = 0;
    
    while (1) {
        uint8_t byte = buffer[pos++];
        count++;
        
        result |= (uint32_t)(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            break;
        }
        
        shift += 7;
        if (shift >= 32) {
            return -1;
        }
    }
    
    *out_value = result;
    return count;
}

int cxlsb_varint_decode_at(const uint8_t* buffer, uint32_t* out_value, size_t* bytes_read) {
    uint32_t result = 0;
    int shift = 0;
    size_t pos = 0;
    
    while (1) {
        uint8_t byte = buffer[pos++];
        
        result |= (uint32_t)(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            break;
        }
        
        shift += 7;
        if (shift >= 32) {
            *bytes_read = pos;
            return -1;
        }
    }
    
    *out_value = result;
    *bytes_read = pos;
    return 0;
}

int cxlsb_varint_size(uint32_t value) {
    if (value < 0x80) return 1;
    if (value < 0x4000) return 2;
    if (value < 0x200000) return 3;
    if (value < 0x10000000) return 4;
    return 5;
}