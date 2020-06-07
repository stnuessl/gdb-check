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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "tasks.h"

#define ARRAY_SIZE(x_) (sizeof((x_)) / sizeof(*(x_)))

struct callback {
    void (*fn)(void);
};

static int tfd_10ms;
static int tfd_100ms;

__attribute__((noreturn, format(printf, 1, 2)))
void die(const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);

    exit(EXIT_FAILURE);
}

void stdin_run(void)
{
    char buf[128];

    /* 
     * Read all data from stdin as unprocessed data gets passed to
     * the parent process and may influence it.
     */

    do {
        ssize_t n = read(STDIN_FILENO, buf, ARRAY_SIZE(buf));
        if (n < 0) {
            if (errno == EAGAIN)
                break;

            die("read() failed\n");
        }

        /* For non-blocking file descriptors, "n" cannot be zero */
        if (n > 0) {
            /* We expect line based input, overwrite the newline character */
            buf[n - 1] = '\0';

            if (!!strcasestr(buf, "q") || !!strcasestr(buf, "exit"))
                exit(EXIT_SUCCESS);
        }
    } while (1);

}

static int timer_init(int efd, int ms, struct callback *callback)
{
    struct itimerspec time;
    struct epoll_event ev;
    int tfd, err;

    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (tfd < 0) 
        die("timerfd_create() failed\n");

    time.it_interval.tv_sec = 0;
    time.it_interval.tv_nsec = ms * 1000 * 1000;
    time.it_value.tv_sec = 0;
    time.it_value.tv_nsec = ms * 1000 * 1000;

    err = timerfd_settime(tfd, 0, &time, NULL);
    if (err < 0)
        die("timerfd_settime() failed\n");
 
    ev.events = EPOLLIN;
    ev.data.ptr = callback;

    err = epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &ev);
    if (err < 0)
        die("epoll_ctl(): failed to add timer\n");

    return tfd;
}

static void timer_clear(int fd)
{
    uint64_t val;
    ssize_t n;

    n = read(fd, &val, sizeof(val));
    if (n != sizeof(val))
        die("read() failed\n");

}

static void tfd_10ms_run(void)
{
    timer_clear(tfd_10ms);
    task_10ms();
}

static void tfd_100ms_run(void)
{
    timer_clear(tfd_100ms);
    task_100ms();
}
    
static struct callback callback_stdin = { 
    .fn = &stdin_run,
};

static struct callback callback_timer_10ms = {
    .fn = &tfd_10ms_run,
};

static struct callback callback_timer_100ms = {
    .fn = &tfd_100ms_run,
};

static void stdin_init(int efd, struct callback *callback)
{
    struct epoll_event ev;
    int flags, err;

    /* Make stdin non-blocking */
    flags = fcntl(STDIN_FILENO, F_GETFL);
    if (flags < 0)
        die("fcntl(): retrieving flags failed\n");

    if (!(flags & O_NONBLOCK)) {
        err = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        if (err < 0)
            die("fcntl(): setting flags failed\n");
    }

    ev.events = EPOLLIN;
    ev.data.ptr = callback;

    err = epoll_ctl(efd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    if (err < 0)
        die("epoll_ctl(): failed to add stdin\n");
}

int main(int argc, char *argv[])
{
    int efd;

    efd = epoll_create1(EPOLL_CLOEXEC);
    if (efd < 0)
        die("epoll_create() failed\n");

    tfd_10ms = timer_init(efd, 10, &callback_timer_10ms);
    tfd_100ms = timer_init(efd, 100, &callback_timer_100ms);
 
    stdin_init(efd, &callback_stdin);

    while (1) {
        struct epoll_event events[2];
        int n;

        n = epoll_wait(efd, events, ARRAY_SIZE(events), -1);
        if (n < 0)
            die("epoll_wait()");

        for (int i = 0; i < n; ++i) 
            ((struct callback *) events[i].data.ptr)->fn();
    }

    return EXIT_SUCCESS;
}
