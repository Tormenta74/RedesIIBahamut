#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strcat, strcpy

#include "globals.h"
#include "parser.h"
#include "picohttpparser.h"

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
        print("Error when parsing request. %d\n", ret);
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
      print("Error when parsing response. %d\n", ret);
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
   sprintf(buf_aux, "%.*s", (int)body_len, body);
   strcat(buf, buf_aux);

   /* generates output */
   strcpy(buffer, buf);

   return OK;
}
