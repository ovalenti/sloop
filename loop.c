/*
 * loop.c
 *
 *  Created on: Oct 6, 2021
 *      Author: ovalentin
 */

#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sloop/loop.h>

static LIST_HEAD(watches);
static LIST_HEAD(timeouts);
static struct pollfd* poll_fds = NULL;
static nfds_t poll_fds_len     = 0;
static int quit                = 0;

static long long time_now()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    return ((long long)tp.tv_sec) * 1000 + tp.tv_nsec / 1000000;
}

void loop_watch_set(struct loop_watch* watch, enum loop_io_event events)
{
    if (!!watch->events != !!events) {
        if (events)
            list_add(&watch->list, &watches);
        else
            list_del(&watch->list);
    }
    watch->events = events;
}

void loop_timeout_add(struct loop_timeout* timeout, int delay_ms)
{
    struct list_head* insert_location;

    loop_timeout_cancel(timeout);

    timeout->date = time_now() + delay_ms;

    list_for_each(insert_location,
                  &timeouts) if (container_of(insert_location, struct loop_timeout, list)->date > timeout->date) break;

    list_add_tail(&timeout->list, insert_location);

    timeout->registered = 1;
}
void loop_timeout_cancel(struct loop_timeout* timeout)
{
    if (!timeout->registered)
        return;

    list_del(&timeout->list);

    timeout->registered = 0;
}

static int next_timeout()
{
    long long now, next;

    if (list_empty(&timeouts))
        return -1;

    now  = time_now();
    next = list_first_entry(&timeouts, struct loop_timeout, list)->date;
    return now > next ? 0 : next - now;
}

void loop_run()
{
    while (!quit) {
        int rc, nfds = 0;
        struct loop_watch* watch;
        struct loop_timeout* timeout;
        LIST_HEAD(expired);
        long long now;

        list_for_each_entry(watch, &watches, list)
        {
            if (nfds == poll_fds_len) {
                poll_fds_len += 16;
                poll_fds = (struct pollfd*)realloc(poll_fds, poll_fds_len);
            }

            memset(&poll_fds[nfds], 0, sizeof(struct pollfd));
            poll_fds[nfds].fd = watch->fd;
            if (watch->events & EVENT_READ)
                poll_fds[nfds].events |= POLLIN;
            if (watch->events & EVENT_WRITE)
                poll_fds[nfds].events |= POLLOUT;

            nfds++;
        }

        rc = poll(poll_fds, nfds, next_timeout());

        now = time_now();

        while (!list_empty(&timeouts) && (timeout = list_first_entry(&timeouts, struct loop_timeout, list))->date < now)
            list_move_tail(&timeout->list, &expired);

        while (!list_empty(&expired)) {
            timeout = list_first_entry(&expired, struct loop_timeout, list);
            loop_timeout_cancel(timeout);
            timeout->cb(timeout);
        }

        if (rc < 0)
            continue;

        while (nfds--) {
            if (!poll_fds[nfds].revents)
                continue;

            list_for_each_entry(watch, &watches, list)
            {
                if (watch->fd == poll_fds[nfds].fd) {
                    enum loop_io_event events = 0;
                    if (poll_fds[nfds].revents & (POLLIN | POLLERR | POLLNVAL))
                        events |= EVENT_READ;
                    if (poll_fds[nfds].revents & POLLOUT)
                        events |= EVENT_WRITE;

                    watch->cb(watch, events);
                    break;
                }
            }
        }
    }
}
void loop_exit() { quit = 1; }
