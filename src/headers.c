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
 * Both dates are returned in format "Day, DD Mon YYYY HH:MM:SS GMT" (RFC 1123).
 *
 * In:
 * time_t *t: date to be generated (in seconds)
 *
 * Return:
 * String containing either the current date or the desired date in the required format.
 *
 * */
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

    /* formatting */
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
 * Description: Function that returns the current date in the required format using
 * internal functions.
 *
 * Return:
 * String containing the current date.
 * */
char *header_date() {
    /* date_generator requires its output to be freed by the caller */
    return date_generator(NULL);
}

/*
 * Description: Function that returns the name of the server from the server.conf file.
 *
 * In:
 * struct server_options so: structure that contains server information
 *
 * Return:
 * String containing the name of the server.
 * */
char *header_server(struct server_options so) {
    return so.server_signature;
}

/*
 * Description: Receives the path to an internal file and returns the date of its last
 * modification if it exists.
 *
 * In:
 * char *path: path of the file whose last modification date is required
 *
 * Return:
 * String containing the date of the last modification, NULL if it doesn't exist or there has been an error
 * */
char *header_last_modified(char *path) {
    int ret;
    struct stat sb;

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
 * int check_flag: flag that indicates whether the required resource exists and is available
 * int options_flag: flag that indicates if it is an OPTIONS request
 * struct http_pairs *headers: pointer to http_pairs structure to be built
 * int *num_headers: pointer to integer to be initialized with the number of headers generated
 *
 * Return:
 * ERR if there has been any error during the process, OK otherwise
 * */
int header_build(struct server_options so, char *path, char *contenttype, long len, int res_flag, int check_flag, int options_flag, struct http_pairs *headers, int *num_headers) {
    char *date_buf = NULL, *server_buf = NULL, *lm_buf = NULL;

    if (num_headers == NULL || headers == NULL) {
        return ERR;
    }

    /* generation of the two headers that are always part of the response regardless of its method */
    date_buf = header_date();
    if (!date_buf) {
        print("headers: Failed to write the date buffer.");
        return ERR;
    }
    server_buf = header_server(so);
    if (!server_buf) {
        print("headers: Failed to write the servername buffer.");
        return ERR;
    }

    sprintf(headers[0].name, "Date");
    sprintf(headers[0].value, date_buf);
    sprintf(headers[1].name, "Server");
    sprintf(headers[1].value, server_buf);

    *num_headers = 2;

    /* generation of the "Allow" header in case this is a response to an OPTIONS request */
    if (options_flag == 1) {
        sprintf(headers[2].name, "Allow");
        sprintf(headers[2].value, "OPTIONS, GET, POST");
        *num_headers = 3;
    }

    /* generation of the headers that provide information about the resource, in case it is available and/or needed */
    if (res_flag == 1) {
        sprintf(headers[2].name, "Content-Type");
        sprintf(headers[2].value, contenttype);
        sprintf(headers[3].name, "Content-Length");
        sprintf(headers[3].value, "%ld", len);
        *num_headers = 4;

        if (check_flag == 1) {
            lm_buf = header_last_modified(path);

            if (lm_buf == NULL) {
                return ERR;
            }

            sprintf(headers[4].name, "Last-Modified");
            sprintf(headers[4].value, lm_buf);
            *num_headers = 5;

            if (lm_buf) {
                free(lm_buf);
            }
        }
    }

    if (date_buf) {
        free(date_buf);
    }

    return OK;
}
