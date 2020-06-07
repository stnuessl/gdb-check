/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Steffen Nuessle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdbool.h>

#include "tasks.h"

struct data {
    bool ready;
    int num;
};

static volatile struct data data_10ms;
static volatile struct data data_100ms;

static int popcnt(int n)
{
    int val = 0;

    while (n) {
        n &= n - 1;
        ++val;
    }

    return val;
}

static int lzcnt(int n)
{
    int val = 0;

    if (n == 0)
        return 32;

    while (n > 0) {
        n <<= 1;
        ++val;
    }

    return val;
}

void task_10ms(void)
{
    if (data_10ms.ready) {
        int val = popcnt(data_10ms.num);

        fprintf(stdout, "popcnt(%d) = %d\n", data_10ms.num, val);

        data_10ms.ready = false;
    }
}

void task_100ms(void)
{
    if (data_100ms.ready) {
        int val = lzcnt(data_100ms.num);

        fprintf(stdout, "lzcnt(%d) = %d\n", data_100ms.num, val);

        data_100ms.ready = false;
    }
}


