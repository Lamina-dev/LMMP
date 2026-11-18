/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
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

#include "test.h"

static unsigned tests;
static unsigned failed_tests;
static unsigned checks;
static unsigned failures;

void lmmp_test_check(bool ok, const char *file, int line, const char *expr, const char *message) {
    ++checks;
    if (ok)
        return;
    ++failures;
    fprintf(stderr, "%s:%d: %s", file, line, expr);
    if (message)
        fprintf(stderr, " (%s)", message);
    fputc('\n', stderr);
}

void lmmp_test_run(const char *name, void (*test)(void)) {
    unsigned before = failures;
    ++tests;
    printf("[ RUN    ] %s\n", name);
    test();
    if (failures != before)
        ++failed_tests;
    printf("[ %s ] %s\n", failures == before ? "PASSED" : "FAILED", name);
}

int lmmp_test_finish(void) {
    printf("TOTAL: %u tests, %u failed, %u checks\n", tests, failed_tests, checks);
    return failed_tests != 0;
}
