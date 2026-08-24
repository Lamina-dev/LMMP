# LMMP 阈值调优程序

本目录是独立于正常动态库构建的调优套件，按 `include/`、`src/`、`bin/` 组织：

```
tune/lmmp/
├── include/
│   └── lmmp_tune.h          # 可调阈值变量声明
├── src/
│   ├── lmmp_tune.c          # 调优驱动（基准测试 + 搜索 + 写回）
│   └── lmmp_tune_params.c   # LMMP_TUNE 阈值变量定义
├── bin/
│   ├── README.md
│   └── .gitignore           # 忽略编译出的 lmmp_tune
└── CMakeLists.txt
```

## 原理

1. 根 CMake 打开 `LMMP_TUNE_MODE` 后，`src/lmmp` 会编译一个**静态调优核心**
   `liblmmp_tune_core.a`。该核心使用与正常库完全相同的源码，但在编译时定义
   `LMMP_TUNE`。此时 `include/lmmp/impl/mparam.h` 中部分阈值宏不再展开为
   整数常量，而是绑定到 `tune/lmmp/src/lmmp_tune_params.c` 中定义的一组
   `uint64_t` 全局变量。
2. `lmmp_tune` 调优驱动先设置这些变量，再运行基准测试。改变阈值时**不需要
   重新编译库**，因此可以在合理时间内完成多维搜索。
3. 搜索完成后，驱动打印推荐阈值；使用 `--write` 时会把推荐值写回
   `include/lmmp/impl/mparam.h` 中对应 `#define` 的默认值。

## 构建

```bash
cmake -S . -B build-tune \
  -DCMAKE_BUILD_TYPE=Release \
  -DLMMP_TUNE_MODE=ON \
  -DLMMP_ASM=AUTO
cmake --build build-tune --parallel 4
```

产物（调优模式全部输出到 `tune/lmmp/bin`，与 `dist` 分离）：

- `tune/lmmp/bin/liblmmp_tune_core.a`（Linux/macOS 静态核心）
- `tune/lmmp/bin/lmmp_tune_core.lib`（Windows 静态核心）
- `tune/lmmp/bin/lmmp_tune` / `tune/lmmp/bin/lmmp_tune.exe`（调优驱动）

## 运行

```bash
# 查看帮助
./tune/lmmp/bin/lmmp_tune --help

# 快速探索部分阈值（速度较快，适合跑通流程）
./tune/lmmp/bin/lmmp_tune --quick --only mul22,mul33,npr_ushort

# 完整调优所有已接入阈值
./tune/lmmp/bin/lmmp_tune

# 调优并将结果写回 include/lmmp/impl/mparam.h
./tune/lmmp/bin/lmmp_tune --write
```

`--only` 支持逗号分隔的名称列表，可用名称包括：
`mul22`, `mul33`, `mul44`, `mullo`, `npr_ushort`, `npr_uint`,
`ncr`, `pow1`, `elem`, `mat22_mul`, `mat22_sqr`。

## 测量与搜索策略

- **计时**：Windows 使用 `QueryPerformanceCounter`，Linux/macOS 使用
  `clock_gettime(CLOCK_MONOTONIC)`。每个候选阈值先预热，再做多次采样并取
  **中位数**，避免个别异常测量值主导结果。
- **一维阈值**：三轮“粗网格 + 局部细化”搜索；`--quick` 时减少候选点数。
- **二维阈值**：对 `(K, B)` 先做几何/线性粗网格，再在最优值附近做两轮
  局部细化。当前用于 `nPr` 的 ushort/uint 两组 `(K, B)` 阈值。
- **跳过项**：`L1_CACHE_SIZE`、`L2_CACHE_SIZE`、`PART_SIZE`、
  `LMMP_DEFAULT_STACK_SIZE`、`LMMP_POOL_SIZE` 等缓存/内存池参数与内存
  分配策略相关，不参与自动调优；`PRIME_CACHE_*`、`MP_*` 等为固定常量，
  也不参与调优。

## 注意事项

- 调优结果受 CPU、操作系统、编译器版本和后台负载影响，建议在目标平台的
  空闲机器上运行完整模式。
- 建议先运行 `--quick` 确认流程，再以完整模式复核。确认前不要轻易
  `--write`。
- 如果改变后的阈值破坏了 `MUL_TOOM22 < MUL_TOOM33 < MUL_TOOM44 < MUL_FFT`
  的顺序约束，相关 `#if` 会以静态默认值为准，调优时请留意打印结果。

## 如何新增一个待调优阈值

调优框架采用“测量两个算法的成本曲线/成本表，再离线搜索最优阈值”的方式。新增一个阈值通常需要以下步骤：

1. **在 `include/lmmp/impl/mparam.h` 中接入运行时变量**
   - 在 `LMMP_TUNE` 声明块中加入 `extern uint64_t lmmp_tune_XXX_THRESHOLD;`
   - 把原来的 `#define XXX_THRESHOLD value` 包成：
     ```c
     #ifdef LMMP_TUNE
     #define XXX_THRESHOLD lmmp_tune_XXX_THRESHOLD
     #else
     #define XXX_THRESHOLD value
     #endif
     ```
   - 若该宏被用于数组长度（例如 `TO_STR_BASEPOW_THRESHOLD`），还需在对应 `.c` 中增加
     `#ifdef LMMP_TUNE` 的堆分配路径，否则运行时会因 VLA/数组长度报错。

2. **在 `tune/lmmp/src/lmmp_tune_params.c` 和 `tune/lmmp/include/lmmp_tune.h` 中**
   - 增加变量定义和 `extern` 声明。

3. **在 `tune/lmmp/src/lmmp_tune.c` 中添加基准**
   - 1D 阈值：参照 `obj_mul` / `bench_mul_size` 的写法。测量时先把阈值设到
     两个极端（强制走 A / 强制走 B），分别得到每个 size 的两条成本曲线；
     然后既可按 `search_1d` 做在线搜索，也可把曲线缓存后离线扫描最优分隔点。
   - 2D 阈值：参照 `npr_cache_fill` / `npr_cache_eval` 的写法。对每个采样点
     分别测量两种算法的耗时，之后对任意 `(K,B)` 只需查表求和，无需重新计时。

4. **在 `main()` 的 `--only` 列表中增加对应名字**
   - 目前支持：`mul22`, `mul33`, `mul44`, `mullo`, `npr_ushort`, `npr_uint`,
     `ncr`, `pow1`, `elem`, `mat22_mul`, `mat22_sqr`。

5. **非调优时保持零暴露**
   - 需要暴露内部 `static` 函数时，仅用 `#ifdef LMMP_TUNE` 包裹一个
     非 `static` 包装函数，例如 `src/lmmp/numth/nPr.c` 中的
     `lmmp_tune_odd_nPr_product_`。正常构建不会编译该符号。
