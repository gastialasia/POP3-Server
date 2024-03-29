#ifndef COMPARATOR_H
#define COMPARATOR_H

#include <stdio.h>
#include "../include/buffer.h"
#include "../include/pop3nio.h"
#include "../include/server_messages.h"

typedef unsigned int (*fn_type)(buffer *, struct pop3 *, char *arg1, char *arg2);

unsigned int auth_capa_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int trans_capa_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int user_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int invalid_command_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int pass_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int noop(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int stat_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int list_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int rset_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int dele_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int retr_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int auth_quit_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

unsigned int trans_quit_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2);

fn_type comparator(struct parser_event *pe, unsigned int curr_state);

#endif