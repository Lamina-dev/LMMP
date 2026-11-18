# LMMP
Lamina的高精度计算库。

提供接近GMP的任意精度计算能力，在可以支持汇编的情况下，LMMP和GMP通常具有相近的性能表现。

LMMP同时支持或计划支持包括但不限于如开方、阶乘、组合数、素性检验、质因数分解等复杂的高精度计算。很多功能正在开发中。

## 编译

LMMP 使用 CMake 构建，零外部依赖。当前仅支持 GNU/Clang 风格工具链：

+ Windows：MinGW GCC 或 GNU 驱动的 clang（不支持 MSVC / clang-cl）
+ Linux：GCC / Clang
+ macOS：Clang / GCC

动态库 liblmmp.so/liblmmp.dylib/lmmp.dll 由 C 编写。构建默认按平台选择汇编后端，汇编文件采用 GAS/LLVM 兼容的 `.S` 语法。

```bash
cmake -P cmake/lmmp/config.cmake -- configure -B build
cmake --build build -j
```

配置命令默认使用 Ninja。Windows 下不使用 Visual Studio 生成器。所有公开配置项、类型和默认值由 `cmake/lmmp/config.options` 集中声明，可用 `help` 和 `list` 查看。

支持情况：
+ x86_64 Linux / Windows(MinGW/Clang)：使用 `src/lmmp/lmmpn/arch/x64/*.S`
+ arm64 macOS / Linux：使用 `src/lmmp/lmmpn/arch/arm64/*.S`
+ 其他架构或 macOS x64：自动回退到 `src/lmmp/lmmpn/arch/generic/` 的 C 实现

在开启汇编时，所能够支持运行的架构及芯片如下：
+ X86-64：x86-64-v3指令集+adx指令集；支持芯片：Intel Broadwell及之后，AMD Excavator及之后（锐龙、霄龙）
+ ARM64：ARM64-v8.2-a指令集；支持芯片：Cortex-A55/A75/A77/A78, Neoverse-N1/E1, AWS Graviton2, Ampere Altra, 华为鲲鹏920, 富士通A64FX等。


## 主要配置选项

```bash
cmake -P cmake/lmmp/config.cmake -- help
cmake -P cmake/lmmp/config.cmake -- list
```

配置时使用 `--set NAME=VALUE`，配置项由 `config.options` 校验并转换为内部 CMake 缓存值。

```bash
cmake -P cmake/lmmp/config.cmake -- configure -B build \
  --set LMMP_BUILD_TYPE=Release \
  --set LMMP_LIBRARY_BUILD_TYPE=Release \
  --set LMMP_TEST_BUILD_TYPE=Debug \
  --set LMMP_BUILD_TESTS=ON
```

`LMMP_BUILD_TYPE` 是默认目标组编译配置。动态库、测试、示例、性能和调优目标分别由对应的 `LMMP_*_BUILD_TYPE` 选项控制，值为 `inherit` 时继承默认配置。库内断言和内存行为选项也在同一配置目录中声明。

工具链选择的补充说明：

+ `LMMP_COMPILER_DIR` 适合工具链不在 `PATH` 上的场景，等价于自动传全路径的 `CMAKE_C/CXX_COMPILER`；需要精确指定单个可执行文件时仍可直接使用 CMake 原生变量。编译器在首次配置时固化进构建树，**更换目录必须使用全新构建目录**（同树重配会静默沿用旧编译器，构建守卫将显式报错拒绝）。
+ `LMMP_LINKER_DIR` 经驱动的 `-B` 前缀与 `-fuse-ld` 生效。macOS 下 Apple ld64 之外的链接器仅 lld 可用；mold 需要较新驱动支持。链接器可同树重新配置直接切换。

## 动态库

配置 `LMMP_OUTPUT_DIR` 后，构建产物以平铺形态汇聚到该目录，供其他项目直接引入：

```bash
cmake -P cmake/lmmp/config.cmake -- configure -B build-out \
  --set LMMP_COMPILER_DIR=/path/to/mingw64/bin \
  --set LMMP_LINKER_DIR=/path/to/llvm/bin \
  --set LMMP_OUTPUT_DIR=/path/to/lmmp-dist
cmake --build build-out
```

产物布局（`LMMP_OUTPUT_DIR` 内，不区分配置子目录）：

```
lmmp-dist/
├── lmmp.dll / liblmmp.so / liblmmp.dylib   # 动态库
├── liblmmp.dll.a / lmmp.lib 等             # 导入库
└── include/lmmp/                           # 对外头文件
```

默认构建公开动态库。启用测试或示例时，各可执行文件与动态库同层放置。基准和调优程序按目标构建，也输出到该目录。

## 测试、基准与调优

`src/lmmp/<module>/test/` 下的 C 用例由本模块的 `main.c` 统一执行。`perf/` 和 `tune/` 下的每个 `.c` 对应一个独立程序。基准程序测量一种运算或一组直接相关的实现，调优程序测量一个阈值或一组相关阈值。测试、基准和调优程序在构建时会带上共用的内部动态库 `lmmp_internal`。

```bash
# 构建动态库和单元测试
cmake -P cmake/lmmp/config.cmake -- configure -B build --set LMMP_BUILD_TESTS=ON
cmake --build build -j

# 运行全部单元测试
ctest --test-dir build --output-on-failure
# 运行数论模块的测试
ctest --test-dir build -R '^test_numth$' --output-on-failure

# 点名构建开方基准和 2×2 矩阵平方调优程序
cmake --build build --target perf_numth_sqrt
cmake --build build --target tune_numth_mat22_sqr
./dist/lmmp/bin/perf_numth_sqrt
./dist/lmmp/bin/tune_numth_mat22_sqr
```

批量构建前会显示目标清单和总数，输入 `y` 后开始编译。目标构建脚本使用自有命令行参数，不要求 CMake 的 `-D` 变量。

```bash
# 构建全部基准程序
cmake -P cmake/lmmp/build.cmake -- build -B build -t perf
# 构建数论模块的调优程序
cmake -P cmake/lmmp/build.cmake -- build -B build -t tune_numth
```

阈值定义在各模块的 `arch/<后端>/mparam.h`。CMake 按构建所选后端汇总成供全库使用的内部头文件。调优程序用相同输入比较算法路径并输出测量结果。更新参数时修改所属模块的头文件，再重新构建。

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
