# 行号列标显隐控制技术说明

## 问题背景

cxlsb生成的XLSB文件，用WPS/Excel打开时，发现：
- 列坐标字母(A, B, C...)不显示
- 行坐标数字(1, 2, 3...)不显示

对比jxlsb生成的参考文件，坐标正常显示。

## 根因分析

通过对比sheet1.bin的WsProp记录：

**错误实现（隐藏）：**
```
WsProp size=16, data=00 00 00 00 [8字节FF] 00 00 00 00
```

**正确实现（显示）：**
```
WsProp size=23, data=C9 04 02 00 00 00 00 00 00 00 00 [8字节FF] FF FF FF FF 00 00 00 00
```

## 技术原理

### BrtWsProp记录结构

根据MS-XLSB规范：

| 偏移 | 字段 | 说明 |
|------|------|------|
| 0-1 | wsb | 工作表属性标志位 |
| 2-3 | tabid | 标签页ID |
| 4-11 | unused | 未使用 |
| 12-19 | ffMask | FF掩码 |
| 20-23 | unused2 | 未使用 |

### 关键字节解析

```
0xC9 = 0b11001001
```

wsb标志位含义：
- Bit 0: 显示行标题
- Bit 1: 显示列标题  
- Bit 2: 显示网格线
- 其他位控制其他显示属性

`0xC9 0x04` 组合启用标准视图显示。

### 完整数据

```c
static const uint8_t ws_prop_data[] = {
    0xC9, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
};
```

size = 23字节

## 相关记录

### BrtBeginWsView (30字节)

```c
static const uint8_t ws_view_data[] = {
    0xDC, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
```

控制视图缩放比例等。

### BrtWsFmtInfo (12字节)

```c
static const uint8_t fmt_info_data[] = {
    0x00, 0x09, 0x00, 0x00, 0x08, 0x00, 0x0E, 0x01,
    0x00, 0x00, 0x00, 0x00
};
```

控制默认列宽、行高等。

## 最佳实践

1. **不要简化WsProp** - 必须使用完整的23字节数据
2. **从参考文件提取** - 使用Excel生成文件后提取二进制数据
3. **字节对比验证** - 用xxd对比生成文件和参考文件

## 验证方法

```bash
# 提取sheet1.bin
unzip -p file.xlsb xl/worksheets/sheet1.bin > sheet.bin

# 对比前100字节
xxd -l 100 sheet.bin
```

## 参考资料

- MS-XLSB: [2.3.2.1 BrtWsProp]
- jxlsb: SheetWriter.writeBrtWsProp()