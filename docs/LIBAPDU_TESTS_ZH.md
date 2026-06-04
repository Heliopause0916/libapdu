# libapdu 测试套件文档

## 简介

libapdu 包含一套全面的测试套件，位于 `tests/test_apdu.c`。测试套件使用纯 C99 编写，无外部测试框架依赖，对所有 8 个 API 函数在各种 APDU Case 和协议场景下进行了详尽的测试覆盖。

本文档详细分析测试套件的结构、测试函数和覆盖点。

---

## 目录

- [libapdu 测试套件文档](#libapdu-测试套件文档)
  - [简介](#简介)
  - [目录](#目录)
  - [1. 测试框架概述](#1-测试框架概述)
    - [轻量级设计](#轻量级设计)
    - [框架特性](#框架特性)
    - [辅助函数](#辅助函数)
  - [2. 测试文件位置](#2-测试文件位置)
  - [3. 测试覆盖概览](#3-测试覆盖概览)
    - [API 覆盖情况](#api-覆盖情况)
    - [覆盖类别](#覆盖类别)
  - [4. 测试函数详解](#4-测试函数详解)
    - [4.1 `test_get_length()`](#41-test_get_length)
    - [4.2 `test_encode()`](#42-test_encode)
    - [4.3 `test_alloc_and_encode()`](#43-test_alloc_and_encode)
    - [4.4 `test_decode()`](#44-test_decode)
    - [4.5 `test_set_response()`](#45-test_set_response)
    - [4.6 `test_get_response_length()`](#46-test_get_response_length)
    - [4.7 `test_encode_response()`](#47-test_encode_response)
    - [4.8 `test_alloc_and_encode_response()`](#48-test_alloc_and_encode_response)
  - [5. 运行测试](#5-运行测试)
    - [构建与运行](#构建与运行)
    - [预期输出](#预期输出)
    - [退出码](#退出码)
  - [参考资料](#参考资料)

---

## 1. 测试框架概述

### 轻量级设计

测试框架直接在测试文件中实现，无任何外部依赖：

```c
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name)       do { printf("  - %s ... ", name); } while (0)
#define TEST_PASS()            do { tests_passed++; printf("PASS\n"); } while (0)
#define TEST_FAIL(msg)         do { tests_failed++; printf("FAIL: %s\n", msg); } while (0)
#define TEST_ASSERT(cond, msg) do {                                    \
        if (!(cond)) { TEST_FAIL(msg); return; }                       \
    } while (0)
```

### 框架特性

| 组件 | 描述 |
|------|------|
| `tests_passed` | 全局通过的测试计数器 |
| `tests_failed` | 全局失败的测试计数器 |
| `TEST_START(name)` | 打印带格式的测试名称 |
| `TEST_PASS()` | 增加通过计数器并打印 "PASS" |
| `TEST_FAIL(msg)` | 增加失败计数器并打印失败消息 |
| `TEST_ASSERT(cond, msg)` | 验证条件；失败时记录并提前返回 |

### 辅助函数

```c
static void setup_apdu(apdu_t *apdu, int cse, u8 cla, u8 ins, u8 p1, u8 p2,
                       size_t lc, const u8 *data, size_t le);
static int u8cmp(const u8 *a, const u8 *b, size_t n);
```

- `setup_apdu()`：用指定参数初始化 APDU 结构体
- `u8cmp()`：使用 `memcmp()` 比较字节数组

---

## 2. 测试文件位置

| 项目 | 路径 |
|------|------|
| 测试源文件 | `tests/test_apdu.c` |
| 测试代码行数 | 1004 行 |
| 测试函数数量 | 8 个函数 |
| 总测试用例数 | 92 个断言 |

---

## 3. 测试覆盖概览

### API 覆盖情况

| API 函数 | 测试函数 | 测试数量 |
|----------|----------|----------|
| `apdu_get_length()` | `test_get_length()` | 15 |
| `apdu_encode()` | `test_encode()` | 18 |
| `apdu_alloc_and_encode()` | `test_alloc_and_encode()` | 8 |
| `apdu_decode()` | `test_decode()` | 20 |
| `apdu_set_response()` | `test_set_response()` | 8 |
| `apdu_get_response_length()` | `test_get_response_length()` | 8 |
| `apdu_encode_response()` | `test_encode_response()` | 7 |
| `apdu_alloc_and_encode_response()` | `test_alloc_and_encode_response()` | 8 |
| **总计** | | **92** |

### 覆盖类别

| 类别 | 是否覆盖 |
|------|----------|
| APDU Case (1, 2, 3, 4) | ✅ |
| 短格式 | ✅ |
| 扩展格式 | ✅ |
| T=0 协议 | ✅ |
| T=1 协议 | ✅ |
| 错误处理 | ✅ |
| 边界条件 | ✅ |
| NULL 参数验证 | ✅ |
| 内存分配 | ✅ |

---

## 4. 测试函数详解

### 4.1 `test_get_length()`

**测试目的：** 测试 `apdu_get_length()` 计算编码字节长度。

**测试数量：** 15

**覆盖点：**

| 测试名称 | 描述 | 协议 |
|----------|------|------|
| Case1 T0 -> 5 | Case 1 使用 T=0 协议返回 5 字节 | T0 |
| Case1 T1 -> 4 | Case 1 使用 T=1 协议返回 4 字节 | T1 |
| Case2Short Le=255 -> 5 | Case 2 短格式 Le=255 返回 5 字节 | - |
| Case2Ext T0 -> 5 | Case 2 扩展格式 T=0 协议 | T0 |
| Case2Ext T1 -> 7 | Case 2 扩展格式 T=1 协议返回 7 字节 | T1 |
| Case3Short lc=10 -> 15 | Case 3 短格式带 10 字节数据 | - |
| Case3Ext T0 lc=10 -> 15 | Case 3 扩展格式 T=0 协议 | T0 |
| Case3Ext T1 lc=10 -> 17 | Case 3 扩展格式 T=1 协议 | T1 |
| Case4Short lc=5 T0 -> 10 | Case 4 短格式 T=0 协议 | T0 |
| Case4Short lc=5 T1 -> 11 | Case 4 短格式 T=1 协议 | T1 |
| Case4Ext lc=5 T0 -> 10 | Case 4 扩展格式 T=0 协议 | T0 |
| Case4Ext lc=5 T1 -> 14 | Case 4 扩展格式 T=1 协议 | T1 |
| Invalid cse -> 0 | 无效 `cse` 值返回 0 | - |
| Unknown cse 0x99 -> 0 | 未知 `cse` 值返回 0 | - |

**关键验证：**

- 所有四种 APDU Case (1, 2, 3, 4)
- 短格式和扩展格式
- T=0 与 T=1 协议差异
- 无效/未知 `cse` 处理

---

### 4.2 `test_encode()`

**测试目的：** 测试 `apdu_encode()` 将 APDU 结构体编码为字节序列。

**测试数量：** 18

**覆盖点：**

| 测试名称 | 描述 | 预期结果 |
|----------|------|----------|
| Case1 T0 encode | Case 1 T=0 协议 | `{0x00, 0xA4, 0x00, 0x00, 0x00}` |
| Case1 T1 encode | Case 1 T=1 协议 | `{0x00, 0xA4, 0x00, 0x00}` |
| Case2Short Le=255 encode | Case 2 短格式 Le=255 | `{0x00, 0xC0, 0x00, 0x00, 0xFF}` |
| Case2Ext T0 encode | Case 2 扩展格式 T=0 | `{0x00, 0xC0, 0x00, 0x00, 0x00}` |
| Case2Ext T1 encode | Case 2 扩展格式 T=1 | `{0x00, 0xC0, 0x00, 0x00, 0x00, 0x01, 0x00}` |
| Case3Short lc=3 encode | Case 3 短格式 3 字节数据 | `{0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33}` |
| Case3Ext T0 encode | Case 3 扩展格式 T=0 | `{0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33}` |
| Case3Ext T1 encode | Case 3 扩展格式 T=1 | `{0x00, 0xDA, 0x00, 0x00, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33}` |
| Case4Short lc=2 Le=128 T0 encode | Case 4 短格式 T=0 | `{0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB}` |
| Case4Short lc=2 Le=128 T1 encode | Case 4 短格式 T=1 | `{0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB, 0x80}` |
| Case4Ext lc=2 Le=256 T0 encode | Case 4 扩展格式 T=0 | `{0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB}` |
| Case4Ext lc=2 Le=256 T1 encode | Case 4 扩展格式 T=1 | `{0x00, 0xC0, 0x00, 0x01, 0x00, 0x00, 0x02, 0xAA, 0xBB, 0x01, 0x00}` |
| Buffer too small | 缓冲区不足 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL out buffer | NULL 输出缓冲区 | `APDU_ERROR_INVALID_ARGUMENTS` |

**关键验证：**

- 所有 Case 的正确字节序列生成
- T=0 与 T=1 协议编码差异
- 缓冲区大小验证
- NULL 参数处理

---

### 4.3 `test_alloc_and_encode()`

**测试目的：** 测试 `apdu_alloc_and_encode()` 内存分配和编码功能。

**测试数量：** 8

**覆盖点：**

| 测试名称 | 描述 | 预期结果 |
|----------|------|----------|
| Alloc and encode Case3Short | 正常分配 | 有效缓冲区，正确编码 |
| Alloc and encode Case4Ext T1 | 扩展格式分配 | 有效缓冲区，正确编码 |
| NULL apdu returns INVALID_ARGUMENTS | NULL APDU 参数 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL buf returns INVALID_ARGUMENTS | NULL 缓冲区指针 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL len returns INVALID_ARGUMENTS | NULL 长度指针 | `APDU_ERROR_INVALID_ARGUMENTS` |
| Invalid cse returns INTERNAL | 无效 `cse` 值 | `APDU_ERROR_INTERNAL` |

**关键验证：**

- 内存分配成功
- 分配后编码正确
- 调用者负责 `free()` 释放缓冲区
- 所有 NULL 参数组合
- 无效 `cse` 处理

---

### 4.4 `test_decode()`

**测试目的：** 测试 `apdu_decode()` 将字节序列解码为 APDU 结构体。

**测试数量：** 20

**覆盖点：**

| 测试名称 | 描述 | 关键断言 |
|----------|------|----------|
| Decode Case1 | 4 字节输入 | `cse == APDU_CASE_1` |
| Decode Case2Short Le=0 -> 256 | Le=0 解释为 256 | `le == 256` |
| Decode Case2Short Le=128 | Le=0x80 | `le == 128` |
| Decode Case2Ext Le=256 | 扩展格式 | `le == 256` |
| Decode Case2Ext Le=0 -> 65536 | Le=0 解释为 65536 | `le == 65536` |
| Decode Case3Short lc=3 | Lc + 数据解析 | `lc == 3`, `data == buf + 5` |
| Decode Case3Ext lc=3 | 扩展 Lc + 数据 | `lc == 3`, `data == buf + 7` |
| Decode Case4Short lc=2 Le=128 | Lc + 数据 + Le | `lc == 2`, `le == 128` |
| Decode Case4Ext lc=2 Le=256 | 扩展完整格式 | `lc == 2`, `le == 256` |
| Input too short (3 bytes) | 长度 < 4 | `APDU_ERROR_INVALID_DATA` |
| Input empty (0 bytes) | 零长度 | `APDU_ERROR_INVALID_DATA` |
| NULL buf -> INVALID_ARGUMENTS | NULL 缓冲区 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL apdu -> INVALID_ARGUMENTS | NULL APDU 结构体 | `APDU_ERROR_INVALID_ARGUMENTS` |
| len < lc -> INVALID_DATA | 数据不足 | `APDU_ERROR_INVALID_DATA` |
| Trailing garbage -> INVALID_DATA | 解析后仍有剩余数据 | `APDU_ERROR_INVALID_DATA` |

**关键验证：**

- 所有四种 APDU Case 的短格式和扩展格式
- Le=0 → 256（短格式）/ 65536（扩展格式）转换
- `apdu->data` 指向输入缓冲区内部（非拷贝）
- NULL 参数处理
- 输入验证（长度、数据完整性）

---

### 4.5 `test_set_response()`

**测试目的：** 测试 `apdu_set_response()` 设置响应数据和状态字。

**测试数量：** 8

**覆盖点：**

| 测试名称 | 描述 | 关键断言 |
|----------|------|----------|
| Set response: 6 bytes, resplen=3 | 数据截断 | `sw1 == 'E'`, `sw2 == 'F'`, `resplen == 3` |
| Set response: resplen bigger than data | 完整数据复制 | `resplen == 3`（所有数据） |
| Set response: only SW (len=2) | 无响应数据 | `resplen == 0` |
| Set response: resp=NULL, resplen updated | NULL 缓冲区处理 | `resplen == 3`，无复制 |
| Set response: len=1 -> INTERNAL error | 长度不足 | `APDU_ERROR_INTERNAL` |
| Set response: len=0 -> INTERNAL error | 零长度 | `APDU_ERROR_INTERNAL` |

**关键验证：**

- 响应数据复制
- 状态字提取（最后 2 字节）
- `resplen` 截断行为
- NULL `resp` 缓冲区处理
- 最小长度验证（len >= 2）

---

### 4.6 `test_get_response_length()`

**测试目的：** 测试 `apdu_get_response_length()` 计算 R-APDU 编码长度。

**测试数量：** 8

**覆盖点：**

| 测试名称 | 描述 | 预期结果 |
|----------|------|----------|
| resplen=0 -> returns 2 | 无响应数据 | 2 字节（SW1+SW2） |
| resplen=10 -> returns 12 | 正常响应 | 12 字节 |
| resplen=255 -> returns 257 | 最大短响应 | 257 字节 |
| NULL apdu -> returns 0 | NULL 参数 | 0 |
| resplen=SIZE_MAX -> returns 0 | 溢出保护 | 0 |
| resplen=SIZE_MAX-2 -> returns SIZE_MAX | 边界情况 | SIZE_MAX |
| resplen=SIZE_MAX-1 -> returns 0 | 溢出情况 | 0 |

**关键验证：**

- 长度计算：`resplen + 2`
- NULL 参数处理
- 大 `resplen` 值的溢出保护

---

### 4.7 `test_encode_response()`

**测试目的：** 测试 `apdu_encode_response()` 将 R-APDU 编码到调用者缓冲区。

**测试数量：** 7

**覆盖点：**

| 测试名称 | 描述 | 预期结果 |
|----------|------|----------|
| Normal encode: resp data + SW | 正常编码 | `{0x01, 0x02, 0x03, 0x90, 0x00}` |
| Only SW: resplen=0 | 仅状态字 | `{0x61, 0x05}` |
| Buffer too small (need 7, give 5) | 缓冲区不足 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL apdu -> INVALID_ARGUMENTS | NULL APDU 参数 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL out -> INVALID_ARGUMENTS | NULL 输出缓冲区 | `APDU_ERROR_INVALID_ARGUMENTS` |
| resplen>0 but resp=NULL -> INVALID_ARGUMENTS | NULL 响应数据 | `APDU_ERROR_INVALID_ARGUMENTS` |

**关键验证：**

- 响应数据 + SW 编码
- 仅状态字编码
- 缓冲区大小验证
- NULL 参数处理
- `resp`/`resplen` 不一致性验证

---

### 4.8 `test_alloc_and_encode_response()`

**测试目的：** 测试 `apdu_alloc_and_encode_response()` 内存分配和 R-APDU 编码。

**测试数量：** 8

**覆盖点：**

| 测试名称 | 描述 | 预期结果 |
|----------|------|----------|
| Normal alloc and encode | 正常分配 | 有效缓冲区，正确编码 |
| Alloc and encode: only SW (no data) | 仅状态字 | `{0x61, 0x03}` |
| NULL apdu -> INVALID_ARGUMENTS | NULL APDU 参数 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL buf -> INVALID_ARGUMENTS | NULL 缓冲区指针 | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL len -> INVALID_ARGUMENTS | NULL 长度指针 | `APDU_ERROR_INVALID_ARGUMENTS` |
| resplen>0 but resp=NULL -> INTERNAL | 不一致状态 | `APDU_ERROR_INTERNAL` |

**关键验证：**

- 内存分配成功
- 分配后编码正确
- 调用者负责 `free()` 释放缓冲区
- 所有 NULL 参数组合
- `resp`/`resplen` 不一致性处理

---

## 5. 运行测试

### 构建与运行

```bash
# 创建构建目录
mkdir build && cd build

# 使用 CMake 配置
cmake ..

# 构建
cmake --build .

# 运行测试
./test_apdu      # Linux/macOS
test_apdu.exe    # Windows
```

### 预期输出

```
========================================
  libapdu API Test Suite
========================================

[apdu_get_length]
  - Case1 T0 -> 5 ... PASS
  - Case1 T1 -> 4 ... PASS
  ...
  - Unknown cse 0x99 -> 0 ... PASS

[apdu_encode]
  - Case1 T0 encode ... PASS
  - Case1 T1 encode ... PASS
  ...
  - NULL out buffer ... PASS

[apdu_alloc_and_encode]
  - Alloc and encode Case3Short ... PASS
  ...
  - Invalid cse returns INTERNAL ... PASS

[apdu_decode]
  - Decode Case1 ... PASS
  ...
  - Trailing garbage -> INVALID_DATA ... PASS

[apdu_set_response]
  ...
  - Set response: len=0 -> INTERNAL error ... PASS

[apdu_get_response_length]
  ...
  - resplen=SIZE_MAX-1 -> returns 0 (overflow) ... PASS

[apdu_encode_response]
  ...
  - resplen>0 but resp=NULL -> INVALID_ARGUMENTS ... PASS

[apdu_alloc_and_encode_response]
  ...
  - resplen>0 but resp=NULL -> INTERNAL (via encode_response) ... PASS

========================================
  Results: 92 passed, 0 failed
========================================
```

### 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 所有测试通过 |
| 1 | 一个或多个测试失败 |

---

## 参考资料

- [libapdu 库原理解析](./LIBAPDU_PRINCIPLES_ZH.md)
- ISO/IEC 7816-4: Organization, security and commands for interchange