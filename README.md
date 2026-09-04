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

支持情况：
+ x86_64 Linux / Windows(MinGW/Clang)：使用 `src/lmmp/lmmpn/asm/x64/*.S`
+ arm64 macOS / Linux：使用 `src/lmmp/lmmpn/asm/arm64/*.S`
+ 其他架构或 macOS x64：自动回退到 `src/lmmp/lmmpn/generic/` 的 C 实现

在开启汇编时，所能够支持运行的架构及芯片如下：
+ X86-64：x86-64-v3指令集+adx指令集；支持芯片：Intel Broadwell及之后，AMD Excavator及之后（锐龙、霄龙）  
+ ARM64：ARM64-v8.2-a指令集；支持芯片：Cortex-A55/A75/A77/A78, Neoverse-N1/E1, AWS Graviton2, Ampere Altra, 华为鲲鹏920, 富士通A64FX等。


## 主要配置选项

| 选项 | 取值 | 默认 | 说明 |
|---|---|---|---|
| `LMMP_ASM` | `AUTO` / `GENERIC` / `X64` / `ARM64` | `AUTO` | 汇编模式：自动选择、不使用汇编、x64 汇编、arm64 汇编 |
| `LMMP_BUILD_TESTS` | `ON` / `OFF` | `ON` | 是否构建单元测试 |
| `LMMP_BUILD_BENCHMARKS` | `ON` / `OFF` | `ON` | 是否构建性能基准 |
| `LMMP_BUILD_EXAMPLES` | `ON` / `OFF` | `OFF` | 是否构建示例程序 |
| `LMMP_BUILD_MEASURES` | `ON` / `OFF` | `OFF` | 是否构建测量工具（需要同时构建基准） |
| `TARGET_SYSTEM` | `AUTO` / `WIN` / `LIN` / `MAC` | `AUTO` | 目标系统，一般无需手动指定 |
| `LMMP_DEBUG_STACK_OVERFLOW_CHECK` | `ON` / `OFF` | `OFF` | LMMP 内部栈溢出检查（开销：高） |
| `LMMP_DEBUG_ASSERT_CHECK` | `ON` / `OFF` | `OFF` | debug_assert 检查（开销：中） |
| `LMMP_DEBUG_PARAM_ASSERT_CHECK` | `ON` / `OFF` | `OFF` | 函数参数检查（开销：中） |
| `LMMP_DEBUG_MEMORY_CHECK` | `ON` / `OFF` | `OFF` | LMMP 内部堆/栈内存检查（开销：很高） |
| `LMMP_MEMORY_MORE_ALLOC_TIMES` | 正整数 | `1` | 内存检查的额外分配倍数（十分位） |
| `LMMP_DEBUG_MEMORY_LEAK` | `ON` / `OFF` | `OFF` | 内存分配/释放统计（开销：低） |

## 测试、基准与调优

LMMP 提供测试、基准和调优功能。测试与基准默认随主项目一起构建（可用 `-DLMMP_BUILD_TESTS=OFF` / `-DLMMP_BUILD_BENCHMARKS=OFF` 关闭）。调优功能需单独配置，默认不参与项目构建。关于调优程序，可以阅读 `tune/lmmp/README.md`。

```bash
# 构建
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
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

## 目录结构

```
LMMP/                       # 项目根目录
├── LICENSE                 # 许可证文件
├── README.md               # README
├── CMakeLists.txt          # 根目录CMake（全局配置：构建类型、C/C++标准、输出目录等）
├── dist/                   # 编译产物根目录（自动生成，存放所有库和可执行文件）
│   └── lmmp/               # LMMP项目专属产物目录
│       └── bin/            # 可执行文件输出目录
│           ├── release/    # 输出目录
│           └── debug/      # 输出目录
├── include/                # 头文件目录（对外暴露，所有子模块可引用）
│   └── lmmp/               # 项目名嵌套目录
│       ├── impl/           # 内部实现头文件目录（仅供内部使用）
│       ├── version.h       # 版本头文件
│       └── .h              # 其他头文件
├── src/                    # 核心源代码根目录
│   └── lmmp/               # 核心库源代码目录（对应include/lmmp）
│       ├── lmmp/           # 通用函数或通用逻辑实现文件
│       ├── lmmpn/          # 多精度整数运算子模块实现文件
│       │   ├── asm         # 汇编实现文件
│       │   ├── generic     # c模拟汇编实现文件
│       │   └── *.c/        # 实现文件
│       ├── global/         # 全局变量定义文件
│       ├── numth/          # 数论计算子模块实现文件
│       ├── secret/         # 密码学子模块实现文件
│       ├── mprand/         # 随机数生成子模块实现文件
│       └── CMakeLists.txt  # 源码目录CMake
├── benchmark/              # 基准测试根目录
│   └── lmmp/               # LMMP项目基准测试目录
│       ├── CMakeLists.txt  # 基准测试CMake配置
│       ├── include/        # 基准测试私有头文件（仅测试内部使用）
│       ├── src/            # 基准测试源代码目录
│       └── main.cpp        # 基准测试主程序main()
├── example/                # 示例程序根目录
│   └── lmmp/               # LMMP项目基准测试目录
│       ├── CMakeLists.txt  # 示例程序CMake配置
│       ├── example1.cpp    # 示例1
│       └── example2.cpp    # 示例2
├── test/                   # 测试程序根目录
│   └── lmmp/               # LMMP项目基准测试目录
│       ├── CMakeLists.txt  # 测试程序CMake配置
│       ├── include/        # 测试私有头文件（仅测试内部使用）
│       ├── src/            # 测试源代码目录
│       └── main.cpp        # 测试主程序入口函数main()
├── tune/                   # 调优程序根目录（调优程序单独构建）
│   └── lmmp/               # LMMP项目基准测试目录
│       ├── CMakeLists.txt  # 调优程序CMake配置
│       ├── include/        # 调优私有头文件（仅测试内部使用）
│       ├── src/            # 调优源代码目录
│       └── bin/            # 调优可执行文件输出目录
└── build/                  # 构建目录（外部构建，仅供参考）
    ├── CMakeCache.txt      # CMake缓存文件（自动生成）
    ├── Makefile            # 编译脚本（Linux/Mac，自动生成）
    └── else
```

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
