#include <stdio.h>
#include <stdlib.h>         // malloc

#include "globals.h"
#include "parser.h"
#include "picohttpparser.h"

/* wraps phr_parse_request and returns required information in a basic way */
int request_parser(const char *buf, int buflen, char **method, char **path, int *version, struct request_headers *headers) {
   char *method_aux, *path_aux;
   size_t method_len, path_len, num_headers;
   struct phr_header headers_aux[MAX_HEADERS];
   int ret, minor_version, i;

   /* setting num_headers to MAX_HEADERS */
   num_headers = (sizeof(headers) / sizeof(headers[0]));

   ret = phr_parse_request(buf, buflen, (const char **) &method_aux, &method_len, (const char **) &path_aux, &path_len, &minor_version, headers_aux, &num_headers, 0);

   if (ret <= 0) {
      print("Error when parsing request.");
      return ERR;
   }

   /* sets method, path and http version */
   sprintf(*method, "%.*s", (int)method_len, method_aux);
   sprintf(*path, "%.*s", (int)path_len, path_aux);
   *version = minor_version;

   /* fills request_headers structure with number of headers and pairs name:value */
   headers->num_headers = (int) num_headers;
   for (i=0; i<num_headers; i++) {
      sprintf(headers->name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
      sprintf(headers->value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value_len);
   }

   return OK;

}
