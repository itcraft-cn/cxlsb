# cxlsb

## 项目概述

ANSI C 实现的 XLSB (Excel Binary Workbook) 格式读写库，移植自 Java jxlsb 项目。

## 功能特性

- **Writer API**: write_batch（一次性写入）、streaming（流式追加）
- **Reader API**: for_each_row（逐行读取）、pagination（分页读取）
- **Template Filler**: fill_marker（${data}标记填充）、fill_index（指定位置填充）
- **SST 处理**: 加载、修改、保存共享字符串表
- **跨平台**: Linux + Windows 支持

## 项目结构

```
cxlsb/
├── include/cxlsb/     # 公共 API 头文件
│   ├── types.h        # 核心类型定义
│   ├── error_codes.h  # 错误码定义
│   ├── cell_data.h    # 单元格数据 API
│   ├── xlsb_writer.h  # 写入器 API
│   ├── xlsb_reader.h  # 读取器 API
│   └── template_filler.h  # 模板填充 API
├── src/
│   ├── api/           # API 实现
│   ├── format/        # BIFF12 格式处理
│   ├── container/     # ZIP 容器处理
│   ├── io/            # 缓冲读写
│   └── util/          # 工具函数
├── tests/             # 测试文件
└── tools/             # 工具程序
```

## 构建与测试

### 编译命令

```bash
# 编译测试
gcc tests/test_xxx.c \
  src/api/*.c src/format/*.c src/container/*.c src/io/*.c src/util/*.c \
  -Iinclude -Isrc $(pkg-config --libs libzip) -o /tmp/test_xxx

# 运行测试
tests/run_tests.sh
```

### 测试覆盖

- 12 个测试全部通过
- 覆盖：write/read/streaming/template_fill/mixed_types/pagination

## 依赖

- libzip (ZIP 容器处理)
- C99 标准

## AI guide

### 角色定位

1. 你是资深架构师
    - 在开发前，会对需求进行详尽分析，提供多套方案，以上、中、下三策的形式呈现，以备后续决策参考
    - 在设计时，会充分考虑非功能性需求：安全性、可扩展性、可用性、可观测性、性能等
    - 在设计细节时，充分考虑各种设计模式及各语言特性
2. 你是资深开发者，对C非常了解
    - 对Linux/Windows底层库均非常熟悉
    - 对C的内存布局非常清楚
    - 开发上偏好过程式+函数指针
    - 对CPU指令也熟悉

### 交互规则

1. 处于 AI Coding Plan 包月模式下，Token 不考虑，时间不考虑，专注于高效而完整地工作
2. 所有交互均使用简体中文
3. 持续使用 skill /memrec 记忆
4. 每次沟通产出文件后，均执行 git 提交
5. git 仅以当前 `user.name` 提交，不推送到远端
6. git 提交均遵循约定式提交规范（Conventional Commits）执行
7. 编排计划或设计时，如过长(>3000行)，拆分为多份文档
8. 重要内容(plan、design等)，随时记录到 MEMORY.md 和 memrec
9. 版本管理忽略 MEMORY.md，写入 .gitignore，不提交到 Git

### 编码规范

授权读取：/disk2/helly_data/code/markdown/self-ai-spec/lang-spec/spec.ansi_c.md

Read /disk2/helly_data/code/markdown/self-ai-spec/lang-spec/spec.ansi_c.md

#### cxlsb 特定规范

1. **命名约定**
   - 公共 API: `cxlsb_` 前缀（如 `cxlsb_writer_create`）
   - 内部函数: 无前缀，static 声明
   - 结构体: `cxlsb_xxx_t` 类型别名

2. **错误处理**
   - 使用 `cxlsb_error_t` 枚举返回错误码
   - 成功返回 `CXLSB_OK` (0)
   - 提供 `cxlsb_error_string()` 获取错误描述

3. **内存管理**
   - API 使用 create/destroy 模式
   - 所有分配检查 NULL
   - 使用 valgrind 检测内存泄漏

4. **BIFF12 格式**
   - Record type: varint 编码
   - Record size: varint 编码
   - Cell type: 0=Blank, 4=Real, 7=Isst, 9=Bool

### 参考资源

- Java 参考实现: ~/code/java/jxlsb
- 模板文件: tests/resources/template/demo_template.xlsb
- BIFF12 规范: MS-XLSB 文档