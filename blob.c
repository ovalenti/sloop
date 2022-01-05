/*
 * blob.c
 *
 *  Created on: Oct 7, 2021
 *      Author: ovalentin
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"

#define HDR(type, len) ((type << 16) | (len))
#define TYPE(hdr) ((hdr) >> 16)
#define LENGTH(hdr) ((hdr)&0xffff)
#define AT(blob_ptr, off) (struct blob*)(((char*)blob_ptr) + off)

static uint32_t ntoh_u32(struct blob* blob)
{
    unsigned char* c = (unsigned char*)blob;
    uint32_t i;

    i = *(c++);
    i <<= 8;
    i |= *(c++);
    i <<= 8;
    i |= *(c++);
    i <<= 8;
    i |= *c;

    return i;
}

static void hton_u32(struct blob* blob, uint32_t i)
{
    unsigned char* c = (unsigned char*)blob;

    c[3] = i;
    i >>= 8;
    c[2] = i;
    i >>= 8;
    c[1] = i;
    i >>= 8;
    c[0] = i;
}

static void msg_make_room(struct msg* msg, int free_space)
{
    if (msg->alloc < msg->ptr + free_space) {
        msg->alloc = (((msg->ptr + free_space) / 64) + 1) * 64;
        msg->blob  = realloc(msg->blob, msg->alloc);
    }
}

static void msg_make_hdr(struct msg* msg, enum blob_type type, const char* name, int payload)
{
    struct blob* blob;
    int hdr_len;

    if (name == NULL)
        name = "";

    hdr_len = 4 + strlen(name) + 1;

    msg_make_room(msg, hdr_len + payload);
    blob = AT(msg->blob, msg->ptr);

    hton_u32(blob, HDR(type, hdr_len + payload));

    strcpy((char*)AT(blob, 4), name);

    msg->ptr += hdr_len;
}

msg_nest_cookie msg_nest_open(struct msg* msg, const char* name)
{
    msg_nest_cookie cookie = msg->ptr;

    msg_make_hdr(msg, BLOB_TYPE_NEST, name, 0);

    return cookie;
}

void msg_nest_close(struct msg* msg, msg_nest_cookie cookie)
{
    hton_u32(AT(msg->blob, cookie), HDR(BLOB_TYPE_NEST, msg->ptr - cookie));
}

void msg_int_append(struct msg* msg, const char* name, int i)
{
    msg_make_hdr(msg, BLOB_TYPE_INT, name, 4);

    hton_u32(AT(msg->blob, msg->ptr), i);
    msg->ptr += 4;
}

void msg_str_append(struct msg* msg, const char* name, const char* str)
{
    int str_len = strlen(str) + 1;

    msg_make_hdr(msg, BLOB_TYPE_STR, name, str_len);

    strcpy((char*)AT(msg->blob, msg->ptr), str);
    msg->ptr += str_len;
}

enum blob_type blob_type(struct blob* blob) { return TYPE(ntoh_u32(blob)); }

struct blob* blob_next(struct blob* blob) { return AT(blob, LENGTH(ntoh_u32(blob))); }

int blob_fits_parent(struct blob* blob, struct blob* parent)
{
    return (parent <= blob) && (AT(blob, 4) < blob_next(parent)) && (blob_next(blob) <= blob_next(parent));
}

int blob_fits(struct blob* blob, int len) { return len >= 4 && blob_len(blob) <= len; }

int blob_len(struct blob* blob) { return LENGTH(ntoh_u32(blob)); }

char* blob_name(struct blob* blob) { return (char*)AT(blob, 4); }

static int blob_hdr_len(struct blob* blob)
{
    char* bytes = (char*)blob;
    int ptr     = 4;
    int limit   = LENGTH(ntoh_u32(blob)) - 1;

    while (ptr < limit && bytes[ptr] != '\0')
        ptr++;

    return ptr + 1;
}

int blob_int_get(struct blob* blob, int* i)
{
    int hdr_len = blob_hdr_len(blob);

    if (LENGTH(ntoh_u32(blob)) < hdr_len + 4)
        return -1;

    *i = ntoh_u32(AT(blob, hdr_len));
    return 0;
}

int blob_str_get(struct blob* blob, char** str)
{
    int hdr_len  = blob_hdr_len(blob);
    int blob_len = LENGTH(ntoh_u32(blob));

    if (blob_len < hdr_len + 1)
        return -1;

    *str                           = (char*)AT(blob, hdr_len);
    *(char*)AT(blob, blob_len - 1) = '\0';

    return 0;
}

int blob_nest_get(struct blob* blob, struct blob** first_child)
{
    *first_child = AT(blob, blob_hdr_len(blob));

    return 0;
}

int blob_nest_parse(struct blob* blob, char** mapping, struct blob** result, int maplen)
{
    struct blob* child;

    memset(result, 0, sizeof(struct blob*) * maplen);

    if (blob_type(blob) != BLOB_TYPE_NEST)
        return -1;

    if (blob_nest_get(blob, &child))
        return -1;

    while (blob_fits_parent(child, blob)) {
        int i;
        for (i = 0; i < maplen; i++) {
            if (!strcmp(mapping[i], blob_name(child))) {
                if (result[i])
                    return -1;

                result[i] = child;
                break;
            }
        }
        child = blob_next(child);
    }

    return 0;
}
