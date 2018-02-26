#ifndef _PARSER_H
#define _PARSER_H

#define MAX_HEADERS 100

typedef struct request_headers {
   char name[2048];
   char value[2048];
} request_headers_t;

int request_parser(char *buf, size_t buflen, char *method, char *path, int *version, struct request_headers *headers, int *num_headers);

#endif
