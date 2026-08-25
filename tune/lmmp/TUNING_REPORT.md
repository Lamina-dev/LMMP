# LMMP 阈值调优报告

- 机器：macOS-arm64（Apple M 系列开发机）
- 工具链：AppleClang 21 / LLVM，Release，`LMMP_ASM=AUTO`（ARM64 汇编）
- 调优命令：
  `./tune/lmmp/bin/lmmp_tune --samples 9 --min-ms 12`
- 结果文件：`tune/lmmp/bin/lmmp_tune_results.txt` 与 `.h`
- 写回状态：**未写回** `include/lmmp/impl/mparam.h`；本次报告仅为机器相关建议值。

## 阈值变化

| 阈值 | 旧值 | 新值 |
|---|---:|---:|
| `MUL_TOOM22_THRESHOLD` | 20 | 53 |
| `MUL_TOOM33_THRESHOLD` | 65 | 121 |
| `MUL_TOOM44_THRESHOLD` | 581 | 1667 |
| `MUL_FFT_THRESHOLD` | 2316 | 4490 |
| `MULLO_BASECASE_THRESHOLD` | 20 | 5 |
| `MULLO_DC_THRESHOLD` | 3521 | 144 |
| `DIV_DIVIDE_THRESHOLD` | 50 | 20 |
| `BNINV_NEWTON_THRESHOLD` | 20 | 20 |
| `MUL_FFT_MODF_THRESHOLD` | 477 | 544 |
| `MULHI_MERSENNE_THRESHOLD` | 477 | 30 |
| `TO_STR_BASEPOW_THRESHOLD` | 30 | 30 |
| `TO_STR_DIVIDE_THRESHOLD` | 20 | 14 |
| `FROM_STR_BASEPOW_THRESHOLD` | 100 | 266 |
| `FROM_STR_DIVIDE_THRESHOLD` | 45 | 29 |
| `POW_1_EXP_THRESHOLD` | 10 | 126 |
| `POW_WIN2_EXP_THRESHOLD` | 50 | 36 |
| `POW_WIN2_N_THRESHOLD` | 400 | 1341 |
| `FACTORS_MUL_N_THRESHOLD` | 30 | 172 |
| `PERMUTATION_USHORT_K_THRESHOLD` | 18 | 11 |
| `PERMUTATION_USHORT_B_THRESHOLD` | 21164 | 54886 |
| `PERMUTATION_UINT_K_THRESHOLD` | 136 | 224 |
| `PERMUTATION_UINT_B_THRESHOLD` | 1659975 | 1239400 |
| `BINOMIAL_RN_BASECASE_THRESHOLD` | 40 | 8 |
| `ELEM_MUL_BASECASE_THRESHOLD` | 25 | 94 |
| `MAT22_MUL_STRASSEN_THRESHOLD` | 60 | 24 |
| `MAT22_SQR_STRASSEN_THRESHOLD` | 50 | 50 |
| `SQRT_INVNEWTON_THRESHOLD` | 50 | 50 |
| `DIVEXACT_BASECASE_THRESHOLD` | 50 | 15 |
| `DIVEXACT_NN_THRESHOLD` | 350 | 76 |

建议值满足当前静态顺序约束：`MUL_TOOM22 < MUL_TOOM33 < MUL_TOOM44 < MUL_FFT`。

## 性能变化（简要）

调优搜索的目标函数是 GMP 风格的 **badness**：对每个代表尺寸，把错选算法路径的相对损失
`chosen/faster - 1` 求和并最小化。它比“绝对总耗时”对交叉点附近的错误选择更敏感。

- 可计算绝对代表耗时的 21 个一维模块：旧阈值总耗时约 **396.29 ms**，新阈值约
  **396.13 ms**，代表点绝对耗时基本持平（-0.04%）。
- 全部 27 个调优单元（nPr 的 K/B 对合并计一个单元）的 badness 总和：
  **94.97 -> 5.96，下降约 93.7%**。
- 其中改进最明显的是：
  - `BINOMIAL_RN_BASECASE_THRESHOLD`：旧阈值 40 在 8..39 limb 区间错选 basecase，
    新值 8 后这些代表点全部走 factor 路径。
  - `POW_1_EXP_THRESHOLD`：旧值 10 在 11..126 指数区间错选连乘，新值 126 后该区间
    改走 `pow_1` 窗口算法。
  - `MUL_TOOM22_THRESHOLD`：配合修复后的不平衡乘法分派，新值 53 减少了 20..52
    区间的路径损失。
- badness 大幅下降不等于整库同比例加速；它表示在调优覆盖的代表工作负载上，
  “明显选错算法”的比例大幅减少。

## 稳定性说明

本次调优程序相较旧版做了以下稳定性改进：

- 每条路径 9 次重复采样，排序取中位数；MAD/median 超限自动重测。
- A/B 路径交替采样，后台负载漂移同时作用于两侧。
- 密集整数/几何样本点 + 旧默认值附近加密。
- badness 平坦区间优先回靠旧值，避免等价区间随机漂移。
- 修复了旧程序可把 `MUL_TOOM22_THRESHOLD` 调到 30 以上后，`lmmp_mul_` 固定
  `tp[30]` 缓冲区溢出的问题；该问题会直接导致调优进程崩溃，也是旧版前后两次
  结果不可信的重要根因之一。

## 写回方法

确认结果后，可显式写回：

```bash
./tune/lmmp/bin/lmmp_tune --write
```

程序会先生成 `include/lmmp/impl/mparam.h.tune-bak`，再替换对应 `#define`。
