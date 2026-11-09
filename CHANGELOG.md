**中文** | [English](CHANGELOG_en.md)

# 变更日志

本项目所有值得注意的变更都记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [未发布]

### 新增

- LICENSE 文件
- **行列上限溢出策略**（与 rxlsb 对齐）：`CXLSB_MAX_COLUMNS = 16384`、`CXLSB_MAX_ROWS = 1048576`；
  `cxlsb_set_column_overflow/set_row_overflow/set_overflow`，`CXLSB_OVERFLOW_ERROR/CUT`；
  每次写入通过 `cxlsb_sheet_limits_*` 快照，超限返回 `CXLSB_ERR_LIMIT_EXCEEDED` 或截断
- 写入器 ZIP 压缩级别可配置：`cxlsb_writer_set_compression_level`，默认 **1（速度优先）**
- RK 写入支持（整数且 |n| < 2^29），读取器补上 `BrtCellRk` 解析
- 数字格式与样式：`cxlsb_writer_add_format` / `cxlsb_writer_add_style`，单元格 `style_index` 与 `cxlsb_cell_*_with_style` 构造
- 分页读取 `cxlsb_reader_read_page` 与流式读取 `cxlsb_reader_start_streaming/read_batch/end_streaming`

### 修复

- `BrtWsFmtInfo` 记录类型修正为 **485**（旧值 229 会导致 WPS 只显示第一行）
- `BrtWsDim` 字段顺序修正为 first_row/last_row/first_col/last_col；流式写入改为前导占位、结束时回填
- `BrtRowHdr` 支持 >1024 列的多 span，读取端按最后一个 span 计算列范围
- `BrtCellSt` 布局修正（不再多写 1 字节 flags）；SST 序列化字符数按 UTF-16 码元计算
- SST 读取改为一次边界检查 + 顺序解码，并正确处理 UTF-16 代理对
- 空白单元格不再写入 `BrtCellBlank`
- 模板填充统一受行列上限约束（维度裁剪、超限单元格/填充列跳过）
- 修复模板填充数值丢失：`cxlsb_fill_result` 增加 `cell_types`/`numbers`，数值、日期、布尔值不再被写成 0
- 模板填充重复调用时释放上一轮缓冲区，避免内存泄漏
- RK 编码/解码改为标准整数形式（bit1=fInt，bit0=fX100）

### 变更

- 文档：新增「设计定位」章节
- 文档：同步测试套件（15 个测试）与 API 清单
- 版本：进入 0.2.0

## [1.0.0] - 2026-04-28

### 新增
- cxlsb ANSI C 库首次发布
- 写入 API，支持批量与流式模式
- 读取 API，支持行迭代
- 模板填充 API，支持标记与索引填充
- 共享字符串表（SST）管理
- 支持单元格类型：数字、文本、布尔、日期、空白
- 按数据维度动态计算缓冲区大小
- 行级缓存以优化 supplier 调用
- 跨平台支持（Linux、Windows）
- 12 个综合测试，100% 通过

### 修复
- template_filler.c 中的 malloc NULL 检查
- xlsb_reader.c 中的 malloc NULL 检查
- workbook_writer.c 中的 malloc NULL 检查
- 读取器列映射：cells[col - first_col] 使用相对索引
- SST 字符串长度：utf16_len / 2（字符数而非字节数）
- WsView 块对齐，使用正确的字节布局
- 模板保留：保留填充区域外的单元格

### 变更
- 优化 supplier 调用：每行单次调用并缓存
- 定义 CXLSB_SST_INLINE_THRESHOLD 常量（替代魔数）
- template_filler 中动态计算工作表缓冲区大小
- 根据字符串内容动态计算 SST 缓冲区大小

## [0.9.0] - 2026-04-27

### 新增
- 面向大数据集的流式写入模式
- 支持 ${data} 标记检测的模板填充
- 读写往返测试
- 混合模式测试（批量 + 流式）

### 修复
- 流式模式的 sheet_writer 实现
- 单元格引用与记录大小
- Workbook.bin 与 Content_Types.xml 结构

## [0.8.0] - 2026-04-26

### 新增
- 基础 XLSB 写入功能
- 基础 XLSB 读取功能
- BIFF12 格式处理（varint 编码、UTF-16LE）
- 基于 libzip 的 ZIP 容器处理
- 带哈希去重的 SST 表

### 修复
- WsProp/WsView/WsFmtInfo 对齐 jxlsb 参考实现
- 行/列表头可见性控制

## [0.7.0] - 2026-04-25

### 新增
- 项目结构搭建
- 核心类型定义（types.h）
- 错误码定义（error_codes.h）
- 单元格数据辅助（cell_data.h）
- 缓冲区读写实现
- Varint 编解码
- UTF-8 转 UTF-16LE

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 0.2.0 | 2026-11-09 | 未发布：格式与上限策略对齐 |
| 1.0.0 | 2026-04-28 | 生产发布，全部测试通过 |
| 0.9.0 | 2026-04-27 | 流式与模板填充 |
| 0.8.0 | 2026-04-26 | 基础读写功能 |
| 0.7.0 | 2026-04-25 | 项目初始化 |

## 路线图

### [1.1.0] - 计划中
- 单元格字体/填充/边框样式（当前已支持数字格式与 XF 样式索引）
- 条件格式
- 公式支持（只读）
- 流式模式下的合并单元格

### [1.2.0] - 计划中
- Windows CI/CD 流水线
- 性能基准套件
- 使用 valgrind 的内存泄漏检测
- 代码覆盖率报告

### [2.0.0] - 未来
- 多工作表并发处理
- 加密支持
- 压缩优化
- 异步 I/O 支持

## 相关文档

- [README.md](README.md)（中文） / [README_en.md](README_en.md)（英文）
- [MANUAL.md](MANUAL.md)（中文） / [MANUAL_en.md](MANUAL_en.md)（英文）
