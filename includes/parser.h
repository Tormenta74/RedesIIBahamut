#ifndef _PARSER_H
#define _PARSER_H

#define MAX_HEADERS 100
#define MAX_LEN_HEADER 2048

typedef struct request_headers {
    char name[MAX_LEN_HEADER];
    char value[MAX_LEN_HEADER];
} request_headers_t;

int request_parser(char *buf, size_t buflen, char *method, char *path, int *version, struct request_headers *headers, int *num_headers);

#endif
