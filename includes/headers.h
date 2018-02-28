#ifndef _HEADERS_H
#define _HEADERS_H

#include "config.h"

char *header_date();
char *header_server(struct server_options so);
char *header_last_modified(char *path);
char *header_content_length();
char *header_content_type();
char *response_craft(int version, char *content);

//nope
char *date_generator(time_t *t);

#endif /*_HEADERS_H*/
