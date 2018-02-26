#ifndef _PARSER_H
#define _PARSER_H

#define MAX_HEADERS 100
#define MAX_LEN_HEADER 2048

typedef struct http_headers {
    char name[MAX_LEN_HEADER];
    char value[MAX_LEN_HEADER];
} http_headers_t;

int request_parser(char *buf, size_t buflen, char *method, char *path, int *version, struct http_headers *headers, int *num_headers);
int response_parser(char *buf, size_t buflen, int *version, int *rescode, char *resp, struct http_headers *headers, int *num_headers);

#endif
