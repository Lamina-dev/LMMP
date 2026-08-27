/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/*
 * LMMP 阈值调优驱动。
 *
 * 只负责命令行、模块注册和结果输出；每个阈值的测量与搜索位于独立的
 * src/tune_<name>.c 中。默认运行全部阈值，并把结果写到
 * tune/lmmp/bin/lmmp_tune_results.txt / .h。只有显式传入 --write 才会
 * 修改 include/lmmp/impl/mparam.h（自动备份为 .tune-bak）。
 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/lmmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LMMP_SOURCE_DIR
#define LMMP_SOURCE_DIR "."
#endif

int tune_run_mul_toom22(void);
int tune_run_mul_toom33(void);
int tune_run_mul_toom44(void);
int tune_run_mul_fft(void);
int tune_run_mullo_basecase(void);
int tune_run_mullo_dc(void);
int tune_run_div_divide(void);
int tune_run_bninv_newton(void);
int tune_run_mul_fft_modf(void);
int tune_run_mulhi_mersenne(void);
int tune_run_to_str_basepow(void);
int tune_run_to_str_divide(void);
int tune_run_from_str_basepow(void);
int tune_run_from_str_divide(void);
int tune_run_pow_1_exp(void);
int tune_run_pow_win2_exp(void);
int tune_run_pow_win2_n(void);
int tune_run_factors_mul_n(void);
int tune_run_permutation_ushort(void);
int tune_run_permutation_uint(void);
int tune_run_binomial_rn(void);
int tune_run_elem_mul(void);
int tune_run_mat22_mul(void);
int tune_run_mat22_sqr(void);
int tune_run_sqrt_invnewton(void);
int tune_run_divexact_basecase(void);
int tune_run_divexact_nn(void);

static const tune_module_t g_modules[] = {
    {"mul_toom22", "mul22,MUL_TOOM22_THRESHOLD", "MUL_TOOM22_THRESHOLD", tune_run_mul_toom22},
    {"mul_toom33", "mul33,MUL_TOOM33_THRESHOLD", "MUL_TOOM33_THRESHOLD", tune_run_mul_toom33},
    {"mul_toom44", "mul44,MUL_TOOM44_THRESHOLD", "MUL_TOOM44_THRESHOLD", tune_run_mul_toom44},
    {"mul_fft", "MUL_FFT_THRESHOLD", "MUL_FFT_THRESHOLD", tune_run_mul_fft},
    {"mullo_basecase", "mullo,MULLO_BASECASE_THRESHOLD", "MULLO_BASECASE_THRESHOLD", tune_run_mullo_basecase},
    {"mullo_dc", "MULLO_DC_THRESHOLD", "MULLO_DC_THRESHOLD", tune_run_mullo_dc},
    {"div_divide", "DIV_DIVIDE_THRESHOLD", "DIV_DIVIDE_THRESHOLD", tune_run_div_divide},
    {"bninv_newton", "bninv,BNINV_NEWTON_THRESHOLD", "BNINV_NEWTON_THRESHOLD", tune_run_bninv_newton},
    {"mul_fft_modf", "MUL_FFT_MODF_THRESHOLD", "MUL_FFT_MODF_THRESHOLD", tune_run_mul_fft_modf},
    {"mulhi_mersenne", "MULHI_MERSENNE_THRESHOLD", "MULHI_MERSENNE_THRESHOLD", tune_run_mulhi_mersenne},
    {"to_str_basepow", "TO_STR_BASEPOW_THRESHOLD", "TO_STR_BASEPOW_THRESHOLD", tune_run_to_str_basepow},
    {"to_str_divide", "TO_STR_DIVIDE_THRESHOLD", "TO_STR_DIVIDE_THRESHOLD", tune_run_to_str_divide},
    {"from_str_basepow", "FROM_STR_BASEPOW_THRESHOLD", "FROM_STR_BASEPOW_THRESHOLD", tune_run_from_str_basepow},
    {"from_str_divide", "FROM_STR_DIVIDE_THRESHOLD", "FROM_STR_DIVIDE_THRESHOLD", tune_run_from_str_divide},
    {"pow_1_exp", "pow1,POW_1_EXP_THRESHOLD", "POW_1_EXP_THRESHOLD", tune_run_pow_1_exp},
    {"pow_win2_exp", "POW_WIN2_EXP_THRESHOLD", "POW_WIN2_EXP_THRESHOLD", tune_run_pow_win2_exp},
    {"pow_win2_n", "POW_WIN2_N_THRESHOLD", "POW_WIN2_N_THRESHOLD", tune_run_pow_win2_n},
    {"factors_mul_n", "FACTORS_MUL_N_THRESHOLD", "FACTORS_MUL_N_THRESHOLD", tune_run_factors_mul_n},
    {"permutation_ushort", "npr_ushort,PERMUTATION_USHORT", "PERMUTATION_USHORT_K/B_THRESHOLD", tune_run_permutation_ushort},
    {"permutation_uint", "npr_uint,PERMUTATION_UINT", "PERMUTATION_UINT_K/B_THRESHOLD", tune_run_permutation_uint},
    {"binomial_rn", "ncr,BINOMIAL_RN_BASECASE_THRESHOLD", "BINOMIAL_RN_BASECASE_THRESHOLD", tune_run_binomial_rn},
    {"elem_mul", "elem,ELEM_MUL_BASECASE_THRESHOLD", "ELEM_MUL_BASECASE_THRESHOLD", tune_run_elem_mul},
    {"mat22_mul", "MAT22_MUL_STRASSEN_THRESHOLD", "MAT22_MUL_STRASSEN_THRESHOLD", tune_run_mat22_mul},
    {"mat22_sqr", "MAT22_SQR_STRASSEN_THRESHOLD", "MAT22_SQR_STRASSEN_THRESHOLD", tune_run_mat22_sqr},
    {"sqrt_invnewton", "SQRT_INVNEWTON_THRESHOLD", "SQRT_INVNEWTON_THRESHOLD", tune_run_sqrt_invnewton},
    {"divexact_basecase", "DIVEXACT_BASECASE_THRESHOLD", "DIVEXACT_BASECASE_THRESHOLD", tune_run_divexact_basecase},
    {"divexact_nn", "DIVEXACT_NN_THRESHOLD", "DIVEXACT_NN_THRESHOLD", tune_run_divexact_nn},
};

static void usage(void) {
    printf("Usage: lmmp_tune [options]\n");
    printf("  --only <id1,id2,...>   run only selected thresholds\n");
    printf("  --list                 list threshold module ids\n");
    printf("  --samples <N>          odd samples per path (default 7)\n");
    printf("  --min-ms <MS>          target duration per sample (default 10)\n");
    printf("  --mad-limit <X>        retry if MAD/median > X (default 0.25)\n");
    printf("  --max-retry <N>        max re-measure rounds (default 1)\n");
    printf("  --out <FILE.txt>       result text path\n");
    printf("  --write                write tuned values into mparam.h (with backup)\n");
    printf("  --help, -h             show this help\n");
}

static void list_modules(void) {
    for (size_t i = 0; i < sizeof(g_modules) / sizeof(g_modules[0]); ++i)
        printf("%-22s %s%s%s\n", g_modules[i].id, g_modules[i].title,
               g_modules[i].aliases != NULL ? "  aliases: " : "",
               g_modules[i].aliases != NULL ? g_modules[i].aliases : "");
}

int main(int argc, char** argv) {
    const char* out_txt = NULL;
    const char* mparam_path = LMMP_SOURCE_DIR "/include/lmmp/impl/mparam.h";
    const char* backup_path = LMMP_SOURCE_DIR "/include/lmmp/impl/mparam.h.tune-bak";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage();
            return 0;
        } else if (strcmp(argv[i], "--list") == 0) {
            list_modules();
            return 0;
        } else if (strcmp(argv[i], "--write") == 0) {
            g_tune.write = 1;
        } else if (strcmp(argv[i], "--only") == 0 && i + 1 < argc) {
            g_tune.only = argv[++i];
        } else if (strcmp(argv[i], "--samples") == 0 && i + 1 < argc) {
            const int v = atoi(argv[++i]);
            if (v < 1 || v > 31) {
                fprintf(stderr, "--samples must be in [1,31]\n");
                return 2;
            }
            g_tune.samples = (unsigned)(v | 1);
        } else if (strcmp(argv[i], "--min-ms") == 0 && i + 1 < argc) {
            const double v = atof(argv[++i]);
            if (v < 0.5 || v > 1000.0) {
                fprintf(stderr, "--min-ms must be in [0.5,1000]\n");
                return 2;
            }
            g_tune.target_ms = v;
        } else if (strcmp(argv[i], "--mad-limit") == 0 && i + 1 < argc) {
            const double v = atof(argv[++i]);
            if (!(v > 0.0) || v > 10.0) {
                fprintf(stderr, "--mad-limit must be in (0,10]\n");
                return 2;
            }
            g_tune.mad_limit = v;
        } else if (strcmp(argv[i], "--max-retry") == 0 && i + 1 < argc) {
            const int v = atoi(argv[++i]);
            if (v < 0 || v > 16) {
                fprintf(stderr, "--max-retry must be in [0,16]\n");
                return 2;
            }
            g_tune.max_retry = (unsigned)v;
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_txt = argv[++i];
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage();
            return 2;
        }
    }

    if (out_txt == NULL)
        out_txt = LMMP_SOURCE_DIR "/tune/lmmp/bin/lmmp_tune_results.txt";
    char* out_h = (char*)malloc(strlen(out_txt) + 4);
    if (out_h == NULL)
        return 2;
    strcpy(out_h, out_txt);
    char* dot = strrchr(out_h, '.');
    if (dot != NULL && strcmp(dot, ".txt") == 0)
        strcpy(dot, ".h");
    else
        strcat(out_h, ".h");

    tune_timer_init();
    lmmp_global_init();

    printf("LMMP tuning driver (high-density, GMP-style badness search)\n");
    printf("samples_per_path=%u target_ms=%.1f mad_limit=%.3f max_retry=%u write=%s\n",
           g_tune.samples, g_tune.target_ms, g_tune.mad_limit, g_tune.max_retry,
           g_tune.write ? "yes" : "no");
    printf("modules: %llu\n",
           (unsigned long long)(sizeof(g_modules) / sizeof(g_modules[0])));

    const int rc = tune_module_run(g_modules, sizeof(g_modules) / sizeof(g_modules[0]),
                                   g_tune.only);
    tune_record_print_summary();

    if (rc == 1) {
        fprintf(stderr, "No module matched --only filter.\n");
        list_modules();
    } else {
        if (tune_record_write_files(out_txt, out_h) == 0)
            printf("\nResults written to:\n  %s\n  %s\n", out_txt, out_h);
        else
            fprintf(stderr, "Failed to write result files.\n");
        if (rc == 0 && g_tune.write)
            tune_write_mparam(mparam_path, backup_path);
     }

    free(out_h);
    lmmp_global_deinit();
    return rc;
}
