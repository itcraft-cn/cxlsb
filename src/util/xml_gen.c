#include "xml_gen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* THEME_XML = 
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"Office Theme\">\n"
"<a:themeElements>\n"
"<a:clrScheme name=\"Office\">\n"
"<a:dk1><a:sysClr val=\"windowText\" lastClr=\"000000\"/></a:dk1>\n"
"<a:lt1><a:sysClr val=\"window\" lastClr=\"FFFFFF\"/></a:lt1>\n"
"<a:dk2><a:srgbClr val=\"1F497D\"/></a:dk2>\n"
"<a:lt2><a:srgbClr val=\"EEECE1\"/></a:lt2>\n"
"<a:accent1><a:srgbClr val=\"4F81BD\"/></a:accent1>\n"
"<a:accent2><a:srgbClr val=\"C0504D\"/></a:accent2>\n"
"<a:accent3><a:srgbClr val=\"9BBB59\"/></a:accent3>\n"
"<a:accent4><a:srgbClr val=\"8064A2\"/></a:accent4>\n"
"<a:accent5><a:srgbClr val=\"4BACC6\"/></a:accent5>\n"
"<a:accent6><a:srgbClr val=\"F79646\"/></a:accent6>\n"
"<a:hlink><a:srgbClr val=\"0000FF\"/></a:hlink>\n"
"<a:folHlink><a:srgbClr val=\"800080\"/></a:folHlink>\n"
"</a:clrScheme>\n"
"<a:fontScheme name=\"Office\">\n"
"<a:majorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont>\n"
"<a:minorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont>\n"
"</a:fontScheme>\n"
"<a:fmtScheme name=\"Office\">\n"
"<a:fillStyleLst><a:noFill/><a:solidFill/><a:gradFill/></a:fillStyleLst>\n"
"<a:lnStyleLst><a:noLn/><a:ln w=\"25400\"/><a:ln w=\"25400\"/></a:lnStyleLst>\n"
"<a:effectStyleLst><a:effectStyle/><a:effectStyle/><a:effectStyle/></a:effectStyleLst>\n"
"</a:fmtScheme>\n"
"</a:themeElements>\n"
"</a:theme>\n";

static const char* CORE_XML =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\""
" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
" xmlns:dcterms=\"http://purl.org/dc/terms/\""
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
"<dc:creator>cxlsb</dc:creator>\n"
"<dcterms:created xsi:type=\"dcterms:W3CDTF\">2026-04-27T00:00:00Z</dcterms:created>\n"
"</cp:coreProperties>\n";

cxlsb_error_t cxlsb_xml_gen_theme(uint8_t** out, size_t* out_length) {
    size_t len = strlen(THEME_XML);
    uint8_t* buffer = (uint8_t*)malloc(len + 1);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(buffer, THEME_XML, len);
    buffer[len] = '\0';
    
    *out = buffer;
    *out_length = len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_xml_gen_app(int sheet_count, uint8_t** out, size_t* out_length) {
    char* buffer = (char*)malloc(1024);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int len = snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">\n"
        "<Application>cxlsb</Application>\n"
        "<Sheets>%d</Sheets>\n"
        "</Properties>\n",
        sheet_count);
    
    if (len < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_xml_gen_core(uint8_t** out, size_t* out_length) {
    size_t len = strlen(CORE_XML);
    uint8_t* buffer = (uint8_t*)malloc(len + 1);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    memcpy(buffer, CORE_XML, len);
    buffer[len] = '\0';
    
    *out = buffer;
    *out_length = len;
    
    return CXLSB_OK;
}

cxlsb_error_t cxlsb_xml_gen_content_types(int sheet_count, 
                                            bool has_sst,
                                            uint8_t** out, 
                                            size_t* out_length) {
    char* buffer = (char*)malloc(4096);
    if (buffer == NULL) {
        return CXLSB_ERR_NO_MEMORY;
    }
    
    int pos = snprintf(buffer, 4096,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        "<Default Extension=\"bin\" ContentType=\"application/vnd.ms-excel.sheet.binary.macroEnabled.main\"/>\n"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>\n"
        "<Override PartName=\"/docProps/app.xml\""
        " ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>\n"
        "<Override PartName=\"/docProps/core.xml\""
        " ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>\n"
        "<Override PartName=\"/xl/workbook.bin\""
        " ContentType=\"application/vnd.ms-excel.workbook.binary\"/>\n"
        "<Override PartName=\"/xl/styles.bin\""
        " ContentType=\"application/vnd.ms-excel.styles\"/>\n"
        "<Override PartName=\"/xl/theme/theme1.xml\""
        " ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>\n");
    
    if (pos < 0) {
        free(buffer);
        return CXLSB_ERR_BUFFER_OVERFLOW;
    }
    
    if (has_sst) {
        pos += snprintf(buffer + pos, 4096 - pos,
            "<Override PartName=\"/xl/sharedStrings.bin\""
            " ContentType=\"application/vnd.ms-excel.sharedStrings\"/>\n");
    }
    
    for (int i = 1; i <= sheet_count; i++) {
        pos += snprintf(buffer + pos, 4096 - pos,
            "<Override PartName=\"/xl/worksheets/sheet%d.bin\""
            " ContentType=\"application/vnd.ms-excel.worksheet\"/>\n", i);
    }
    
    pos += snprintf(buffer + pos, 4096 - pos, "</Types>\n");
    
    *out = (uint8_t*)buffer;
    *out_length = (size_t)pos;
    
    return CXLSB_OK;
}