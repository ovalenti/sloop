/*
 * blob.c
 *
 *  Created on: Oct 7, 2021
 *      Author: ovalentin
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sloop/blob.h>

#define HDR(type, len) ((type << 16) | (len))
#define TYPE(hdr) ((hdr) >> 16)
#define LENGTH(hdr) ((hdr)&0xffff)
#define AT(blob_ptr, off) (struct sloop_blob*)(((char*)blob_ptr) + off)

static uint32_t ntoh_u32(struct sloop_blob* blob)
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

static void hton_u32(struct sloop_blob* blob, uint32_t i)
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

static void msg_make_room(struct sloop_msg* msg, int free_space)
{
    if (msg->alloc < msg->ptr + free_space) {
        msg->alloc = (((msg->ptr + free_space) / 64) + 1) * 64;
        msg->blob  = realloc(msg->blob, msg->alloc);
    }
}

static void msg_make_hdr(struct sloop_msg* msg, enum blob_type type, const char* name, int payload)
{
    struct sloop_blob* blob;
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

msg_nest_cookie msg_nest_open(struct sloop_msg* msg, const char* name)
{
    msg_nest_cookie cookie = msg->ptr;

    msg_make_hdr(msg, BLOB_TYPE_NEST, name, 0);

    return cookie;
}

void msg_nest_close(struct sloop_msg* msg, msg_nest_cookie cookie)
{
    hton_u32(AT(msg->blob, cookie), HDR(BLOB_TYPE_NEST, msg->ptr - cookie));
}

void msg_int_append(struct sloop_msg* msg, const char* name, int i)
{
    msg_make_hdr(msg, BLOB_TYPE_INT, name, 4);

    hton_u32(AT(msg->blob, msg->ptr), i);
    msg->ptr += 4;
}

void msg_str_append(struct sloop_msg* msg, const char* name, const char* str)
{
    int str_len = strlen(str) + 1;

    msg_make_hdr(msg, BLOB_TYPE_STR, name, str_len);

    strcpy((char*)AT(msg->blob, msg->ptr), str);
    msg->ptr += str_len;
}

enum blob_type blob_type(struct sloop_blob* blob) { return TYPE(ntoh_u32(blob)); }

struct sloop_blob* blob_next(struct sloop_blob* blob) { return AT(blob, LENGTH(ntoh_u32(blob))); }

int blob_fits_parent(struct sloop_blob* blob, struct sloop_blob* parent)
{
    return (parent <= blob) && (AT(blob, 4) < blob_next(parent)) && (blob_next(blob) <= blob_next(parent));
}

int blob_fits(struct sloop_blob* blob, int len) { return len >= 4 && blob_len(blob) <= len; }

int blob_len(struct sloop_blob* blob) { return LENGTH(ntoh_u32(blob)); }

char* blob_name(struct sloop_blob* blob) { return (char*)AT(blob, 4); }

static int blob_hdr_len(struct sloop_blob* blob)
{
    char* bytes = (char*)blob;
    int ptr     = 4;
    int limit   = LENGTH(ntoh_u32(blob)) - 1;

    while (ptr < limit && bytes[ptr] != '\0')
        ptr++;

    return ptr + 1;
}

int blob_int_get(struct sloop_blob* blob, int* i)
{
    int hdr_len = blob_hdr_len(blob);

    if (LENGTH(ntoh_u32(blob)) < hdr_len + 4)
        return -1;

    *i = ntoh_u32(AT(blob, hdr_len));
    return 0;
}

int blob_str_get(struct sloop_blob* blob, char** str)
{
    int hdr_len  = blob_hdr_len(blob);
    int blob_len = LENGTH(ntoh_u32(blob));

    if (blob_len < hdr_len + 1)
        return -1;

    *str                           = (char*)AT(blob, hdr_len);
    *(char*)AT(blob, blob_len - 1) = '\0';

    return 0;
}

int blob_nest_get(struct sloop_blob* blob, struct sloop_blob** first_child)
{
    *first_child = AT(blob, blob_hdr_len(blob));

    return 0;
}

int blob_nest_parse(struct sloop_blob* blob, char** mapping, struct sloop_blob** result, int maplen)
{
    struct sloop_blob* child;

    memset(result, 0, sizeof(struct sloop_blob*) * maplen);

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
