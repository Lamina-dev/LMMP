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

#include <cstdio>
#include <cstring>
#include <string>

#include "lmmp/lmmp.h"
#include "lmmp_measure.hpp"

int main(int argc, char** argv) {
    std::string filter;
    bool list = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            filter = argv[++i];
        } else if (std::strcmp(argv[i], "--list") == 0) {
            list = true;
        } else {
            std::printf("Usage: LmmpBenchmark [--filter <substr>] [--list]\n");
            return 1;
        }
    }

    if (list) {
        lmmp_measure::list_all();
        return 0;
    }

    lmmp_global_init();
    int rc = lmmp_measure::run_all(filter);
    lmmp_global_deinit();
    return rc;
}
