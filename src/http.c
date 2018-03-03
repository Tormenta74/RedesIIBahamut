#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strcat, strcpy
#include <time.h>

#include "globals.h"
#include "picohttpparser.h"
#include "http.h"


/****************************************************************
 * DATA STRUCTURES UTILS
 ****************************************************************/

/*
 * Description: Prints the information of a request by a client
 *
 * In:
 * struct http_req_data *rd: structure which contains information on the request
 * */
void http_request_data_print(struct http_req_data *rd) {
    int i = 0;

    print("HTTP version: %d", rd->version);
    print("Method: %s", rd->method);
    print("Path: %s", rd->path);
    print("\nHeaders:");

    for(i=0; i<rd->num_headers; i++) {
        print("%s:\t%s", rd->headers[i].name, rd->headers[i].value);
    }

    /* prints the body only if it exists (it doesn't exist if rd->body is NULL) */
    if(rd->body) {
        print("Body: %s", rd->body);
    }
}

/*
 * Description: Frees the resources allocated to create a http_req_data structure which
 * contains the information on a request by a client
 *
 * In:
 * struct http_req_data *rd: structure to be freed which contains information on the request
 * */
void http_request_data_free(struct http_req_data *rd) {
    if(rd->method) {
        free(rd->method);
    }
    if(rd->path) {
        free(rd->path);
    }
    if(rd->body) {
        free(rd->body);
    }
}

/****************************************************************
 * PARSING REQUESTS AND RESPONSES
 ****************************************************************/


 /*
  * Description: aims to get a pointer to a string containing the body of a request.
  * In some cases there may not be a body, if that happens we indicate the lack of body
  * by the use of a NULL pointer; but, on the other hand, if the body exists, we provide
  * the pointer to the required string.
  *
  * In:
  * char *buf: buffer containing the request in which to look for the body
  * char **body: pointer to string, used to return the pointer to the body
  *
  * Return:
  * ERR if there has been an error during the process, OK otherwise
  * */
int http_response_body(char *buf, char **body) {
    char sequence[5], *pointer;
    int i = 0;

    if(buf == NULL || body == NULL) {
        return ERR;
    }

    /* increases the pointer starting at buf[0] until it finds the sequence "\r\n\r\n" or the end of string '\0' */
    pointer = buf;
    do {
        sprintf(sequence, "%.*s", 4, pointer);
        pointer++;
        i++;
    } while(strcmp(sequence, "\r\n\r\n")!=0 && *pointer!='\0');

    /* if the loop has been exited due to finding the character '\0', the request was not built correctly and ERR is returned */
    if (*pointer == '\0') {
        return ERR;
    }

    /* if the request was built correctly and "\r\n\r\n" has been found, we place the pointer at the beginning of the body */
    pointer += 3;

    /* if the beginning of the body is actually the '\0' character, it means that there is no body and NULL is placed on *body;
    otherwise, the pointer to the beginning of the body is placed on *body */
    if(*pointer != '\0') {
        *body = pointer;
    } else {
        *body = NULL;
    }

    return OK;
}

/*
 * Description: splits the path given by http_request_parse into two strings: the actual path
 * and the arguments. Splitting is necessary only to respond to GET requests, and this case is
 * identified by the presence of a '?' symbol (the symbol that separates both strings). If it
 * is a GET request, the function allocates memory for both strings, and the caller has the
 * responsibility to free them; if it is not a GET request, there is no allocation.
 *
 * In:
 * char *buf: buffer that contains the path given by http_request_parse
 * size_t buflen: length of buf
 * char **path: pointer to string, to save the actual path
 * char **args: pointer to string, to save the arguments
 * size_t *args_len: pointer to size_t, to save the length of args
 *
 * Return:
 * ERR if there has been an error during the process, OK otherwise
 * */
int http_request_get_split(char *buf, size_t buflen, char **path, char **args, size_t *args_len) {
    char *aux;
    size_t path_len_aux, args_len_aux;

    if (buf == NULL) {
        return ERR;
    }

    /* increases the pointer starting at buf[0] until it reaches either a '?' or a '\0' */
    aux = buf;
    while(*aux != '?' && *aux != '\0') {
        aux++;
    }
    /* if it stopped at '\0', there is no '?', so there are no arguments and the given path is the actual path */
    if (*aux == '\0') {
        /* we define the lack of arguments as a NULL pointer and size=0 */
        *args = NULL;
        *args_len = 0;
        /* no allocation */
        *path = buf;
        return OK;
    }

    /* sets length for both strings */
    path_len_aux = aux - buf;
    aux++;
    args_len_aux = buflen - (aux - buf);

    /* memory allocation */
    *path = (char*)malloc(sizeof(char)*path_len_aux);
    *args = (char*)malloc(sizeof(char)*args_len_aux);
    if(!*path || !*args) {
        return ERR;
    }

    /* sets actual path and arguments */
    sprintf(*path, "%.*s", (int)path_len_aux, buf);
    sprintf(*args, "%.*s", (int)args_len_aux, aux);
    *args_len = args_len_aux;

    return OK;
}

/*
 * Description: parses a request, filling a structure with all the necessary information
 * for the server to process and create a response.
 *
 * In:
 * char *buf: buffer that contains the request
 * size_t buflen: length of buf
 * struct http_req_data *rd: structure in which to save all the information
 *
 * Return:
 * ERR if there has been an error during the process, OK otherwise
 * */
int http_request_parse(char *buf, size_t buflen, struct http_req_data *rd) {
    char *method_aux, *path_aux, *body_aux;
    size_t method_len, path_len, body_len, nheaders_aux;
    struct phr_header headers_aux[MAX_HEADERS];
    int ret, minor_version, i;

    /* setting nheaders_aux to MAX_HEADERS */
    nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

    /* usage of the picohttpparser library function */
    ret = phr_parse_request(buf, buflen, (const char**)&method_aux, &method_len, (const char**)&path_aux, &path_len, &minor_version, headers_aux, &nheaders_aux, 0);

    if(ret <= 0) {
        print("phr_parse_request failure.");
        return ERR;
    }

    /* sets method, path, http version and number of headers */
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
    rd->path_len = path_len;
    sprintf(rd->path, "%.*s", (int)path_len, path_aux);

    rd->version = minor_version;
    rd->num_headers = nheaders_aux;

    /* sets body, only if it exists; if it doesn't exist, rd->body is set to NULL and rd->body_len to 0 */
    ret = http_response_body(buf, &body_aux);
    if(ret == ERR) {
        print("http_response_body failure.\n");
        return ERR;
    }
    if(body_aux == NULL) {
        rd->body = NULL;
        rd->body_len = 0;
    } else {
        /* length of body calculation as (length of request) - (pointer at body - pointer at start of request) */
        body_len = buflen - (body_aux - buf);
        rd->body = (char*)malloc(body_len*sizeof(char));
        rd->body_len = body_len;
        if(!rd->body) {
            print("Failed to allocate memory (%s,%d)", __FILE__, __LINE__);
            return ERR;
        }
        sprintf(rd->body, "%.*s", (int)body_len, body_aux);
    }

    /* fills http_pairs structure with pairs {name, value} and sets number of headers */
    rd->num_headers = nheaders_aux;
    for(i=0; i<nheaders_aux; i++) {
        sprintf(rd->headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
        sprintf(rd->headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
    }

    return OK;
}

/*
 * Description: Builds a server response (always after processing a request)
 *
 * In:
 * char* buffer: string in which to save output response
 * int version: version of HTTP used by the server
 * int rescode: response code used by the server
 * char *resp: string which contains the message associated to rescode
 * size_t resp_len: length of resp
 * int num_headers: number of headers of the response
 * struct http_pairs *headers: structure cointaining all the information about headers
 * char *body: body of the response
 * size_t body_len: length of body
 *
 * Return:
 * ERR if there has been an error during the process, OK otherwise
 * */
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

/****************************************************************
 * DEPRECATED OR UNUSED
 ***************************************************************/

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

    if(buf == NULL || arguments == NULL) {
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
    if(counter >= MAX_ARGS) {
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
        if(*aux == '\0') {
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

    if(method == NULL || buffer == NULL || args == NULL) {
        print("Request arguments parsing failure.\n");
        return ERR;
    }

    // checks if method is actually "GET" or "POST"
    if((ret = strcmp(method, "GET")) && strcmp(method, "POST")) {
        print("Request method when parsing arguments failure.\n");
        return ERR;
    }

    // if method is "GET", takes the substring after the '?' in the url (the buffer needs to be the path)
    // if method is "POST", takes the whole buffer (the buffer needs to be the body)
    aux = strdup(buffer);
    if(!ret) {
        while(*aux != '?') {
            aux++;
        }
        aux++;
    }

    ret = argument_parser(aux, args);
    if(ret == ERR) {
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
