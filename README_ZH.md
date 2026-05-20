# libapdu

一个轻量级、平台无关的 C 语言 APDU（Application Protocol Data Unit）解析与组装库，提取自 [OpenSC](https://github.com/OpenSC/OpenSC)。

## 特性

- **纯 APDU 编解码** — 不依赖任何智能卡读卡器或传输层
- **支持所有 ISO 7816-4 APDU 类型**：
  - Case 1：无数据（仅 CLA, INS, P1, P2）
  - Case 2 Short/Extended：仅接收数据
  - Case 3 Short/Extended：仅发送数据
  - Case 4 Short/Extended：发送并接收数据
- **T0 和 T1 协议编码** — 正确处理两种协议的长度字节格式
- **短帧与扩展帧 APDU** — 解析时自动检测，编码时支持手动或自动选择
- **无外部依赖** — 仅需 C99 标准库
- **体积小巧** — 编译后的静态库约 13 KB

## API 概览

```c
#include <libapdu/apdu.h>

// 计算 APDU 编码后的字节长度
size_t apdu_get_length(const apdu_t *apdu, unsigned int proto);

// 将 APDU 结构编码为字节流
int apdu_encode(const apdu_t *apdu, unsigned int proto, u8 *out, size_t outlen);

// 分配缓冲区并编码 APDU（调用者需 free()）
int apdu_alloc_and_encode(const apdu_t *apdu, u8 **buf, size_t *len, unsigned int proto);

// 将字节流解码为 APDU 结构
int apdu_decode(const u8 *buf, size_t len, apdu_t *apdu);

// 设置响应数据及状态字（SW1, SW2）
int apdu_set_response(apdu_t *apdu, const u8 *buf, size_t len);
```

## 示例

```c
#include <stdio.h>
#include <libapdu/apdu.h>

int main(void) {
    /* 构建 SELECT FILE 指令：00 A4 04 00 02 3F 00 */
    apdu_t apdu;
    u8     encoded[64];
    size_t len;
    u8     file_id[] = { 0x3F, 0x00 };

    apdu.cse     = APDU_CASE_4_SHORT;
    apdu.cla     = 0x00;
    apdu.ins     = 0xA4;
    apdu.p1      = 0x04;
    apdu.p2      = 0x00;
    apdu.lc      = sizeof(file_id);
    apdu.le      = 0;
    apdu.data    = file_id;
    apdu.datalen = sizeof(file_id);

    len = apdu_get_length(&apdu, APDU_PROTO_T0);
    apdu_encode(&apdu, APDU_PROTO_T0, encoded, sizeof(encoded));

    /* encoded[0..len-1] = { 0x00, 0xA4, 0x04, 0x00, 0x02, 0x3F, 0x00 } */

    /* 解码回结构体 */
    apdu_t decoded;
    apdu_decode(encoded, len, &decoded);
    printf("CLA=%02X, INS=%02X, P1=%02X, P2=%02X\n",
           decoded.cla, decoded.ins, decoded.p1, decoded.p2);

    return 0;
}
```

## 编译构建

### 使用 CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 使用 MSVC（无 CMake）

```batch
cl /c /Iinclude src/apdu.c /Fo:apdu.obj
lib apdu.obj /OUT:libapdu.lib
```

### 使用 GCC

```bash
gcc -c -Iinclude src/apdu.c -o apdu.o
ar rcs libapdu.a apdu.o
```

## 项目集成

在你的 CMakeLists.txt 中添加：

```cmake
# 方式 A：FetchContent
include(FetchContent)
FetchContent_Declare(libapdu
    GIT_REPOSITORY https://github.com/your-org/libapdu.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(libapdu)

target_link_libraries(your_target PRIVATE libapdu::libapdu)

# 方式 B：find_package（安装后）
find_package(libapdu REQUIRED)
target_link_libraries(your_target PRIVATE libapdu::libapdu)
```

## 来源

本库提取自 [OpenSC](https://github.com/OpenSC/OpenSC) 的 `src/libopensc/apdu.c`。原始代码由 Nils Larsch 编写，采用 LGPL 2.1+ 许可证。

提取过程中移除了所有 OpenSC 特有的依赖（`sc_card_t`、`sc_context_t`、日志系统、安全消息、读卡器传输层等），仅保留纯 APDU 解析与组装的逻辑。

## 许可证

LGPL 2.1+ — 详见 [LICENSE](LICENSE) 文件。
