#ifndef CXLSB_RELS_GEN_H
#define CXLSB_RELS_GEN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

cxlsb_error_t cxlsb_rels_gen_root(uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_rels_gen_workbook(int sheet_count, 
                                        bool has_sst,
                                        uint8_t** out, 
                                        size_t* out_length);

#endif /* CXLSB_RELS_GEN_H */