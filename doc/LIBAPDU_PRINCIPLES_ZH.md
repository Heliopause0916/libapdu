# libapdu 库原理解析

## 简介

libapdu 是一个轻量级的 C99 静态库，用于 APDU（Application Protocol Data Unit，应用协议数据单元）的编码与解码。该库从 OpenSC 项目中提取而来，保留了纯粹的 APDU 解析与组装逻辑，无外部依赖，仅依赖 C99 标准库。

本文档详细解析 libapdu 的核心原理，包括 APDU 协议基础、数据结构设计、API 函数实现细节、编解码流程以及错误处理机制。

---

## 目录

1. [APDU 基础概念](#1-apdu-基础概念)
   - [什么是 APDU](#11-什么是-apdu)
   - [命令 APDU 结构](#12-命令-apducommand-apdu--c-apdu)
   - [响应 APDU 结构](#13-响应-apduresponse-apdu--r-apdu)
   - [APDU 的四种 Case](#14-apdu-的四种-case情况)
2. [数据结构解析](#2-数据结构解析)
   - [apdu_t 结构体](#21-apdu_t-结构体)
   - [字段含义详解](#22-字段含义详解)
   - [关键常量定义](#23-关键常量定义)
3. [API 函数详解](#3-api-函数详解)
   - [apdu_get_length()](#31-apdu_get_length)
   - [apdu_encode()](#32-apdu_encode)
   - [apdu_alloc_and_encode()](#33-apdu_alloc_and_encode)
   - [apdu_decode()](#34-apdu_decode)
   - [apdu_set_response()](#35-apdu_set_response)
4. [编解码流程](#4-编解码流程)
   - [命令 APDU 解码流程](#41-命令-apdu-解码流程)
   - [APDU 编码流程](#42-apdu-编码流程)
   - [短 APDU vs 扩展 APDU](#43-短-apdu-vs-扩展-apdu)
5. [错误处理](#5-错误处理)
   - [错误码定义](#51-错误码定义)
   - [各函数错误返回场景](#52-各函数错误返回场景)
6. [关键实现细节](#6-关键实现细节)
   - [字节序处理](#61-字节序处理)
   - [长度字段处理](#62-长度字段处理)
   - [T0 vs T1 协议差异](#63-t0-vs-t1-协议差异)
   - [边界条件处理](#64-边界条件处理)
7. [内存语义与调用者责任](#7-内存语义与调用者责任)
8. [总结](#8-总结)

---

## 1. APDU 基础概念

### 1.1 什么是 APDU

**APDU（Application Protocol Data Unit，应用协议数据单元）** 是智能卡与读卡器之间通信的标准协议格式，定义于 ISO/IEC 7816-4 标准。

APDU 定义了两种通信方向的数据格式：
- **命令 APDU（C-APDU）**：终端发送给智能卡的命令
- **响应 APDU（R-APDU）**：智能卡返回给终端的响应

### 1.2 命令 APDU（Command APDU / C-APDU）

命令 APDU 由终端发送给智能卡，结构如下：

```
+------+-----+----+----+-----+----------+-----+
| CLA  | INS | P1 | P2 | Lc  | Data     | Le  |
+------+-----+----+----+-----+----------+-----+
| 1字节| 1字节|1字节|1字节|0-3字节|0-65535字节|0-3字节|
+------+-----+----+----+-----+----------+-----+
```

| 字段 | 说明 |
|------|------|
| **CLA** | 类别字节（Class Byte），标识命令的类别和特性 |
| **INS** | 指令字节（Instruction Byte），标识具体的指令代码 |
| **P1, P2** | 参数字节（Parameter Bytes），提供指令的附加参数 |
| **Lc** | 命令数据长度，Data 字段的字节数 |
| **Data** | 命令数据，实际传输的数据内容 |
| **Le** | 期望响应长度，指示期望返回的最大字节数 |

### 1.3 响应 APDU（Response APDU / R-APDU）

响应 APDU 由智能卡返回给终端，结构如下：

```
+------------------+-------+-------+
| Data (optional)  | SW1   | SW2   |
+------------------+-------+-------+
| 0-N 字节         | 1字节 | 1字节 |
+------------------+-------+-------+
```

| 字段 | 说明 |
|------|------|
| **Data** | 可选的响应数据 |
| **SW1, SW2** | 状态字（Status Words），表示命令执行结果 |

常见状态字：
- `0x9000`：命令执行成功
- `0x6100`：仍有数据可读取（需使用 GET RESPONSE）
- `0x6A82`：未找到文件

### 1.4 APDU 的四种 Case（情况）

根据 Lc 和 Le 的存在与否，命令 APDU 分为四种情况：

| Case | Lc | Le | 说明 |
|------|----|----|------|
| **Case 1** | 无 | 无 | 无命令数据，无期望响应 |
| **Case 2** | 无 | 有 | 无命令数据，有期望响应 |
| **Case 3** | 有 | 无 | 有命令数据，无期望响应 |
| **Case 4** | 有 | 有 | 有命令数据，有期望响应 |

每种 Case 又分为**短格式（Short）**和**扩展格式（Extended）**：
- 短格式：Lc/Le 使用 1 字节表示
- 扩展格式：Lc/Le 使用 2 或 3 字节表示

---

## 2. 数据结构解析

### 2.1 `apdu_t` 结构体

`apdu_t` 是库的核心数据结构，用于表示一个 APDU：

```c
typedef struct apdu {
    int      cse;               /* APDU case (APDU_CASE_*) */
    u8       cla, ins, p1, p2;  /* CLA, INS, P1, P2 bytes */
    size_t   lc, le;            /* Lc and Le */
    const u8 *data;             /* Command data pointer */
    size_t   datalen;           /* Command data length */
    u8       *resp;             /* Response buffer */
    size_t   resplen;           /* In: buffer size; Out: actual length */
    u8       control;           /* Reader control flag (legacy) */
    unsigned sw1, sw2;          /* Status words */
    u8       mac[8];            /* MAC for secure messaging (legacy) */
    size_t   mac_len;           /* MAC length (legacy) */
    unsigned long flags;        /* APDU flags (legacy) */
    struct apdu *next;          /* Linked list for chaining (legacy) */
} apdu_t;
```

### 2.2 字段含义详解

| 字段 | 类型 | 用途 | 编解码作用 |
|------|------|------|-----------|
| `cse` | `int` | APDU 情况类型 | **解码时自动判断**；编码时需预先设置 |
| `cla`, `ins`, `p1`, `p2` | `u8` | 命令头 4 字节 | 编解码时直接映射 |
| `lc` | `size_t` | 命令数据长度 | 解码时解析 Lc 字段；编码时决定 Lc 字段格式 |
| `le` | `size_t` | 期望响应长度 | 解码时解析 Le 字段；编码时决定 Le 字段格式 |
| `data` | `const u8*` | 命令数据指针 | 解码时指向输入缓冲区内部；编码时读取源数据 |
| `datalen` | `size_t` | 数据长度 | 解码时设置；实际与 `lc` 相同（冗余字段）|
| `resp` | `u8*` | 响应数据缓冲区 | 由调用者分配；`apdu_set_response()` 写入 |
| `resplen` | `size_t` | 缓冲区大小/实际长度 | **双向语义**：输入时表示缓冲区大小，输出时表示实际数据长度 |
| `control` | `u8` | 读卡器控制标志 | **遗留字段**，当前未使用 |
| `sw1`, `sw2` | `unsigned` | 状态字 | `apdu_set_response()` 设置 |
| `mac[8]` | `u8[8]` | 安全消息 MAC | **遗留字段**，当前未使用 |
| `mac_len` | `size_t` | MAC 长度 | **遗留字段**，当前未使用 |
| `flags` | `unsigned long` | APDU 标志 | **遗留字段**，当前未使用 |
| `next` | `struct apdu*` | 链表指针 | **遗留字段**，用于命令链接 |

### 2.3 关键常量定义

**APDU Case 类型常量：**

```c
#define APDU_CASE_NONE       0x00   // 未确定/无效
#define APDU_CASE_1          0x01   // Case 1
#define APDU_CASE_2_SHORT    0x02   // Case 2 短格式
#define APDU_CASE_3_SHORT    0x03   // Case 3 短格式
#define APDU_CASE_4_SHORT    0x04   // Case 4 短格式

#define APDU_SHORT_MASK      0x0f   // 短格式掩码
#define APDU_EXT             0x10   // 扩展格式标志

#define APDU_CASE_2_EXT      (APDU_CASE_2_SHORT | APDU_EXT)  // 0x12
#define APDU_CASE_3_EXT      (APDU_CASE_3_SHORT | APDU_EXT)  // 0x13
#define APDU_CASE_4_EXT      (APDU_CASE_4_SHORT | APDU_EXT)  // 0x14
```

**缓冲区大小限制：**

```c
#define APDU_MAX_SHORT_BUFFER_SIZE   261   /* 最大短 APDU 长度 */
#define APDU_MAX_SHORT_DATA_SIZE     0xFF  /* 短 APDU 最大数据长度：255 */
#define APDU_MAX_SHORT_RESP_SIZE     (0xFF + 1)  /* 短 APDU 最大响应长度：256 */

#define APDU_MAX_EXT_BUFFER_SIZE     65538 /* 最大扩展 APDU 长度 */
#define APDU_MAX_EXT_DATA_SIZE       0xFFFF      /* 扩展 APDU 最大数据长度：65535 */
#define APDU_MAX_EXT_RESP_SIZE       (0xFFFF + 1) /* 扩展 APDU 最大响应长度：65536 */
```

**协议类型常量：**

```c
#define APDU_PROTO_T0   0x00   // T=0 协议（字节传输）
#define APDU_PROTO_T1   0x01   // T=1 协议（块传输）
```

---

## 3. API 函数详解

libapdu 提供 5 个核心 API 函数：

### 3.1 `apdu_get_length()`

**函数签名：**

```c
size_t apdu_get_length(const apdu_t *apdu, unsigned int proto);
```

**功能说明：**

计算 APDU 编码后的字节长度。用于预先确定编码所需的缓冲区大小。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `apdu` | APDU 结构体指针 |
| `proto` | 协议类型（`APDU_PROTO_T0` 或 `APDU_PROTO_T1`）|

**返回值：**

- 成功：返回编码后的字节长度
- 失败：返回 `0`（表示无效输入或未知的 `cse` 值）

**长度计算规则：**

| Case | T0 协议 | T1 协议 |
|------|---------|---------|
| Case 1 | 5 字节（+1 字节 0x00）| 4 字节 |
| Case 2 Short | 5 字节 | 5 字节 |
| Case 2 Ext | 5 字节 | 7 字节 |
| Case 3 Short | 5 + Lc 字节 | 5 + Lc 字节 |
| Case 3 Ext | 5 + Lc 字节 | 7 + Lc 字节 |
| Case 4 Short | 5 + Lc 字节 | 6 + Lc 字节 |
| Case 4 Ext | 5 + Lc 字节 | 9 + Lc 字节 |

**使用示例：**

```c
apdu_t apdu;
apdu.cse = APDU_CASE_3_SHORT;
apdu.lc = 16;

size_t len = apdu_get_length(&apdu, APDU_PROTO_T1);
// len = 5 + 16 = 21
```

### 3.2 `apdu_encode()`

**函数签名：**

```c
int apdu_encode(const apdu_t *apdu, unsigned int proto, u8 *out, size_t outlen);
```

**功能说明：**

将 APDU 结构体编码为字节序列。调用者需预先分配输出缓冲区。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `apdu` | 待编码的 APDU 结构体 |
| `proto` | 协议类型 |
| `out` | 输出缓冲区 |
| `outlen` | 输出缓冲区大小 |

**返回值：**

| 返回值 | 说明 |
|--------|------|
| `APDU_SUCCESS`（0）| 编码成功 |
| `APDU_ERROR_INVALID_ARGUMENTS`（-1300）| 缓冲区过小或参数无效 |

**使用示例：**

```c
apdu_t apdu = {
    .cse = APDU_CASE_3_SHORT,
    .cla = 0x00,
    .ins = 0xA4,
    .p1 = 0x04,
    .p2 = 0x00,
    .lc = 2,
    .data = (const u8 *)"\x3F\x00"
};

size_t len = apdu_get_length(&apdu, APDU_PROTO_T1);
u8 *buf = malloc(len);

int ret = apdu_encode(&apdu, APDU_PROTO_T1, buf, len);
// buf = {0x00, 0xA4, 0x04, 0x00, 0x02, 0x3F, 0x00}

free(buf);
```

### 3.3 `apdu_alloc_and_encode()`

**函数签名：**

```c
int apdu_alloc_and_encode(const apdu_t *apdu, u8 **buf, size_t *len, unsigned int proto);
```

**功能说明：**

分配内存并编码 APDU。内部自动分配所需缓冲区，简化调用流程。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `apdu` | 待编码的 APDU 结构体 |
| `buf` | 输出参数，指向分配的缓冲区指针 |
| `len` | 输出参数，编码后的长度 |
| `proto` | 协议类型 |

**返回值：**

| 返回值 | 说明 |
|--------|------|
| `APDU_SUCCESS` | 编码成功 |
| `APDU_ERROR_INVALID_ARGUMENTS` | 参数为 NULL |
| `APDU_ERROR_INTERNAL` | 内部错误（长度计算为 0 或编码失败）|
| `APDU_ERROR_OUT_OF_MEMORY` | 内存分配失败 |

**内存语义：**

- 函数内部通过 `malloc()` 分配内存
- **调用者必须在不再使用时调用 `free()` 释放**

**使用示例：**

```c
apdu_t apdu = { /* ... */ };
u8 *buf = NULL;
size_t len = 0;

int ret = apdu_alloc_and_encode(&apdu, &buf, &len, APDU_PROTO_T1);
if (ret == APDU_SUCCESS) {
    // 使用 buf...
    free(buf);  // 记得释放！
}
```

### 3.4 `apdu_decode()`

**函数签名：**

```c
int apdu_decode(const u8 *buf, size_t len, apdu_t *apdu);
```

**功能说明：**

将字节序列解码为 APDU 结构体。自动判断 APDU 的 Case 类型（短格式/扩展格式）。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `buf` | 输入字节序列 |
| `len` | 输入长度 |
| `apdu` | 输出的 APDU 结构体 |

**返回值：**

| 返回值 | 说明 |
|--------|------|
| `APDU_SUCCESS` | 解码成功 |
| `APDU_ERROR_INVALID_ARGUMENTS` | 参数为 NULL |
| `APDU_ERROR_INVALID_DATA` | 数据格式无效（长度不足、数据不完整等）|

**内存语义：**

- `apdu->data` 指向输入缓冲区内部位置（**非拷贝**）
- **调用者必须保持输入缓冲区有效，直到不再使用 `apdu`**

**使用示例：**

```c
u8 raw[] = {0x00, 0xA4, 0x04, 0x00, 0x02, 0x3F, 0x00};
apdu_t apdu;

int ret = apdu_decode(raw, sizeof(raw), &apdu);
if (ret == APDU_SUCCESS) {
    printf("Case: %d, LC: %zu\n", apdu.cse, apdu.lc);
    // apdu.data 指向 raw + 5，必须保持 raw 有效
}
```

### 3.5 `apdu_set_response()`

**函数签名：**

```c
int apdu_set_response(apdu_t *apdu, const u8 *buf, size_t len);
```

**功能说明：**

设置响应数据和状态字。将原始响应数据解析到 APDU 结构体中。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `apdu` | 要更新的 APDU 结构体 |
| `buf` | 响应数据（最后 2 字节为 SW1, SW2）|
| `len` | 响应数据长度（必须 >= 2）|

**返回值：**

| 返回值 | 说明 |
|--------|------|
| `APDU_SUCCESS` | 设置成功 |
| `APDU_ERROR_INTERNAL` | `len < 2` |

**使用示例：**

```c
apdu_t apdu = { /* ... */ };
u8 response[] = {0x3F, 0x00, 0x90, 0x00};  // 数据 + SW1 + SW2

int ret = apdu_set_response(&apdu, response, sizeof(response));
// apdu.sw1 = 0x90, apdu.sw2 = 0x00
// apdu.resplen = 2 (数据长度)
```

---

## 4. 编解码流程

### 4.1 命令 APDU 解码流程

解码过程通过解析字节序列，自动判断 APDU 的 Case 类型：

```
                    +------------------+
                    | 输入字节序列 buf |
                    +--------+---------+
                             |
                             v
                    +--------+---------+
                    | 长度检查 >= 4 ?  |
                    +--------+---------+
                             | 是
                             v
                    +--------+---------+
                    | 解析 CLA INS P1 P2 |
                    +--------+---------+
                             |
                             v
                    +--------+---------+
                    | 剩余长度 == 0 ?  |
                    +--------+---------+
                      | 是            | 否
                      v               v
              +-------+------+  +-----+-----+
              | Case 1       |  | 检查首字节 |
              +--------------+  +-----+-----+
                                       |
                       +---------------+---------------+
                       | 0x00 且 len >= 3            | 其他
                       v                             v
               +-------+-------+            +-------+-------+
               | 扩展 APDU     |            | 短 APDU       |
               +-------+-------+            +-------+-------+
                       |                             |
           +-----------+-----------+     +-----------+-----------+
           | 解析 3 字节长度字段   |     | 解析 1 字节长度字段   |
           +-----------+-----------+     +-----------+-----------+
                       |                             |
           +-----------+-----------+     +-----------+-----------+
           | 判断 Case 2/3/4 Ext   |     | 判断 Case 2/3/4 Short |
           +-----------------------+     +-----------------------+
```

**关键判断逻辑：**

1. **长度 < 4**：无效数据
2. **长度 = 4**：Case 1
3. **首字节 = 0x00 且 len >= 3**：扩展 APDU
4. **其他**：短 APDU

### 4.2 APDU 编码流程

编码过程根据 `cse` 字段决定输出格式：

```
                    +------------------+
                    | apdu_t 结构体    |
                    +--------+---------+
                             |
                             v
                    +--------+---------+
                    | 写入 CLA INS P1 P2 |
                    +--------+---------+
                             |
                             v
                    +--------+---------+
                    | 根据 cse 分发    |
                    +--------+---------+
                             |
       +----------+----------+----------+----------+
       |          |          |          |          |
       v          v          v          v          v
   Case 1    Case 2     Case 3     Case 4     其他
       |          |          |          |          |
       |          |          |          |          |
       v          v          v          v          v
   T0: +0x00   Short:      Lc +       Lc +      错误
   T1: 无     Ext:        Data       Data
             根据协议    根据协议    +Le
```

### 4.3 短 APDU vs 扩展 APDU

| 特性 | 短 APDU | 扩展 APDU |
|------|---------|-----------|
| **Lc 字节数** | 1 字节 | 3 字节（0x00 + 2 字节长度）|
| **Le 字节数** | 1 字节 | 2 或 3 字节 |
| **Lc 最大值** | 255 | 65535 |
| **Le 最大值** | 256 | 65536 |
| **识别方式** | 首字节非 0x00（或长度判断）| 首字节为 0x00 |

**扩展 APDU 的 Lc 编码格式：**

```
+------+------+------+
| 0x00 | Lc_H | Lc_L |
+------+------+------+
  1字节  高字节  低字节
```

---

## 5. 错误处理

### 5.1 错误码定义

```c
#define APDU_SUCCESS                 0      // 成功
#define APDU_ERROR_INVALID_ARGUMENTS -1300  // 无效参数
#define APDU_ERROR_INVALID_DATA      -1305  // 无效数据
#define APDU_ERROR_INTERNAL          -1400  // 内部错误
#define APDU_ERROR_OUT_OF_MEMORY     -1404  // 内存不足
```

**错误码来源：** 这些错误码继承自 OpenSC 项目，保持了与原项目的兼容性。

### 5.2 各函数错误返回场景

| 函数 | 错误码 | 触发条件 |
|------|--------|----------|
| `apdu_get_length` | `0` | 未知的 `cse` 值 |
| `apdu_encode` | `INVALID_ARGUMENTS` | `out == NULL` |
| `apdu_encode` | `INVALID_ARGUMENTS` | 缓冲区过小 |
| `apdu_encode` | `INVALID_ARGUMENTS` | T0 协议且 `lc > 255`（Case 3 Ext）|
| `apdu_alloc_and_encode` | `INVALID_ARGUMENTS` | 任一参数为 NULL |
| `apdu_alloc_and_encode` | `INTERNAL` | `apdu_get_length()` 返回 0 |
| `apdu_alloc_and_encode` | `OUT_OF_MEMORY` | `malloc()` 失败 |
| `apdu_alloc_and_encode` | `INTERNAL` | `apdu_encode()` 失败 |
| `apdu_decode` | `INVALID_ARGUMENTS` | `buf` 或 `apdu` 为 NULL |
| `apdu_decode` | `INVALID_DATA` | `len < 4` |
| `apdu_decode` | `INVALID_DATA` | 数据长度不匹配 Lc |
| `apdu_decode` | `INVALID_DATA` | 扩展 APDU 数据不足 |
| `apdu_decode` | `INVALID_DATA` | 解析后仍有剩余数据 |
| `apdu_set_response` | `INTERNAL` | `len < 2` |

---

## 6. 关键实现细节

### 6.1 字节序处理

库使用**大端序（Big-Endian）**处理多字节长度字段，符合 ISO 7816 标准：

**编码时（高字节在前）：**

```c
*p++ = (u8)(apdu->le >> 8);  // 先写高字节
*p   = (u8)apdu->le;          // 后写低字节
```

**解码时（高字节在前）：**

```c
apdu->le  = (size_t)(*p++) << 8;  // 高字节左移 8 位
apdu->le += *p++;                  // 加上低字节
```

### 6.2 长度字段处理

**短格式：**

- Lc：1 字节，范围 1-255
- Le：1 字节，范围 0-256（`0x00` 表示 256）

**扩展格式：**

- Lc：3 字节（`0x00` + 2 字节长度），范围 1-65535
- Le：2 字节（跟随 Lc 时）或 3 字节（无 Lc 时），范围 0-65536

**Le = 0 的特殊含义：**

在 APDU 协议中，Le 字段为 0 表示期望最大长度响应：

```c
// 短 APDU：Le = 0 表示期望 256 字节响应
if (apdu->le == 0)
    apdu->le = 0xFF + 1;  // 即 256

// 扩展 APDU：Le = 0 表示期望 65536 字节响应
if (apdu->le == 0)
    apdu->le = 0xFFFF + 1;  // 即 65536
```

### 6.3 T0 vs T1 协议差异

T=0 和 T=1 是智能卡通信的两种传输协议，对 APDU 编码有不同要求：

| 场景 | T=0 协议 | T=1 协议 |
|------|---------|---------|
| **Case 1** | 需要追加 0x00 字节 | 无额外字节 |
| **Case 2 Ext** | 简化为 1 字节 Le | 使用 3 字节（0x00 + Le）|
| **Case 3 Ext** | 不支持（Lc > 255 需用 ENVELOPE）| 支持 3 字节 Lc |
| **Case 4 Short** | 不编码 Le（用过程字节获取响应）| 编码 1 字节 Le |
| **Case 4 Ext** | 简化为 Case 3 格式 | 完整编码 3 字节 Lc + 2 字节 Le |

**T=0 协议的特殊处理：**

```c
case APDU_CASE_1:
    if (proto == APDU_PROTO_T0)
        *p = (u8)0x00;  // T0 需要额外 0x00 字节
    break;
```

### 6.4 边界条件处理

库对输入数据进行严格的边界检查：

**最小长度检查：**

```c
if (len < 4)
    return APDU_ERROR_INVALID_DATA;
```

**数据长度一致性检查：**

```c
if (len < apdu->lc)
    return APDU_ERROR_INVALID_DATA;
```

**完整性检查（解析后无剩余数据）：**

```c
if (len)  // 解析后仍有剩余数据
    return APDU_ERROR_INVALID_DATA;
```

---

## 7. 内存语义与调用者责任

libapdu 的 API 设计遵循明确的内存管理规则：

| 函数 | 内存分配 | 调用者责任 |
|------|----------|-----------|
| `apdu_decode()` | 无分配 | 保持输入缓冲区有效，直到不再使用 `apdu` |
| `apdu_encode()` | 无分配 | 预先分配足够大的输出缓冲区 |
| `apdu_alloc_and_encode()` | 内部 `malloc()` | 使用后调用 `free()` 释放返回的缓冲区 |
| `apdu_get_length()` | 无分配 | 无 |
| `apdu_set_response()` | 无分配 | 保持响应缓冲区有效 |

**关键注意事项：**

1. **`apdu_decode()` 不拷贝数据**：`apdu->data` 直接指向输入缓冲区内部位置。如果在解码后修改或释放输入缓冲区，将导致 `apdu->data` 成为悬空指针。

2. **`apdu_alloc_and_encode()` 需要手动释放**：该函数使用 `malloc()` 分配内存，调用者必须负责释放。

3. **`resplen` 的双向语义**：在设置响应前，`resplen` 表示缓冲区大小；在设置响应后，表示实际返回的数据长度。

---

## 8. 总结

libapdu 是一个精简、高效的 APDU 编解码库，具有以下核心特点：

### 设计特点

1. **纯 C99 实现**：无外部依赖，仅依赖标准库，易于集成
2. **完整的 Case 支持**：支持 ISO 7816 定义的 4 种 APDU Case，包括短格式和扩展格式
3. **协议适配**：区分 T=0 和 T=1 协议的差异处理
4. **内存语义清晰**：明确区分调用者分配和库分配的场景

### API 设计

| API | 功能 | 典型使用场景 |
|-----|------|-------------|
| `apdu_get_length()` | 计算编码长度 | 预分配缓冲区 |
| `apdu_encode()` | 编码到调用者缓冲区 | 已知缓冲区大小 |
| `apdu_alloc_and_encode()` | 分配并编码 | 简化调用流程 |
| `apdu_decode()` | 解码到结构体 | 解析接收到的命令 |
| `apdu_set_response()` | 设置响应数据 | 处理响应 |

### 遗留兼容

库保留了 OpenSC 的部分兼容字段（`mac`、`control`、`next` 等），但这些字段在当前 API 中未实际使用，可安全忽略。

---

## 参考资料

- ISO/IEC 7816-4: Organization, security and commands for interchange
- [OpenSC Project](https://github.com/OpenSC/OpenSC)