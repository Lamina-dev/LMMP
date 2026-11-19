/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef LMMP_TUNE_COMMON_H
#define LMMP_TUNE_COMMON_H

#include <stdbool.h>
#include <stdio.h>

#include "lmmp/lmmp.h"

typedef enum { LMMP_TUNE_RUN, LMMP_TUNE_INFO, LMMP_TUNE_CURRENT, LMMP_TUNE_HELP } lmmp_tune_action;

typedef struct {
    lmmp_tune_action action;
    unsigned samples;
    double min_ms;
    double mad_limit;
    unsigned max_retry;
    unsigned long long seed;
    const char* output_path;
} lmmp_tune_options;

typedef struct {
    void (*bind)(void* context);
    void (*reset)(void* context);
    mp_limb_t (*call)(void* context);
    void* context;
} lmmp_tune_bench;

void lmmp_tune_options_init(lmmp_tune_options* options);
bool lmmp_tune_parse(int argc, char** argv, lmmp_tune_options* options);
void lmmp_tune_usage(const char* program_id);
FILE* lmmp_tune_open_output(const lmmp_tune_options* options);
void lmmp_tune_close_output(const lmmp_tune_options* options, FILE* output);

bool lmmp_tune_measure(const lmmp_tune_bench* bench,
                       const lmmp_tune_options* options,
                       double* median_us,
                       double* mad_ratio,
                       unsigned* retries);

#endif
