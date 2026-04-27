#include "../src/util/utf16.h"
#include <stdio.h>
#include <stdint.h>

int main() {
    uint8_t utf16[] = {0xe8, 0x90, 0xe8, 0x95, 0xba, 0x4e, 0x58, 0x54, 0xdf, 0x7e, 0xa1, 0x8b};
    char* utf8 = cxlsb_utf16le_to_utf8(utf16, 12);
    printf("Result: '%s'\n", utf8);
    
    for (int i = 0; utf8[i]; i++) {
        printf("byte[%d] = 0x%02x\n", i, (unsigned char)utf8[i]);
    }
    return 0;
}