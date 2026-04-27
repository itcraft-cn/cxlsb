#include "cxlsb/error_codes.h"

const char* cxlsb_error_string(cxlsb_error_t error) {
    switch (error) {
        case CXLSB_OK: return "Success";
        case CXLSB_ERR_NULL_PTR: return "Null pointer";
        case CXLSB_ERR_NO_MEMORY: return "No memory";
        case CXLSB_ERR_INVALID_ARG: return "Invalid argument";
        case CXLSB_ERR_BUFFER_OVERFLOW: return "Buffer overflow";
        case CXLSB_ERR_FILE_NOT_FOUND: return "File not found";
        case CXLSB_ERR_FILE_OPEN_FAILED: return "File open failed";
        case CXLSB_ERR_FILE_WRITE_FAILED: return "File write failed";
        case CXLSB_ERR_FILE_READ_FAILED: return "File read failed";
        case CXLSB_ERR_ZIP_OPEN_FAILED: return "ZIP open failed";
        case CXLSB_ERR_ZIP_ADD_FAILED: return "ZIP add entry failed";
        case CXLSB_ERR_ZIP_ENTRY_NOT_FOUND: return "ZIP entry not found";
        case CXLSB_ERR_ZIP_READ_FAILED: return "ZIP read failed";
        case CXLSB_ERR_INVALID_RECORD: return "Invalid BIFF12 record";
        case CXLSB_ERR_INVALID_CELL_TYPE: return "Invalid cell type";
        case CXLSB_ERR_INVALID_SHEET_INDEX: return "Invalid sheet index";
        case CXLSB_ERR_PARSE_FAILED: return "Parse failed";
        case CXLSB_ERR_NOT_IN_TEMPLATE_MODE: return "Not in template mode";
        case CXLSB_ERR_IN_TEMPLATE_MODE: return "In template mode";
        case CXLSB_ERR_SHEET_NOT_STARTED: return "Sheet not started";
        case CXLSB_ERR_SHEET_NOT_ENDED: return "Sheet not ended";
        case CXLSB_ERR_FILL_NOT_STARTED: return "Fill not started";
        case CXLSB_ERR_MARKER_NOT_FOUND: return "Marker not found";
        case CXLSB_ERR_TEMPLATE_LOAD_FAILED: return "Template load failed";
        default: return "Unknown error";
    }
}