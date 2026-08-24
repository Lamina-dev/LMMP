# LMMP 阈值调优报告（macOS-arm64 / Apple M1 / Clang-2100 / Release）

> 数据来源：`./tune/lmmp/bin/lmmp_tune --quick --only ...`
>
> 快速模式候选点较少，计时粒度较粗，以下结果用于验证调优流程；
> 生产调优请在空闲机器上运行完整模式。

## 一维阈值快速探索

| 阈值 | 原值 | 快速建议 | 说明 |
|---|---:|---:|---|
| `MUL_TOOM22_THRESHOLD` | 20 | 20~21 | 与原值基本一致 |
| `MUL_TOOM33_THRESHOLD` | 65 | 132~156 | 快速搜索偏向更大阈值，建议完整模式复核 |
| `MULLO_BASECASE_THRESHOLD` | 20 | 59 | 计时粒度过细，可信度低 |
| `BINOMIAL_RN_BASECASE_THRESHOLD` | 40 | 10 | 计时粒度过细，可信度低 |
| `POW_1_EXP_THRESHOLD` | 10 | 39~41 | 计时粒度过细，可信度低 |

## 二维阈值：成本表法结果

新的 2D 调优先分别强制走 product / factor 路径，为每个 `(n,r)` 采样点
测量两个成本，之后在成本表上做细网格搜索，无需反复计时。

| 阈值 | 原值 | 快速建议 | 说明 |
|---|---:|---:|---|
| `PERMUTATION_USHORT_K_THRESHOLD` | 18 | 17 | 成本表法，约 2.17 ms |
| `PERMUTATION_USHORT_B_THRESHOLD` | 21164 | 512 | 快速模式 B 下界取 512 |
| `PERMUTATION_UINT_K_THRESHOLD` | 136 | 65 | 成本表法，约 3.75 ms |
| `PERMUTATION_UINT_B_THRESHOLD` | 1659975 | 65536 | 快速模式 B 下界取 65536 |

## 已接入但未纳入自动搜索的阈值

以下阈值已接入 `LMMP_TUNE` 运行时绑定，可按 `tune/lmmp/README.md`
中的步骤为它们添加基准测试：

- `BNINV_NEWTON_THRESHOLD`
- `MUL_FFT_MODF_THRESHOLD`
- `TO_STR_DIVIDE_THRESHOLD` / `TO_STR_BASEPOW_THRESHOLD`
- `FROM_STR_DIVIDE_THRESHOLD` / `FROM_STR_BASEPOW_THRESHOLD`
- `MULHI_MERSENNE_THRESHOLD`
- `DIVEXACT_BASECASE_THRESHOLD` / `DIVEXACT_NN_THRESHOLD`

其中 `TO_STR_BASEPOW_THRESHOLD` 原本作为数组长度使用，已在 `LMMP_TUNE`
下改为堆分配，因此可以运行时调优；正常构建仍保持栈上定长数组。

## 如何写回

```bash
# 完整模式复核后，如确认建议值合理，可写回默认 mparam.h：
cmake --build build-tune --target lmmp_tune
./tune/lmmp/bin/lmmp_tune --write
```
