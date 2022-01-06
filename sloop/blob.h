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

struct blob;

struct msg {
    struct blob* blob;
    int alloc;

    int ptr;
};

msg_nest_cookie msg_nest_open(struct msg* msg, const char* name);
void msg_nest_close(struct msg* msg, msg_nest_cookie cookie);

void msg_int_append(struct msg* msg, const char* name, int i);
void msg_str_append(struct msg* msg, const char* name, const char* str);

enum blob_type blob_type(struct blob* blob);
struct blob* blob_next(struct blob* blob);
int blob_fits_parent(struct blob* blob, struct blob* parent);
int blob_fits(struct blob* blob, int len);
int blob_len(struct blob* blob);
char* blob_name(struct blob* blob);

int blob_int_get(struct blob* blob, int* i);
int blob_str_get(struct blob* blob, char** str);
int blob_nest_get(struct blob* blob, struct blob** first_child);

int blob_nest_parse(struct blob* blob, char** mapping, struct blob** result, int maplen);

#endif /* BLOB_H_ */
