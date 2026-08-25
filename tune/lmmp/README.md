# LMMP 阈值调优程序

本目录是独立于正常动态库构建的调优套件。调优模式会编译一个静态调优核心
`liblmmp_tune_core.a`，其中 `include/lmmp/impl/mparam.h` 的可调阈值被绑定到
`uint64_t` 运行时变量；调优驱动先设置变量，再运行基准，因此候选阈值搜索不需要
反复重新编译核心。

```
tune/lmmp/
├── include/
│   ├── lmmp_tune.h            # 可调阈值变量声明
│   └── lmmp_tune_internal.h   # 计时/统计/搜索公共接口与模块注册声明
├── src/
│   ├── lmmp_tune.c            # CLI、模块注册、结果输出
│   ├── lmmp_tune_common.c     # 计时、配对测量、badness 搜索、结果写回
│   ├── lmmp_tune_params.c     # LMMP_TUNE 阈值变量定义
│   └── tune_<name>.c          # 每个阈值（或 nPr 的 K/B 阈值对）一个文件
├── bin/                       # 构建产物和调优结果
│   ├── lmmp_tune
│   ├── lmmp_tune_results.txt  # 最后一次运行结果（旧值 -> 新值）
│   └── lmmp_tune_results.h    # 便于查看/复用的宏定义版本
└── CMakeLists.txt
```

## 构建

```bash
cmake -S . -B build-tune \
  -DCMAKE_BUILD_TYPE=Release \
  -DLMMP_TUNE_MODE=ON \
  -DLMMP_ASM=AUTO
cmake --build build-tune --parallel 8
```

调优模式与正常动态库构建互斥，不会影响 `dist/` 下的正常产物。

## 运行

```bash
# 列出全部阈值模块
./tune/lmmp/bin/lmmp_tune --list

# 完整调优全部 29 个已接入阈值
./tune/lmmp/bin/lmmp_tune

# 只调若干阈值（id 或兼容别名均可，逗号分隔）
./tune/lmmp/bin/lmmp_tune --only mul22,mul33,npr_ushort

# 调优并把结果写回 include/lmmp/impl/mparam.h
# 写回前自动备份 mparam.h.tune-bak；不传 --write 时只写 bin/ 下的结果文件
./tune/lmmp/bin/lmmp_tune --write
```

结果文件默认写入：

- `tune/lmmp/bin/lmmp_tune_results.txt`
- `tune/lmmp/bin/lmmp_tune_results.h`

可用参数：

| 参数 | 默认值 | 说明 |
|---|---:|---|
| `--only <a,b,...>` | 全部 | 只运行指定模块 |
| `--list` | - | 列出模块 id、标题和兼容别名 |
| `--samples <N>` | 7 | 每点每路径的重复采样数，内部强制为奇数 |
| `--min-ms <MS>` | 10 | 每次采样希望达到的累计计时毫秒数 |
| `--mad-limit <X>` | 0.25 | `MAD/median` 超过该值时整轮重测 |
| `--max-retry <N>` | 1 | 不稳定样本的最大重测轮数 |
| `--out <file>` | `bin/lmmp_tune_results.txt` | 结果文本路径，同时生成同名 `.h` |
| `--write` | 关 | 写回 `mparam.h` 并生成 `.tune-bak` 备份 |

不再提供 `--quick`。旧程序的快速模式采样过少、网格过粗，是前后两次调优结果
漂移的主要来源之一。如只想快速验证构建，可使用
`--only mul22 --samples 3 --min-ms 1`。

## 测量与搜索策略

1. **成对交替测量**：对每个一维样本点，先分别校准 low/high 两条路径的重复
   次数，再按 `L,A,L,A,...` 或 `A,B,A,B,...` 交替采样。这样后台负载漂移和
   CPU 温漂会同时作用于两条曲线，降低相对偏差。
2. **中位数 + MAD 复核**：每个采样点默认 7 或 9 次重复，排序后取中位数；
   若 `MAD/median` 超过阈值则整轮重测。中位数本身不受少数极慢/极快样本主导。
3. **密集样本点**：小范围逐整数，大范围按 1.06 几何步进，并在旧默认值附近
   额外加密。
4. **GMP 风格 badness 搜索**：对候选阈值，把每个样本点错选路径的相对损失
   `chosen/faster - 1` 求和，选择总 badness 最小的整数阈值。若最小值附近
   存在 1% 以内的平坦区间，优先保留靠近旧默认值的候选，避免测量噪声在等价
   区间里随机游走。
5. **二维 nPr 阈值**：先对稠密 `(n,r)` 样本分别强制 product/factor 路径并
   交替测量，再在 K 逐整数、B 对数稠密网格上求最小 badness；同样有平坦区
   间回靠旧值策略。
6. **递归内部阈值**：`TO_STR_DIVIDE_THRESHOLD` 与
   `FROM_STR_DIVIDE_THRESHOLD` 无法化简为两条独立曲线，改为固定外层阈值后，
   在候选阈值整数域上逐点在线测量并做归一化 badness 选择。

## 当前接入的阈值

`--list` 输出全部模块。旧名称仍作为别名保留：`mul22`、`mul33`、`mul44`、
`mullo`、`npr_ushort`、`npr_uint`、`ncr`、`pow1`、`elem`、`bninv`。

覆盖范围为 `mparam.h` 中全部 29 个 `LMMP_TUNE` 运行时阈值：

- 乘法/低位乘法：`MUL_TOOM22/33/44`、`MUL_FFT`、`MULLO_BASECASE`、
  `MULLO_DC`、`MUL_FFT_MODF`、`MULHI_MERSENNE`
- 除法/逆元/开方：`DIV_DIVIDE`、`BNINV_NEWTON`、`SQRT_INVNEWTON`
- 字符串转换：`TO_STR_DIVIDE/BASEPOW`、`FROM_STR_DIVIDE/BASEPOW`
- 数论组合：`PERMUTATION_USHORT_K/B`、`PERMUTATION_UINT_K/B`、
  `BINOMIAL_RN_BASECASE`、`ELEM_MUL_BASECASE`、`FACTORS_MUL_N`
- 幂：`POW_1_EXP`、`POW_WIN2_EXP`、`POW_WIN2_N`
- 精确除法：`DIVEXACT_BASECASE`、`DIVEXACT_NN`
- 2x2 矩阵：`MAT22_MUL_STRASSEN`、`MAT22_SQR_STRASSEN`

## 注意事项

- 调优结果与 CPU、OS、编译器、后台负载有关，仅对本机/同类机型有效。
- 完整模式耗时较长（本机约数十分钟），运行期间应尽量关闭重负载任务。
- `mparam.h` 中 `MUL_TOOM22 < MUL_TOOM33 < MUL_TOOM44 < MUL_FFT` 的静态
  断言使用固定默认值；如最终写入值破坏顺序，相关 `#if` 会以默认顺序为准。
  建议写回前检查结果文件。
- `--write` 是可选的。默认只生成 `bin/lmmp_tune_results.*`，不会修改任何
  库源码。
