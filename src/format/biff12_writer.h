#ifndef CXLSB_BIFF12_WRITER_H
#define CXLSB_BIFF12_WRITER_H

#include "../io/buffer_writer.h"
#include "cxlsb/types.h"
#include "cxlsb/error_codes.h"

typedef struct cxlsb_biff12_writer {
    cxlsb_buffer_writer_t* buffer;
} cxlsb_biff12_writer_t;

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create(size_t initial_capacity);

cxlsb_biff12_writer_t* cxlsb_biff12_writer_create_fixed(size_t capacity);

void cxlsb_biff12_writer_destroy(cxlsb_biff12_writer_t* writer);

cxlsb_error_t cxlsb_biff12_writer_write_record_header(cxlsb_biff12_writer_t* writer,
                                                       uint32_t record_type,
                                                       uint32_t record_size);

cxlsb_error_t cxlsb_biff12_writer_write_empty_record(cxlsb_biff12_writer_t* writer,
                                                      uint32_t record_type);

cxlsb_error_t cxlsb_biff12_writer_write_varint(cxlsb_biff12_writer_t* writer, 
                                                uint32_t value);

cxlsb_error_t cxlsb_biff12_writer_write_wide_string(cxlsb_biff12_writer_t* writer,
                                                     const char* str);

cxlsb_error_t cxlsb_biff12_writer_write_cell_real(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   double value, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_rk(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 double value, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_st(cxlsb_biff12_writer_t* writer,
                                                 uint32_t row, uint32_t col,
                                                 const char* str, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_isst(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   uint32_t sst_index, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_bool(cxlsb_biff12_writer_t* writer,
                                                   uint32_t row, uint32_t col,
                                                   bool value, uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_cell_blank(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, uint32_t col,
                                                    uint32_t xf_id);

cxlsb_error_t cxlsb_biff12_writer_write_row_header(cxlsb_biff12_writer_t* writer,
                                                    uint32_t row, 
                                                    uint32_t first_col,
                                                    uint32_t last_col);

cxlsb_error_t cxlsb_biff12_writer_write_ws_dim(cxlsb_biff12_writer_t* writer,
                                                uint32_t first_row,
                                                uint32_t first_col,
                                                uint32_t last_row,
                                                uint32_t last_col);

uint8_t* cxlsb_biff12_writer_get_buffer(cxlsb_biff12_writer_t* writer);

size_t cxlsb_biff12_writer_get_length(cxlsb_biff12_writer_t* writer);

void cxlsb_biff12_writer_reset(cxlsb_biff12_writer_t* writer);

#endif /* CXLSB_BIFF12_WRITER_H */