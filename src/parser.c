#include <stdio.h>
#include <stdlib.h>         // malloc

#include "globals.h"
#include "parser.h"
#include "picohttpparser.h"

/* wraps phr_parse_request and returns required information in a basic way */
int request_parser(char *buf, size_t buflen, char *method, char *path, int *version, struct request_headers *headers, int *num_headers) {
   char *method_aux, *path_aux;
   size_t method_len, path_len, nheaders_aux;
   struct phr_header headers_aux[MAX_HEADERS];
   int ret, minor_version, i;

   /* setting nheaders_aux to MAX_HEADERS */
   nheaders_aux = (sizeof(headers_aux) / sizeof(headers_aux[0]));

   ret = phr_parse_request(buf, buflen, (const char **) &method_aux, &method_len, (const char **) &path_aux, &path_len, &minor_version, headers_aux, &nheaders_aux, 0);

   if (ret <= 0) {
      print("Error when parsing request. %d\n", ret);
      return ERR;
   }

   /* sets method, path, http version and number of headers */
   sprintf(method, "%.*s", (int)method_len, method_aux);
   sprintf(path, "%.*s", (int)path_len, path_aux);
   *version = minor_version;
   *num_headers = (int) nheaders_aux;

   /* fills request_headers structure with pairs {name, value} */
   for (i=0; i<nheaders_aux; i++) {
      sprintf(headers[i].name, "%.*s", (int)headers_aux[i].name_len, headers_aux[i].name);
      sprintf(headers[i].value, "%.*s", (int)headers_aux[i].value_len, headers_aux[i].value);
   }

   return OK;

}
