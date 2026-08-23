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
#include "lmmp_test.hpp"

int main(int argc, char** argv) {
    std::string filter;
    bool list = false;
    bool help = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            filter = argv[++i];
        } else if (std::strcmp(argv[i], "--list") == 0) {
            list = true;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            help = true;
        } else {
            std::printf("Unknown argument: %s\n", argv[i]);
            help = true;
        }
    }

    if (help) {
        std::printf("Usage: LmmpTest [--filter <substr>] [--list] [--help]\n");
        std::printf("  --filter <substr>  Run only tests whose category/name contains substr.\n");
        std::printf("  --list             List all test cases.\n");
        return 1;
    }

    if (list) {
        lmmp_test::list_all();
        return 0;
    }

    lmmp_global_init();
    int rc = lmmp_test::run_all(filter);
    lmmp_global_deinit();
    return rc;
}
