#ifndef CXLSB_LIMITS_H
#define CXLSB_LIMITS_H

#include <stdbool.h>

/** Excel 规范最大列数（XFD）。 */
#define CXLSB_MAX_COLUMNS 16384
/** Excel 规范最大行数。 */
#define CXLSB_MAX_ROWS 1048576

/** 超出上限时的行为。 */
typedef enum cxlsb_overflow_behavior {
    CXLSB_OVERFLOW_ERROR = 0, /* 返回 CXLSB_ERR_LIMIT_EXCEEDED */
    CXLSB_OVERFLOW_CUT = 1    /* 静默截断到上限 */
} cxlsb_overflow_behavior_t;

/*
 * 全局配置：可随时修改，但每个写入操作在开始时快照，
 * 因此修改只影响其后的操作。列/行按维度“后设置覆盖先设置”。
 */
void cxlsb_set_column_overflow(cxlsb_overflow_behavior_t behavior);
void cxlsb_set_row_overflow(cxlsb_overflow_behavior_t behavior);
void cxlsb_set_overflow(cxlsb_overflow_behavior_t columns, cxlsb_overflow_behavior_t rows);

cxlsb_overflow_behavior_t cxlsb_get_column_overflow(void);
cxlsb_overflow_behavior_t cxlsb_get_row_overflow(void);

#endif /* CXLSB_LIMITS_H */
