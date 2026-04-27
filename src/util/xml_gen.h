#ifndef CXLSB_XML_GEN_H
#define CXLSB_XML_GEN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cxlsb/error_codes.h"

cxlsb_error_t cxlsb_xml_gen_theme(uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_xml_gen_app(int sheet_count, uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_xml_gen_core(uint8_t** out, size_t* out_length);

cxlsb_error_t cxlsb_xml_gen_content_types(int sheet_count, 
                                            bool has_sst,
                                            uint8_t** out, 
                                            size_t* out_length);

#endif /* CXLSB_XML_GEN_H */