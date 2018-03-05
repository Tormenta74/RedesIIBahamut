#ifndef _HTTP_H
#define _HTTP_H

#define MAX_HEADERS 32
#define MAX_ARGS 32
#define MAX_LEN_HEADER 512

typedef struct http_pairs {
    char name[MAX_LEN_HEADER];
    char value[MAX_LEN_HEADER];
} http_pairs_t;

typedef struct http_req_data {
    int version;
    char *method;
    char *path;
    size_t path_len;
    char *body;
    size_t body_len;
    int num_headers;
    struct http_pairs headers[MAX_HEADERS];
} http_req_data_t;

// depr
typedef struct http_args_data {
    int num_pairs;
    struct http_pairs args[MAX_ARGS];
} http_args_data_t;

void http_request_data_print(struct http_req_data *req_data);
void http_request_data_free(struct http_req_data *req_data);

int http_request_parse(char *buf, size_t buflen, struct http_req_data *req_data);

int http_request_body(char *buf, char **body);
int http_request_get_split(char *buf, size_t buflen, char **path, char **args, size_t *args_len);
int http_response_build(char** buffer, int version, int rescode, char *resp, size_t resp_len, int num_headers, struct http_pairs *headers, char *body, size_t body_len);

// not used
//int response_parser(char *buf, size_t buflen, int *version, int *rescode, char *resp, struct http_pairs *headers, int *num_headers);
//int argument_parser(char *buf, struct http_args_data *arguments);
//int request_argument_parser(char *method, char *buffer, struct http_args_data *args);
//// depr
//int http_request_parse_old(char *buf, size_t buflen, char *method, char *path, int *version, struct http_pairs *headers, int *num_headers);

#endif
