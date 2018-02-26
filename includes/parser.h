#ifndef _PARSER_H
#define _PARSER_H

#define MAX_HEADERS 32
#define MAX_LEN_HEADER 512

typedef struct http_headers {
    char name[MAX_LEN_HEADER];
    char value[MAX_LEN_HEADER];
} http_headers_t;

typedef struct http_req_data {
    int version;
    char *method;
    char *path;
    int num_headers;
    struct http_headers headers[MAX_HEADERS];
} http_req_data_t;

void request_data_print(struct http_req_data *req_data);
void request_data_free(struct http_req_data *req_data);

int request_parser_new(char *buf, size_t buflen, struct http_req_data *req_data);

int request_parser(char *buf, size_t buflen, char *method, char *path, int *version, struct http_headers *headers, int *num_headers);
int response_parser(char *buf, size_t buflen, int *version, int *rescode, char *resp, struct http_headers *headers, int *num_headers);
int response_builder(char* buffer, int version, int rescode, char *resp, size_t resp_len, int num_headers, struct http_headers *headers, char *body, size_t body_len);

#endif
