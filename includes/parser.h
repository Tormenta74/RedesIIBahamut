#ifndef _PARSER_H
#define _PARSER_H

#define MAX_HEADERS 100

typedef struct request_headers {
   char *name;
   char *value;
   int num_headers;
} request_headers_t;

int request_parser(const char *buf, int buflen, char **method, char **path, int *version, struct request_headers *headers);

#endif
