/*
 * stream_buffer.c
 *
 *  Created on: Oct 7, 2021
 *      Author: ovalentin
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sloop/sb.h>

struct chunk {
    char* data;
    int len;
    struct list_head list;
};

static void chunk_append(struct sb* sb, char* data, int len)
{
    struct chunk* chunk = (struct chunk*)malloc(sizeof(struct chunk));

    chunk->data = data;
    chunk->len  = len;

    list_add(&chunk->list, &sb->out);
}

static void chunk_del(struct chunk* buffer)
{
    list_del(&buffer->list);
    free(buffer->data);
    free(buffer);
}

static void update_subscription(struct sb* sb)
{
    enum loop_io_event events = 0;

    if (sb->attached) {
        if (!list_empty(&sb->out))
            events |= EVENT_WRITE;

        if (!sb->block)
            events |= EVENT_READ;
    }

    loop_watch_set(&sb->watch, events);
}

static void pump_in(struct sb* sb)
{
    int rc;

    if (sb->in_buffer_alloc <= sb->in_buffer_len + 1)
        sb->in_buffer = realloc(sb->in_buffer, sb->in_buffer_alloc += 1024);

    rc = read(sb->watch.fd, sb->in_buffer, sb->in_buffer_alloc - sb->in_buffer_len);

    if (rc < 0 && errno != EINTR) {
        sb->eos(sb, 1);
    } else if (rc == 0) {
        if (sb->in_buffer_len)
            sb->data_available(sb);
        sb->eos(sb, 0);
    } else {
        sb->in_buffer_len += rc;
        sb->data_available(sb);
    }
}
static void pump_out(struct sb* sb)
{
    int rc;
    struct chunk* chunk;

    if (list_empty(&sb->out))
        return;

    chunk = list_first_entry(&sb->out, struct chunk, list);

    rc = write(sb->watch.fd, chunk->data + sb->out_buffer_ptr, chunk->len - sb->out_buffer_ptr);

    if (rc >= 0)
        sb->out_buffer_ptr += rc;

    if (sb->out_buffer_ptr == chunk->len) {
        chunk_del(chunk);
        sb->out_buffer_ptr = 0;
    }
}

static void watch_cb(struct loop_watch* watch, enum loop_io_event events)
{
    struct sb* sb = container_of(watch, struct sb, watch);

    if (events & EVENT_WRITE)
        pump_out(sb);

    if (events & EVENT_READ)
        pump_in(sb);

    update_subscription(sb);
}

void sb_attach(struct sb* sb)
{
    sb->attached = 1;
    sb->watch.cb = &watch_cb;
    INIT_LIST_HEAD(&sb->out);

    update_subscription(sb);
}

void sb_detach(struct sb* sb)
{
    sb->attached = 0;

    while (!list_empty(&sb->out))
        chunk_del(list_first_entry(&sb->out, struct chunk, list));

    if (sb->in_buffer)
        free(sb->in_buffer);

    update_subscription(sb);
}

void sb_in_block(struct sb* sb, int block)
{
    sb->block = block;
    update_subscription(sb);
}

void sb_in_peek(struct sb* sb, char** data, int* len)
{
    *data = sb->in_buffer;
    *len  = sb->in_buffer_len;
}

void sb_in_take(struct sb* sb, int len)
{
    sb->in_buffer_len -= len;
    if (sb->in_buffer_len)
        memmove(sb->in_buffer, sb->in_buffer + len, sb->in_buffer_len);
}

void sb_out_steal(struct sb* sb, char* data, int len)
{
    chunk_append(sb, data, len);

    update_subscription(sb);
}

void sb_out(struct sb* sb, const char* data, int len)
{
    char* copy = (char*)malloc(len);
    memcpy(copy, data, len);

    sb_out_steal(sb, copy, len);
}
