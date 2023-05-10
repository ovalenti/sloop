/*
 * msg.h
 *
 *  Created on: Oct 7, 2021
 *      Author: ovalentin
 */

#ifndef BLOB_H_
#define BLOB_H_

enum blob_type { BLOB_TYPE_INT = 0, BLOB_TYPE_STR, BLOB_TYPE_NEST };

typedef int msg_nest_cookie;

struct sloop_blob;

struct sloop_msg {
    struct sloop_blob* blob;
    int alloc;

    int ptr;
};

msg_nest_cookie msg_nest_open(struct sloop_msg* msg, const char* name);
void msg_nest_close(struct sloop_msg* msg, msg_nest_cookie cookie);

void msg_int_append(struct sloop_msg* msg, const char* name, int i);
void msg_str_append(struct sloop_msg* msg, const char* name, const char* str);

enum blob_type blob_type(struct sloop_blob* blob);
struct sloop_blob* blob_next(struct sloop_blob* blob);
int blob_fits_parent(struct sloop_blob* blob, struct sloop_blob* parent);
int blob_fits(struct sloop_blob* blob, int len);
int blob_len(struct sloop_blob* blob);
char* blob_name(struct sloop_blob* blob);

int blob_int_get(struct sloop_blob* blob, int* i);
int blob_str_get(struct sloop_blob* blob, char** str);
int blob_nest_get(struct sloop_blob* blob, struct sloop_blob** first_child);

int blob_nest_parse(struct sloop_blob* blob, char** mapping, struct sloop_blob** result, int maplen);

#endif /* BLOB_H_ */
