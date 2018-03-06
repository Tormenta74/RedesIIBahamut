#ifndef _HEADERS_H
#define _HEADERS_H

#include "config.h"
#include "http.h"

char *header_date();
char *header_server(struct server_options so);
char *header_last_modified(char *path);
int header_build(struct server_options so, char *path, char *contenttype, long len, int check_flag, int options_flag, struct http_pairs *headers, int *num_headers);

//nope
char *date_generator(time_t *t);

#endif /*_HEADERS_H*/
