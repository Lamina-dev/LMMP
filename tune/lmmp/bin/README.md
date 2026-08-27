# tune/lmmp/bin

构建 `LMMP_TUNE_MODE=ON` 时，`lmmp_tune` 可执行文件、静态调优核心和调优结果
输出到此目录。

常用命令：

```bash
# 列出全部阈值模块
./tune/lmmp/bin/lmmp_tune --list

# 完整高精度调优（不修改 mparam.h，只写结果文件）
./tune/lmmp/bin/lmmp_tune --samples 9 --min-ms 12

# 只调指定阈值
./tune/lmmp/bin/lmmp_tune --only mul22,mul33,npr_ushort

# 确认结果后写回 include/lmmp/impl/mparam.h 中的 LMMP_DEFAULT_* 宏
./tune/lmmp/bin/lmmp_tune --write
```

结果文件：

- `lmmp_tune_results.txt`：旧值/新值表格。
- `lmmp_tune_results.h`：可直接查看或复用的宏定义。

详细说明见 `tune/lmmp/README.md`。
