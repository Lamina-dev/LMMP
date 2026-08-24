# LMMP 阈值调优报告（macOS-arm64 / Apple M1 / Clang-2100 / Release）

> 数据来源：本机快速探索模式 `./tune/lmmp/bin/lmmp_tune --quick --only mul22,mul33,mullo,ncr,pow1`
>
> 说明：快速模式候选点较少，且部分基准目标函数单次耗时较小（0.01 ms 量级），
> 因此下表数据用于验证调优流程，**不建议直接写回**。生产调优请在空闲机器上
> 使用完整模式复核。

## 快速探索建议值

| 阈值 | 原值 | 快速探索建议 | 备注 |
|---|---:|---:|---|
| `MUL_TOOM22_THRESHOLD` | 20 | 21 | 与原值基本一致，目标函数约 0.145 ms |
| `MUL_TOOM33_THRESHOLD` | 65 | 155 | 快速模式在该点稳定收敛，建议完整模式复核 |
| `MULLO_BASECASE_THRESHOLD` | 20 | 59 | 计时粒度过细，可信度低 |
| `BINOMIAL_RN_BASECASE_THRESHOLD` | 40 | 10 | 计时粒度过细，可信度低 |
| `POW_1_EXP_THRESHOLD` | 10 | 41 | 计时粒度过细，可信度低 |

## 目标函数计时对比（快速模式，越小越好）

| 阈值 | 原值目标函数 | 建议值目标函数 |
|---|---:|---:|
| `MUL_TOOM22_THRESHOLD` | 0.147 ms | 0.145 ms |
| `MUL_TOOM33_THRESHOLD` | 0.147 ms | 0.141 ms |
| `MULLO_BASECASE_THRESHOLD` | 0.014 ms | 0.014 ms |
| `BINOMIAL_RN_BASECASE_THRESHOLD` | 0.013 ms | 0.004 ms |
| `POW_1_EXP_THRESHOLD` | 0.000 ms | 0.000 ms |

## 结论

1. 调优框架可以稳定完成一维/二维搜索，并输出可写回的建议值。
2. 在 Apple M1 上，`MUL_TOOM22`/`MUL_TOOM33` 的建议值与现有默认值同量级；
   `MUL_TOOM33` 快速搜索偏向更大阈值，需要完整模式复核。
3. `MULLO_BASECASE`、`BINOMIAL_RN_BASECASE`、`POW_1_EXP` 的目标函数单次耗时
   过短，计时噪声大，需要增大重复次数或增加更重的基准输入才能得到可信结果。
4. 二维阈值 `PERMUTATION_USHORT (K,B)`、`PERMUTATION_UINT (K,B)` 已接入，
   完整模式会执行“几何粗网格 + 局部细化”搜索。快速模式未在本次报告中运行，
   以避免候选点不足导致误导性结论。

## 如何写回

```bash
# 完整模式复核后，如确认建议值合理，可写回默认 mparam.h：
cmake --build build-tune --target lmmp_tune
./tune/lmmp/bin/lmmp_tune --write
```
