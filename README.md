# LMMP
Lamina的高精度计算库。

提供接近GMP的任意精度计算能力，在可以支持汇编的情况下，LMMP和GMP通常具有相近的性能表现。

LMMP同时支持或计划支持包括但不限于如开方、阶乘、组合数、素性检验、质因数分解等复杂的高精度计算。很多功能正在开发中。

## 编译

LMMP 使用 CMake 构建，零外部依赖。当前仅支持 GNU/Clang 风格工具链：

+ Windows：MinGW GCC 或 GNU 驱动的 clang（不支持 MSVC / clang-cl）
+ Linux：GCC / Clang
+ macOS：Clang / GCC

动态库 liblmmp.so/liblmmp.dylib/lmmp.dll 由纯 C 编写；汇编为可选优化，使用 GAS/LLVM 兼容的 `.S` 语法（不再使用 NASM）。显式开启：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMMP_ASM=ARM64
cmake --build build -j
```

注意生成器：装有 Visual Studio 的 Windows 机器上，cmake 默认生成器可能为 Visual Studio（会被本库构建时直接拒绝）。Windows 下请显式 `-G Ninja`（推荐）或 `-G "MinGW Makefiles"`。`LMMP_ASM` / `TARGET_SYSTEM` 取值大小写不敏感。

支持情况：
+ x86_64 Linux / Windows(MinGW/Clang)：使用 `src/lmmp/lmmpn/arch/x64/*.S`
+ arm64 macOS / Linux：使用 `src/lmmp/lmmpn/arch/arm64/*.S`
+ 其他架构或 macOS x64：自动回退到 `src/lmmp/lmmpn/arch/generic/` 的 C 实现

在开启汇编时，所能够支持运行的架构及芯片如下：
+ X86-64：x86-64-v3指令集+adx指令集；支持芯片：Intel Broadwell及之后，AMD Excavator及之后（锐龙、霄龙）
+ ARM64：ARM64-v8.2-a指令集；支持芯片：Cortex-A55/A75/A77/A78, Neoverse-N1/E1, AWS Graviton2, Ampere Altra, 华为鲲鹏920, 富士通A64FX等。


## 主要配置选项

| 选项 | 取值 | 默认 | 说明 |
|---|---|---|---|
| `LMMP_ASM` | `AUTO` / `GENERIC` / `X64` / `ARM64` | `AUTO` | 汇编模式：自动选择、不使用汇编、x64 汇编、arm64 汇编 |
| `LMMP_COMPILER_DIR` | 目录路径 | 空 | 编译器所在目录（自动识别其中的 gcc/g++ 或 clang/clang++）；为空使用系统默认编译器 |
| `LMMP_LINKER_DIR` | 目录路径 | 空 | 链接器所在目录（按文件名识别：`ld.lld`/`lld` => lld，`ld.mold`/`mold` => mold，`ld`=> GNU ld）；为空使用工具链默认链接器 |
| `LMMP_OUTPUT_DIR` | 目录路径 | 空 | 统一产物目录：动态库、导入库与对外头文件平铺于此，附加构建（tests 等）开启后也输出于此；为空使用源内 `dist/lmmp/` 布局 |
| `LMMP_BUILD_TESTS` | `ON` / `OFF` | `OFF` | 是否构建单元测试（开启后与动态库输出到同一目录） |
| `LMMP_BUILD_BENCHMARKS` | `ON` / `OFF` | `OFF` | 是否构建性能基准（开启后与动态库输出到同一目录） |
| `LMMP_BUILD_EXAMPLES` | `ON` / `OFF` | `OFF` | 是否构建示例程序（开启后与动态库输出到同一目录） |
| `TARGET_SYSTEM` | `AUTO` / `WIN` / `LIN` / `MAC` | `AUTO` | 目标系统，一般无需手动指定 |
| `LMMP_DEBUG_STACK_OVERFLOW_CHECK` | `ON` / `OFF` | `OFF` | LMMP 内部栈溢出检查（开销：高） |
| `LMMP_DEBUG_ASSERT_CHECK` | `ON` / `OFF` | `OFF` | debug_assert 检查（开销：中） |
| `LMMP_DEBUG_PARAM_ASSERT_CHECK` | `ON` / `OFF` | `OFF` | 函数参数检查（开销：中） |
| `LMMP_DEBUG_MEMORY_CHECK` | `ON` / `OFF` | `OFF` | LMMP 内部堆/栈内存检查（开销：很高） |
| `LMMP_MEMORY_MORE_ALLOC_TIMES` | 正整数 | `1` | 内存检查的额外分配倍数（十分位） |
| `LMMP_DEBUG_MEMORY_LEAK` | `ON` / `OFF` | `OFF` | 内存分配/释放统计（开销：低） |

工具链选择的补充说明：

+ `LMMP_COMPILER_DIR` 适合工具链不在 `PATH` 上的场景，等价于自动传全路径的 `CMAKE_C/CXX_COMPILER`；需要精确指定单个可执行文件时仍可直接使用 CMake 原生变量。编译器在首次配置时固化进构建树，**更换目录必须使用全新构建目录**（同树重配会静默沿用旧编译器，构建守卫将显式报错拒绝）。
+ `LMMP_LINKER_DIR` 经驱动的 `-B` 前缀与 `-fuse-ld` 生效。macOS 下 Apple ld64 之外的链接器仅 lld 可用；mold 需要较新驱动支持。链接器可同树重新配置直接切换。

## 动态库

配置 `LMMP_OUTPUT_DIR` 后，构建产物以平铺形态汇聚到该目录，供其他项目直接引入：

```bash
cmake -S . -B build-out -DCMAKE_BUILD_TYPE=Release \
  -DLMMP_COMPILER_DIR=/path/to/mingw64/bin \
  -DLMMP_OUTPUT_DIR=/path/to/lmmp-dist
cmake --build build-out
```

产物布局（`LMMP_OUTPUT_DIR` 内，不区分配置子目录）：

```
lmmp-dist/
├── lmmp.dll / liblmmp.so / liblmmp.dylib   # 动态库
├── liblmmp.dll.a / lmmp.lib 等             # 导入库
└── include/lmmp/                           # 对外头文件
```

单元测试、基准与示例默认不构建；如需在同一目录获得它们，加 `-DLMMP_BUILD_TESTS=ON` 等即可，可执行文件与动态库同层放置。

## 测试、基准与调优

LMMP 提供测试、基准和调优功能。默认只构建动态库；测试与基准需显式开启（`-DLMMP_BUILD_TESTS=ON` / `-DLMMP_BUILD_BENCHMARKS=ON`）。调优功能需单独配置，默认不参与项目构建。关于调优程序，可以阅读 `tune/lmmp/README.md`。

```bash
# 构建（含单元测试）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLMMP_BUILD_TESTS=ON
cmake --build build -j

# 运行全部单元测试
ctest --test-dir build --output-on-failure
# 或直接运行，支持 --filter 过滤、--list 列出用例
./dist/lmmp/bin/debug/LmmpTest --list
./dist/lmmp/bin/debug/LmmpTest --filter numth/gcd

# 运行性能基准（同样支持 --filter / --list）
./dist/lmmp/bin/debug/LmmpBenchmark
./dist/lmmp/bin/debug/LmmpBenchmark --filter lmmpn/mul
```

## 接口与调用说明

目前，LMMP的接口主要在``include/lmmp/``目录下，分为两个系列：

+ 通用算子系列：
  这部分模块为核心算子，为尽可能高效，通常均有极为严格的调用限制。
  - ``lmmp.h``：LMMP全局资源管理模块，包含如初始化、释放线程局部资源，abort回调函数，内存管理等功能。
  - ``lmmpn.h``：多精度整数运算子模块，包含如加减乘除、取模、移位、比较、开方等高精度计算。
  - ``numth.h``：数论运算子模块，包含如幂次方、逆元、GCD、阶乘、组合数、素性检验等高精度计算。
  - ``mprand.h``：随机数生成子模块，包含生成高精度随机数生成算法。
  - ``secret.h``：加密模块，包含如hash、加密解密（未实现）等高精度计算。
+ 标准接口系列：
  这部分模块均未实现，计划采用不透明结构体的方式，提供更高级的接口。

## 快速开始

```c++
#include <stdio.h>
#include "include/lmmp/numth.h"

int main() {
    lmmp_global_init(); // 初始化LMMP（单线程）全局资源

    uint n = 10000;
    printf("calculating factorial of %d\n", n);
    mp_bitcnt_t bits;
    mp_size_t len = lmmp_factorial_size_(n, &bits); // 获取阶乘缓冲区大小
    mp_ptr dst = (mp_ptr)lmmp_alloc(len * sizeof(mp_limb_t)); // 分配阶乘缓冲区
    len = lmmp_factorial_(dst, bits, len, n); // 计算阶乘
    printf("completed.\n");

    printf("result: %llx ... %llx\n", dst[len - 1], dst[0]);

    lmmp_free(dst);
    lmmp_global_deinit(); // 释放LMMP（单线程）全局资源
}
```
