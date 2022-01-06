/*
 * loop.h
 *
 *  Created on: Oct 6, 2021
 *      Author: ovalentin
 */

#ifndef LOOP_H_
#define LOOP_H_

#include <sloop/list.h>

enum loop_io_event {
    EVENT_READ  = 1 << 0,
    EVENT_WRITE = 1 << 1,
};

struct loop_watch {
    int fd;
    void (*cb)(struct loop_watch* watch, enum loop_io_event events);

    int events;
    struct list_head list;
};

void loop_watch_set(struct loop_watch* watch, enum loop_io_event events);

struct loop_timeout {
    void (*cb)(struct loop_timeout* timeout);

    long long date;
    int registered;
    struct list_head list;
};

void loop_timeout_add(struct loop_timeout* timeout, int delay_ms);
void loop_timeout_cancel(struct loop_timeout* timeout);

void loop_run();
void loop_exit();

#endif /* LOOP_H_ */
