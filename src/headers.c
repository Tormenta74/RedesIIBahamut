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

/*
 * Description: Internal function that generates a date depending on the parameter passed
 * as an argument. If the parameter is NULL, a string with the current date is returned;
 * if the parameter is a pointer to a time_t date value, a string with that date is returned.
 * Both dates are returned in format "Day, DD MMM YYYY HH:MM:SS GMT" (RFC 1123). Memory is
 * allocated for the buffer: it is the responsability of the caller to free it.
 *
 * In:
 * time_t *t: date to be generated (in seconds)
 *
 * Return:
 * String containing either the current date or the desired date.
 */
char *date_generator(time_t *t) {
    time_t rawtime;
    struct tm *timeinfo;
    size_t nformatted;
    char buffer[128];

    /* checks the argument to determine which date is needed */
    if (t != NULL) {
        /* date passed as argument */
        rawtime = *t;
    } else {
        /* current date */
        time(&rawtime);
    }

    timeinfo = gmtime(&rawtime);

    /* universal time formatting */
    nformatted = strftime(buffer, 128, "%a, %d %b %Y %T %Z", timeinfo);
    if (nformatted == 0) {
        return NULL;
    }

    /* allocation for the string, which must be freed by the caller */
    return strndup(buffer, nformatted);
}

/****************************************************************
 * HEADER CONSTRUCTIONS
 ****************************************************************/

/*
 * Description: Returns the current date in the required format using
 * internal functions.
 *
 * Return:
 * String containing the current date.
 */
char *header_date() {
    /* date_generator requires its output to be freed by the caller */
    return date_generator(NULL);
}

/*
 * Description: Returns the name of the server parsed from the configuration file.
 *
 * In:
 * struct server_options so: structure that contains server information
 *
 * Return:
 * String containing the name of the server.
 */
char *header_server(struct server_options so) {
    return so.server_signature;
}

/*
 * Description: Receives the path to a file and returns the date of its last modification
 * if it exists.
 *
 * In:
 * char *path: path of the file whose last modification date is required
 *
 * Return:
 * String containing the date of the last modification, NULL if it doesn't exist or there
 * has been an error.
 */
char *header_last_modified(char *path) {
    int ret;
    struct stat sb;

    /* stat fills structure sb with lots of info about the file with path "path",
    including last modification time (st_mtime) */
    ret = stat(path, &sb);
    if (ret) {
        return NULL;
    }

    return date_generator(&sb.st_mtime);
}

/*
 * Description: Generates the headers of a server response depending on the request that is being
 * processed. The information about the request is given by the flags check_flag (which indicates
 * whether the requested resource exists and is available) and options_flag (which indicates if
 * the request is an OPTIONS request). The output headers to be generated are returned to the
 * http_pairs structure and the num_headers integer.
 *
 * In:
 * struct server_options so: structure that cointains server information
 * char *path: path of the required resource
 * char *contenttype: string containing the content type of the required resource
 * long len: length of the required resource
 * int res_flag: flag that indicates whether there is a resource delivered in the server response (requested or not)
 * int check_flag: flag that indicates whether the required resource exists and is available
 * int options_flag: flag that indicates if it is an OPTIONS request
 * struct http_pairs *headers: pointer to http_pairs structure to be built
 * int *num_headers: pointer to integer to be initialized with the number of headers generated
 *
 * Return:
 * ERR if there has been any error during the process, OK otherwise.
 */
int header_build(struct server_options so, char *path, char *contenttype, long len, int res_flag, int check_flag, int options_flag, struct http_pairs *headers, int *num_headers) {
    char *date_buf = NULL, *server_buf = NULL, *lm_buf = NULL;

    if (num_headers == NULL || headers == NULL) {
        return ERR;
    }

    /* generation of the two headers that are always part of the response regardless of its method */
    date_buf = header_date();
    if(!date_buf) {
        print("headers: Failed to write the date buffer.");
        return ERR;
    }
    server_buf = header_server(so);
    if(!server_buf) {
        print("headers: Failed to write the servername buffer.");
        return ERR;
    }

    /* including both headers into the structure */
    sprintf(headers[0].name, "Date");
    sprintf(headers[0].value, date_buf);
    sprintf(headers[1].name, "Server");
    sprintf(headers[1].value, server_buf);

    /* sets num_headers to 2 for now */
    *num_headers = 2;

    /* generation of the "Allow" header in case this is a response to an OPTIONS request */
    if (options_flag == 1) {
        sprintf(headers[2].name, "Allow");
        sprintf(headers[2].value, "OPTIONS, GET, POST");
        /* if options_flag == 1 the rest of the flags should always be 0, therefore we'll have 3 headers */
        *num_headers = 3;
    }

    /* res_flag indicates that there is a resource that is being sent to the client inside the response,
    while check_flag indicates that the client has requested a resource and that resource has been found*/
    if (res_flag == 1) {
        /* if there is a resource involved, we include Content-Type and Content-Length headers */
        sprintf(headers[2].name, "Content-Type");
        sprintf(headers[2].value, contenttype);
        sprintf(headers[3].name, "Content-Length");
        sprintf(headers[3].value, "%ld", len);
        /* sets num_headers to 4 for now (date+server+content-type+content-length) */
        *num_headers = 4;

        /* if the resource has been found, we add another header */
        if (check_flag == 1) {
            /* we obtain the last modification time of the resource requested */
            lm_buf = header_last_modified(path);

            if (lm_buf == NULL) {
                return ERR;
            }

            /* we include this header into the structure */
            sprintf(headers[4].name, "Last-Modified");
            sprintf(headers[4].value, lm_buf);
            /* if this header is included into the structure, it is because there is a resource
            involved, so res_flag will always be activated and there will be 5 headers */
            *num_headers = 5;

            /* we free the memory allocated by header_last_modified */
            if(lm_buf) {
                free(lm_buf);
            }
        }
    }

    /* we free the memory allocated by header_date and header_server */
    if(date_buf) {
        free(date_buf);
    }

    return OK;
}
