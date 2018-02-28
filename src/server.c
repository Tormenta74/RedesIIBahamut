#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <unistd.h>         // getpid

#include "globals.h"
#include "config.h"
#include "picohttpparser.h"
#include "headers.h"
#include "http.h"

struct server_options so;

int process_request(char *buf, size_t buflen, char *response) {
    int ret, num_headers;
    struct http_pairs res_headers[MAX_HEADERS];
    struct http_req_data rd;
    char real_path[MAX_CHAR];
    char *path_aux, *args_aux, *file_content, *content_type;
    size_t args_len;
    int check_flag;
    long file_len;

    ret = http_request_parse(buf, buflen, &rd);

    http_request_data_print(&rd);

    if(ret == ERR) {
        print("Error when processing request.");
        return ERR;
    }

    // GET
    if(strcmp(rd.method, "GET") == 0) {
        ret = http_request_get_split(rd.path, rd.path_len, &path_aux, &args_aux, &args_len);
        if(ret == ERR) {
            print("Error while splitting path in GET request.");
            return ERR;
        }

        ret = finder_setup();
        if(ret == ERR) {
            print("finder_setup failure.");
            return ERR;
        }
        strcpy(real_path, so.server_root);
        strcat(real_path, path_aux);
        file_len = finder_load(real_path, args_aux, args_len, &file_content, &content_type, &check_flag);

        ret = header_build(so, real_path, content_type, file_len, check_flag, 0, res_headers, &num_headers);
        if(ret == ERR) {
            print("Error while creating headers for GET response.");
            return ERR;
        }

        ret = http_response_build(response, rd.version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
        if(ret == ERR) {
            print("Error while creating GET response.");
            return ERR;
        }

        finder_clean();
        if (args_len != 0) {
            free(path_aux);
            free(args_aux);
        }
    }

    // POST
    if(strcmp(rd.method, "POST") == 0) {
        ret = finder_setup();
        if(ret == ERR) {
            print("finder_setup failure.");
            return ERR;
        }
        strcpy(real_path, so.server_root);
        strcat(real_path, rd.path);
        file_len = finder_load(real_path, rd.body, rd.body_len, &file_content, &content_type, &check_flag);

        ret = header_build(so, real_path, content_type, file_len, check_flag, 0, res_headers, &num_headers);
        if(ret == ERR) {
            print("Error while creating headers for POST response.");
            return ERR;
        }

        ret = http_response_build(response, rd.version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
        if(ret == ERR) {
            print("Error while creating POST response.");
            return ERR;
        }

        finder_clean();
    }

    // OPTIONS
    if(strcmp(rd.method, "OPTIONS") == 0) {
        ret = header_build(so, NULL, NULL, 0, 0, 1, res_headers, &num_headers);
        if(ret == ERR) {
            print("Error while creating headers for OPTIONS response.");
            return ERR;
        }

        ret = http_response_build(response, rd.version, 200, "OK", 2, num_headers, res_headers, NULL, 0);
        if(ret == ERR) {
            print("Error while creating OPTIONS response.");
            return ERR;
        }

        return OK;
    }

    return OK;
}

int main() {
    int ret;
    char buf[MAX_CHAR], buf2[MAX_CHAR], buf3[MAX_CHAR], response[MAX_CHAR];
    struct http_req_data rd;

    // config related

    ret = config_parse("server.conf", &so);
    if(ret == ERR) {
        printf("Parsing failed.\n");
        exit(ERR);
    }
    // http parsing related

    // change your request here
    sprintf(buf, "OPTIONS /somedir/page.html HTTP/1.1\r\n\r\nHolamuchachos");
    sprintf(buf2, "GET /Documents/prueba.txt?key1=value1&key2=value2 HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\nJeje soy un chico #listo\r\n\r\n");
    sprintf(buf3, "POST /Documents/prueba.txt HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\nkey1=value1?key2=value2");

    // change the number directly for now: strlen is not reliable
    printf("TEST NUMBER ONE:\n\n");
    ret = process_request(buf, 52, response);
    if(ret == ERR) {
        printf("Parsing failed.\n");
        return ERR;
    }
    printf("\nResponse:\n");
    printf("%s\n", response);

    printf("TEST NUMBER TWO:\n\n");
    ret = process_request(buf2, strlen(buf2), response);
    if (ret == ERR) {
        printf("Request failed.\n");
        return ERR;
    }
    printf("\nResponse:\n");
    printf("%s\n", response);

    printf("TEST NUMBER THREE:\n\n");
    ret = process_request(buf3, strlen(buf3), response);
    if (ret == ERR) {
        printf("Request failed.\n");
        return ERR;
    }
    printf("\nResponse:\n");
    printf("%s\n", response);

    return OK;
}
