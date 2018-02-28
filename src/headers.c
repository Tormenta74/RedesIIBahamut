#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#include "globals.h"
#include "headers.h"
#include "config.h"
#include "http.h"

char *date_generator(time_t *t) {
    time_t rawtime;
    struct tm *timeinfo;
    size_t nformatted;
    char buffer[128], *date_buf;

    if (t != NULL) {
        rawtime = *t;
    } else {
        time(&rawtime);
    }

    timeinfo = gmtime(&rawtime);

    nformatted = strftime(buffer, 128, "%a, %d %b %Y %T %Z", timeinfo);
    if(nformatted == 0) {
        return NULL;
    }

    date_buf = strndup(buffer, nformatted);

    return date_buf;
}

/****************************************************************
 * HEADER CONSTRUCTIONS
 */

// returns the current date in RFC 1123 format
// (the char* array is allocated; it's the responsability
// of the caller to free it)
char *header_date() {
    return date_generator(NULL);
}

char *header_server(struct server_options so) {
    return so.server_signature;
}

char *header_last_modified(char *path) {
    int ret;
    struct stat sb;

    ret = stat(path, &sb);
    if (ret) {
        return NULL;
    }

    return date_generator(&sb.st_mtime);
}

int header_build(struct server_options so, char *path, char *contenttype, long len, int check_flag, int options_flag, struct http_pairs *headers, int *num_headers) {
    char *date_buf, *server_buf, *lm_buf;

    if (num_headers == NULL || headers == NULL) {
        return ERR;
    }

    date_buf = header_date();
    server_buf = header_server(so);

    sprintf(headers[0].name, "Date");
    sprintf(headers[0].value, date_buf);
    sprintf(headers[1].name, "Server");
    sprintf(headers[1].value, server_buf);

    *num_headers = 2;

    if (options_flag == 1) {
        sprintf(headers[2].name, "Allow");
        sprintf(headers[2].value, "OPTIONS, GET, POST");
        *num_headers = 3;
    }

    if (check_flag == 1) {
        lm_buf = header_last_modified(path);

        if (lm_buf == NULL) {
            return ERR;
        }

        sprintf(headers[2].name, "Content-Type");
        sprintf(headers[2].value, contenttype);
        sprintf(headers[3].name, "Content-Length");
        sprintf(headers[3].value, "%d", len);
        sprintf(headers[4].name, "Last-Modified");
        sprintf(headers[4].value, lm_buf);

        *num_headers = 5;
    }

    return OK;
}
