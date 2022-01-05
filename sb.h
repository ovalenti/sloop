/*
 * Stream buffers.
 *
 * When a SB is attached to a watch:
 * - it reads everything to a buffer, which can be polled and consumed
 * - it stores all the writes and forward them asap
 *
 */

#ifndef SB_H_
#define SB_H_

#include "list.h"
#include "loop.h"

struct sb {
    void (*data_available)(struct sb* sb);
    void (*eos)(struct sb* sb, int error);
    struct loop_watch watch; // fd is a parameter

    int block;
    int attached;
    char* in_buffer;
    int in_buffer_len;
    int in_buffer_alloc;
    struct list_head out;
    int out_buffer_ptr;
};

void sb_attach(struct sb* sb);
void sb_detach(struct sb* sb);

void sb_in_block(struct sb* sb, int block);
void sb_in_peek(struct sb* sb, char** data, int* len);
void sb_in_take(struct sb* sb, int len);

void sb_out(struct sb* sb, const char* data, int len);
void sb_out_steal(struct sb* sb, char* data, int len);

#endif /* SB_H_ */
