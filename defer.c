/*
 * defer.c
 *
 *  Created on: Nov 16, 2021
 *      Author: ovalentin
 */

#include <stdlib.h>
#include <unistd.h>

#include "loop.h"

#include "defer.h"

static LIST_HEAD(deferred_calls);
static struct loop_timeout deferred_trigger = { 0 };

struct deferred_call {
    void (*fnct)(void* data);
    void* data;
    struct list_head list;
};

static void deferred_trigger_cb(struct loop_timeout* timeout)
{
    LIST_HEAD(calls_to_execute);

    list_splice_init(&deferred_calls, &calls_to_execute);

    while (!list_empty(&calls_to_execute)) {
        struct deferred_call* call = list_first_entry(&calls_to_execute, struct deferred_call, list);
        call->fnct(call->data);
        list_del(&call->list);
        free(call);
    }
}

void defer_call(void (*fnct)(void* data), void* data)
{
    struct deferred_call* call = (struct deferred_call*)malloc(sizeof(struct deferred_call));

    call->fnct = fnct;
    call->data = data;

    list_add(&call->list, &deferred_calls);

    deferred_trigger.cb = &deferred_trigger_cb;
    loop_timeout_add(&deferred_trigger, 0);
}
