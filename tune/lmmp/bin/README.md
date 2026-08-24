# tune/lmmp/bin

构建 `LMMP_TUNE_MODE=ON` 时，`lmmp_tune` 可执行文件输出到此目录。

常用命令：

```bash
# 快速探索（候选点较少，适合快速查看调优流程）
./tune/lmmp/bin/lmmp_tune --quick --only mul22,mul33,npr_ushort

# 完整调优并写回 mparam.h（写回前建议确认结果）
./tune/lmmp/bin/lmmp_tune --write
```

更多说明见 `tune/lmmp/README.md`。
