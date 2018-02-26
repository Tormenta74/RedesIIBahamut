#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strcat, strcpy
#include <time.h>

#include "globals.h"
#include "headers.h"
#include "parser.h"
#include "picohttpparser.h"

// returns 
char *header_date() {
    time_t rawtime;
    struct tm *timeinfo;
    size_t nformatted;
    char buffer[128], *date_buf;

    time(&rawtime);
    timeinfo = gmtime(&rawtime);

    nformatted = strftime(buffer, 128, "%a, %d %b %Y %T %Z", timeinfo);
    if(nformatted == 0) {
        return NULL;
    }

    date_buf = strndup(buffer, nformatted);

    return date_buf;
}

/* wraps phr_parse_request and returns required information in a simple way */
int request_parser_new(char *buf, size_t buflen, struct http_req_data *req_data) {
    char *method_aux, *path_aux;
    size_t method_len, path_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    ret = phr_parse_request(buf, buflen, (const char**)&method_aux, &method_len, (const char**)&path_aux, &path_len, &minor_version, headers_aux, &nheaders_aux, 0);

    if (ret <= 0) {
        print("phr_parse_request failure.");
        return ERR;
    }

    /* set method, path, http version and number of headers */

    req_data->method = (char*)malloc(method_len*sizeof(char));
    if(!req_data->method) {
        print("Failed to allocate memory (%s,%d)", __FILE__, __LINE__);
        return ERR;
    }
    sprintf(req_data->method, "%.*s", (int)method_len, method_aux);

    req_data->path = (char*)malloc(path_len*sizeof(char));
    if(!req_data->path) {
        print("Failed to allocate memory (%s,%d)", __FILE__, __LINE__);
        return ERR;
    }
    sprintf(req_data->path, "%.*s", (int)path_len, path_aux);

    req_data->version = minor_version;
    req_data->num_headers = nheaders_aux;

    /*                                                       */
    /* fills http_headers structure with pairs {name, value} */
    /*                                                       */

    req_data->num_headers = nheaders_aux;

    for (i=0; i<nheaders_aux; i++) {
        sprintf(req_data->headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(req_data->headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;
}

void request_data_print(struct http_req_data *req_data) {
    int i = 0;

    print("HTTP version: %d", req_data->version);
    print("Method: %s", req_data->method);
    print("Path: %s", req_data->path);
    print("\nHeaders:");

    for(i=0; i<req_data->num_headers; i++) {
        print("%s\t%s", req_data->headers[i].name, req_data->headers[i].value);
    }
}

/* wraps phr_parse_request and returns required information in a simple way */
int request_parser(char *buf, size_t buflen, char *method, char *path, int *version, struct http_headers *headers, int *num_headers) {
    char *method_aux, *path_aux;
    size_t method_len, path_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    ret = phr_parse_request(buf, buflen, (const char**)&method_aux, &method_len, (const char**)&path_aux, &path_len, &minor_version, headers_aux, &nheaders_aux, 0);

    if (ret <= 0) {
        return ERR;
    }

    /* sets method, path, http version and number of headers */
    sprintf(method, "%.*s", (int)method_len, method_aux);
    sprintf(path, "%.*s", (int)path_len, path_aux);
    *version = minor_version;
    *num_headers = (int) nheaders_aux;

    /* fills http_headers structure with pairs {name, value} */
    for (i=0; i<nheaders_aux; i++) {
        sprintf(headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;
}

/* wraps phr_parse_response and returns required information in a simple way */
int response_parser(char *buf, size_t buflen, int *version, int *rescode, char *resp, struct http_headers *headers, int *num_headers) {
    char *resp_aux;
    size_t resp_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, status, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    ret = phr_parse_response(buf, buflen, &minor_version, &status, (const char **) &resp_aux, &resp_len, headers_aux, &nheaders_aux, 0);

    if (ret <= 0) {
        return ERR;
    }

    /* sets response message, http version, response code and number of headers */
    sprintf(resp, "%.*s", (int)resp_len, resp_aux);
    *version = minor_version;
    *rescode = status;
    *num_headers = (int) nheaders_aux;

    /* fills http_headers structure with pairs {name, value} */
    for (i=0; i<nheaders_aux; i++) {
        sprintf(headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;

}

/* builds server responses */
int response_builder(char* buffer, int version, int rescode, char *resp, size_t resp_len, int num_headers, struct http_headers *headers, char *body, size_t body_len) {
    int i;
    char buf[MAX_CHAR], buf_aux[MAX_CHAR];

    /* prints first line: http version, response code and response message */
    sprintf(buf, "HTTP/1.%d %d %.*s\r\n", version, rescode, (int) resp_len, resp);

    /* prints headers, pairs {name, value} */
    for (i=0; i<num_headers; i++) {
        sprintf(buf_aux, "%s: %s\r\n", headers[i].name, headers[i].value);
        strcat(buf, buf_aux);
    }

    /* extra /r/n to indicate end of headers and start of body */
    strcat(buf, "\r\n");

    /* prints body */
    if ((int)body_len > 0) {
        sprintf(buf_aux, "%.*s", (int)body_len, body);
        strcat(buf, buf_aux);
    }

    /* generates output */
    strcpy(buffer, buf);

    return OK;
}
