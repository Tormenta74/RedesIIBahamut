#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#include "globals.h"
#include "headers.h"
#include "config.h"

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
