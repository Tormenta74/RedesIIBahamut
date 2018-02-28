#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strcat, strcpy
#include <time.h>

#include "globals.h"
#include "headers.h"
#include "parser.h"
#include "picohttpparser.h"

/****************************************************************
 * HEADER CONSTRUCTIONS
 */

// returns the current date in RFC 1123 format
// (the char* array is allocated; it's the responsability
// of the caller to free it)
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

/****************************************************************
 * DATA STRUCTURES UTILS
 */

void http_request_data_print(struct http_req_data *rd) {
    int i = 0;

    print("HTTP version: %d", rd->version);
    print("Method: %s", rd->method);
    print("Path: %s", rd->path);
    print("\nHeaders:");

    for(i=0; i<rd->num_headers; i++) {
        print("%s\t%s", rd->headers[i].name, rd->headers[i].value);
    }
}

void http_request_data_free(struct http_req_data *rd) {
    if(rd->method) {
        free(rd->method);
    }
    if(rd->path) {
        free(rd->path);
    }
}

/****************************************************************
 * PARSING REQUESTS AND RESPONSES
 */


/* returns pointer to body */
int http_response_body(char *buf, char **body) {
    char sequence[5], *pointer;

    pointer = buf;
    do {
        sprintf(sequence, "%.*s", 4, pointer);
        pointer++;
    } while(strcmp(sequence, "\r\n\r\n"));

    pointer += 3;

    *body = pointer;

    return OK;
}

/* wraps phr_parse_request and returns required information in a simple way */
int http_request_parse(char *buf, size_t buflen, struct http_req_data *rd) {
    char *method_aux, *path_aux;
    size_t method_len, path_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    ret = phr_parse_request(buf, buflen, (const char**)&method_aux, &method_len, (const char**)&path_aux, &path_len, &minor_version, headers_aux, &nheaders_aux, 0);

    if(ret <= 0) {
        print("phr_parse_request failure.");
        return ERR;
    }

    /* set method, path, http version and number of headers */

    rd->method = (char*)malloc(method_len*sizeof(char));
    if(!rd->method) {
        print("Failed to allocate memory (%s,%d)", __FILE__, __LINE__);
        return ERR;
    }
    sprintf(rd->method, "%.*s", (int)method_len, method_aux);

    rd->path = (char*)malloc(path_len*sizeof(char));
    if(!rd->path) {
        print("Failed to allocate memory (%s,%d)", __FILE__, __LINE__);
        return ERR;
    }
    sprintf(rd->path, "%.*s", (int)path_len, path_aux);

    rd->version = minor_version;
    rd->num_headers = nheaders_aux;

    /*                                                       */
    /* fills http_pairs structure with pairs {name, value} */
    /*                                                       */

    rd->num_headers = nheaders_aux;

    for(i=0; i<nheaders_aux; i++) {
        sprintf(rd->headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(rd->headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;
}




/* builds server responses */
int http_response_build(char* buffer, int version, int rescode, char *resp, size_t resp_len, int num_headers, struct http_pairs *headers, char *body, size_t body_len) {
    int i;
    char buf[MAX_CHAR], buf_aux[MAX_CHAR];

    /* prints first line: http version, response code and response message */
    sprintf(buf, "HTTP/1.%d %d %.*s\r\n", version, rescode, (int) resp_len, resp);

    /* prints headers, pairs {name, value} */
    for(i=0; i<num_headers; i++) {
        sprintf(buf_aux, "%s: %s\r\n", headers[i].name, headers[i].value);
        strcat(buf, buf_aux);
    }

    /* extra /r/n to indicate end of headers and start of body */
    strcat(buf, "\r\n");

    /* prints body */
    if((int)body_len > 0) {
        sprintf(buf_aux, "%.*s", (int)body_len, body);
        strcat(buf, buf_aux);
    }

    /* generates output */
    strcpy(buffer, buf);

    return OK;
}

/****************************************************************/
/* DEPRECATED OR UNUSED */

/* wraps phr_parse_response and returns required information in a simple way */
int response_parser(char *buf, size_t buflen, int *version, int *rescode, char *resp, struct http_pairs *headers, int *num_headers) {
    char *resp_aux;
    size_t resp_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, status, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    ret = phr_parse_response(buf, buflen, &minor_version, &status, (const char **) &resp_aux, &resp_len, headers_aux, &nheaders_aux, 0);

    if(ret <= 0) {
        return ERR;
    }

    /* sets response message, http version, response code and number of headers */
    sprintf(resp, "%.*s", (int)resp_len, resp_aux);
    *version = minor_version;
    *rescode = status;
    *num_headers = (int) nheaders_aux;

    /* fills http_pairs structure with pairs {name, value} */
    for(i=0; i<nheaders_aux; i++) {
        sprintf(headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;

}

/* parses arguments from string "key1=value1&key2=value2&..." */
int argument_parser(char *buf, struct http_args_data *arguments) {
    char *aux;
    int counter=0, index=0;

    if (buf == NULL || arguments == NULL) {
        print("Argument parsing failure.\n");
        return ERR;
    }

    // checks if we have exceeded the maximum number of arguments
    aux = buf;
    while(*aux != '\0') {
        if(*aux == '&') {
            counter++;
        }
        aux++;
    }

    // exits if we have exceeded the maximum number of arguments, sets num_pairs otherwise
    if (counter >= MAX_ARGS) {
        print("Too many arguments. Argument parsing failure.\n");
        return ERR;
    }
    arguments->num_pairs = counter+1;

    aux = buf;
    counter = 0;
    while(1) {
        if((*aux != '=') && (*aux != '&') && (*aux != '\0')) {
            // increases the counter for each character of the word
            counter++;
        } else if(*aux == '=') {
            // when finding '=', the first member of {key, value} has been explored
            sprintf(arguments->args[index].name, "%.*s", counter, buf);
            buf += counter + 1;
            counter = 0;
        } else {
            // when finding '&' or '\0', the second member of {key, value} has been explored
            sprintf(arguments->args[index].value, "%.*s", counter, buf);
            buf += counter + 1;
            counter = 0;
            index++;
        }
        // when finding '\0' we have finished exploring the original buffer
        if (*aux == '\0') {
            break;
        }
        // continues to the next character
        aux++;
    }

    return OK;

}

/* arguments parser for GET and POST */
int request_argument_parser(char *method, char *buffer, struct http_args_data *args) {
    int ret;
    char *aux;

    if (method == NULL || buffer == NULL || args == NULL) {
        print("Request arguments parsing failure.\n");
        return ERR;
    }

    // checks if method is actually "GET" or "POST"
    if ((ret = strcmp(method, "GET")) && strcmp(method, "POST")) {
        print("Request method when parsing arguments failure.\n");
        return ERR;
    }

    // if method is "GET", takes the substring after the '?' in the url (the buffer needs to be the path)
    // if method is "POST", takes the whole buffer (the buffer needs to be the body)
    aux = strdup(buffer);
    if (!ret) {
        while (*aux != '?') {
            aux++;
        }
        aux++;
    }

    ret = argument_parser(aux, args);
    if (ret == ERR) {
        print("Argument parser failure, as you have seen.\n");
        return ERR;
    }

    return OK;

}

/* wraps phr_parse_request and returns required information in a simple way */
int http_request_parse_old(char *buf, size_t buflen, char *method, char *path, int *version, struct http_pairs *headers, int *num_headers) {
    char *method_aux, *path_aux;
    size_t method_len, path_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    ret = phr_parse_request(buf, buflen, (const char**)&method_aux, &method_len, (const char**)&path_aux, &path_len, &minor_version, headers_aux, &nheaders_aux, 0);

    if(ret <= 0) {
        return ERR;
    }

    /* sets method, path, http version and number of headers */
    sprintf(method, "%.*s", (int)method_len, method_aux);
    sprintf(path, "%.*s", (int)path_len, path_aux);
    *version = minor_version;
    *num_headers = (int) nheaders_aux;

    /* fills http_pairs structure with pairs {name, value} */
    for(i=0; i<nheaders_aux; i++) {
        sprintf(headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;
}
