#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <unistd.h>         // getpid

#include "globals.h"
#include "libtcp.h"
#include "libconcurrent.h"
#include "config.h"
#include "config.h"
#include "finder.h"
#include "picohttpparser.h"
#include "headers.h"
#include "http.h"
#include "server.h"

#define GET     1000
#define POST    1001
#define OPTIONS 1002

extern pthread_mutex_t nconn_lock;  // server.c
extern int n_conn;                  // server.c

struct server_options so;

int process_request(char *buf, size_t buflen, char *response) {
    int status, num_headers;
    struct http_pairs res_headers[MAX_HEADERS];
    struct http_req_data rd;
    char real_path[MAX_CHAR];
    char *path_aux, *args_aux, *file_content, *content_type;
    size_t args_len;
    int check_flag;
    long file_len;

    status = http_request_parse(buf, buflen, &rd);

    http_request_data_print(&rd);

    if (status == ERR) {
        print("Error when processing request.");
        return ERR;
    }

    // GET
    if (strcmp(rd.method, "GET") == 0) {
        status = http_request_get_split(rd.path, rd.path_len, &path_aux, &args_aux, &args_len);
        if (status == ERR) {
            print("Error while splitting path in GET request.");
            return ERR;
        }

        status = finder_setup();
        if (status == ERR) {
            print("finder_setup failure.");
            return ERR;
        }
        strcpy(real_path, so.server_root);
        strcat(real_path, path_aux);
        file_len = finder_load(real_path, args_aux, args_len, &file_content, &content_type, &check_flag);

        status = header_build(so, real_path, content_type, file_len, check_flag, 0, res_headers, &num_headers);
        if (status == ERR) {
            print("Error while creating headers for GET response.");
            return ERR;
        }

        status = http_response_build(response, rd.version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
        if (status == ERR) {
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
    if (strcmp(rd.method, "POST") == 0) {
        status = finder_setup();
        if (status == ERR) {
            print("finder_setup failure.");
            return ERR;
        }
        strcpy(real_path, so.server_root);
        strcat(real_path, rd.path);
        file_len = finder_load(real_path, rd.body, rd.body_len, &file_content, &content_type, &check_flag);

        status = header_build(so, real_path, content_type, file_len, check_flag, 0, res_headers, &num_headers);
        if (status == ERR) {
            print("Error while creating headers for POST response.");
            return ERR;
        }

        status = http_response_build(response, rd.version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
        if (status == ERR) {
            print("Error while creating POST response.");
            return ERR;
        }

        finder_clean();
    }

    // OPTIONS
    if (strcmp(rd.method, "OPTIONS") == 0) {
        status = header_build(so, NULL, NULL, 0, 0, 1, res_headers, &num_headers);
        if (status == ERR) {
            print("Error while creating headers for OPTIONS response.");
            return ERR;
        }

        status = http_response_build(response, rd.version, 200, "OK", 2, num_headers, res_headers, NULL, 0);
        if (status == ERR) {
            print("Error while creating OPTIONS response.");
            return ERR;
        }

        return OK;
    }

    return OK;
}

void error_response(int errcode, char *err, size_t errlen, char *err_extended, int sockfd) {
    int num_headers;
    struct http_pairs res_headers[MAX_HEADERS];
    char response[MAX_CHAR], body[MAX_CHAR];

    header_build(so, NULL, NULL, 0, 0, 0, res_headers, &num_headers);

    sprintf(body, "<HTML><HEAD><title>%d Error Page</title></HEAD><BODY><p align=\"center\"><h1>Error %d</h1><br>%s<p></BODY></HTML>", errcode, errcode, err_extended);

    http_response_build(response, 1, errcode, err, errlen, num_headers, res_headers, body, strlen(body));

    if (tcp_send(sockfd, (const void*)response, strlen(response)) < 0) {
        print("could not send response (%s:%d).", __FILE__, __LINE__);
        print("errno (send): %s.", strerror(errno));

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }
}

/*
 * Description: Sends the 400 error code response, including an HTML page.
 *
 * In:
 * int sockfd: file descriptor of bad requester socket
 */
void ill_formed_request(int sockfd) {
    error_response(400, "Bad Request", 11, "The server could not understand the request due to malformed syntax.", sockfd);
}

/*
 * Description: Sends the 501 error code response, including an HTML page.
 *
 * In:
 * int sockfd: file descriptor of way too entitled socket
 */
void unsupported_verb(int sockfd) {
    error_response(501, "Not Implemented", 15, "This method is not implemented by the server.", sockfd);
}

/*
 * Description: Sends the 404 error code response, including an HTML page.
 *
 * In: int sockfd: file descriptor of way too entitled socket
 */
void resource_not_found(int sockfd) {
    error_response(404, "Not Found", 9, "The server has not found any resource matching the request URI.", sockfd);
}

/*
 * Description: Sends the 415 error code response, including an HTML page.
 *
 * In: int sockfd: file descriptor of way too entitled socket
 */
void unsupported_media_type(int sockfd) {
    error_response(415, "Unsupported Media Type", 22, "The requested resource has a media type not supported by the server.", sockfd);
}

/*
 * Description: GET request processing.
 *
 * In:
 * int sockfd: file descriptor of way too entitled socket
 */
int get(int sockfd, struct http_req_data *rd) {
    int status, num_headers, check_flag, response_len;
    long file_len;
    struct http_pairs res_headers[MAX_HEADERS];
    char real_path[MAX_CHAR], *response = NULL;
    char *path_aux, *args_aux, *file_content, *content_type;
    size_t args_len;

    status = http_request_get_split(rd->path, rd->path_len, &path_aux, &args_aux, &args_len);
    if (status == ERR) {
        print("Error while splitting path in GET request.");
        return ERR;
    }

    strcpy(real_path, so.server_root);
    strcat(real_path, path_aux);
    file_len = finder_load(real_path, args_aux, args_len, &file_content, &content_type, &check_flag);

    if (file_len == NOT_FOUND) {
        return NOT_FOUND;
    } else if (file_len == NO_MATCH) {
        return NO_MATCH;
    }

    status = header_build(so, real_path, content_type, file_len, check_flag, 0, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for GET response.");
        if (args_len != 0) {
            free(path_aux);
            free(args_aux);
        }
        return ERR;
    }

    status = http_response_build(response, rd->version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
    if (status == ERR) {
        print("Error while creating GET response.");
        if (args_len != 0) {
            free(path_aux);
            free(args_aux);
        }
        return ERR;
    }

    // TO BE MODIFIED
    response_len = strlen(response);
    status = 0;

    do {
        status = tcp_send(sockfd, (const void *)(response + status), response_len - status);
        if (status < 0) {
            print("could not send response (%s:%d).", __FILE__, __LINE__);
            print("errno (send): %s.", strerror(errno));

            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);

            conc_exit();

            if (args_len != 0) {
                free(path_aux);
                free(args_aux);
            }

            return ERR;
        }
        response += status;
        response_len -= status;
    } while (response_len > 0);

    if (args_len != 0) {
        free(path_aux);
        free(args_aux);
    }

    return OK;
}

/*
 * Description: POST request processing.
 *
 * In:
 * int sockfd: file descriptor of way too entitled socket
 */
int post(int sockfd, struct http_req_data *rd) {
    char real_path[MAX_CHAR], *response = NULL;
    char *file_content, *content_type;
    long file_len;
    int check_flag, status, response_len, num_headers;
    struct http_pairs res_headers[MAX_HEADERS];

    strcpy(real_path, so.server_root);
    strcat(real_path, rd->path);
    file_len = finder_load(real_path, rd->body, rd->body_len, &file_content, &content_type, &check_flag);

    if (file_len == NOT_FOUND) {
        return NOT_FOUND;
    } else if (file_len == NO_MATCH) {
        return NO_MATCH;
    }

    status = header_build(so, real_path, content_type, file_len, check_flag, 0, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for POST response.");
        return ERR;
    }

    status = http_response_build(response, rd->version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
    if (status == ERR) {
        print("Error while creating POST response.");
        return ERR;
    }

    // TO BE MODIFIED
    response_len = strlen(response);
    status = 0;

    do {
        status = tcp_send(sockfd, (const void *)(response + status), response_len - status);
        if (status < 0) {
            print("could not send response (%s:%d).", __FILE__, __LINE__);
            print("errno (send): %s.", strerror(errno));

            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);

            conc_exit();

            return ERR;
        }
        response += status;
        response_len -= status;
    } while (response_len > 0);

    return OK;
}

/*
 * Description: Sends the very smol response informing of the services
 *              offered by our humble server.
 *
 * In:
 * int sockfd: file descriptor of very polite socket
 * int version: HTTP version
 */
int options(int sockfd, int version) {
    int status, num_headers;
    struct http_pairs res_headers[MAX_HEADERS];
    char response[MAX_CHAR];

    // construct headers
    status = header_build(so, NULL, NULL, 0, 0, 1, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for OPTIONS response.");
        return ERR;
    }

    // construct response
    status = http_response_build(response, version, 200, "OK", 2, num_headers, res_headers, NULL, 0);
    if (status == ERR) {
        print("Error while creating OPTIONS response.");
        return ERR;
    }

    return OK;
}

void *serve_http(void *args) {
    // get the information right away
    int sockfd = *(int*)args;
    int len = -1, client_alive = 1, status;
    char receive_buffer[MAX_RECV_LEN],
         *aux_receive = NULL;
    struct http_req_data rd;

    print("serve_http: handdling socket %d.", sockfd);

    // done with the memory
    if (args) {
        free(args);
    }

    // in case of error, decrease the number of active connections
    if (sockfd <= 0) {
        print("Wrong parameters for socket. (%s:%d).", __FILE__, __LINE__);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit(NULL);
    }

    // TODO: establish connection's end
    while (client_alive) {
        // clean the buffer
        bzero(receive_buffer, MAX_RECV_LEN);
        // clean the auxiliary buffer as well
        if (aux_receive) {
            free(aux_receive);
        }

        /***************/
        ////         ////
        //// RECEIVE ////
        ////         ////
        /***************/

        // receiving 0 is an indicator of the client closing the connection
        if ((len = tcp_receive(sockfd, receive_buffer, MAX_RECV_LEN)) == 0) {
            print("Client closing connection.");
            tcp_close_socket(sockfd);

            // decrease nconn and exit
            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);
            conc_exit();
        }

        // check for errors on recv
        if (len < 0) {
            print("Could not receive any data (%s:%d).", __FILE__, __LINE__);
            print("errno (receive): %s.", strerror(errno));

            // decrease nconn and exit
            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);
            conc_exit();
        }

        // just in case there is more to the packet:
        if (len == MAX_RECV_LEN) {
            int rebufo = (MAX_RECV_LEN/2) * sizeof(char);
            aux_receive = (char*)malloc(rebufo);
            rebufo = tcp_receive_nb(sockfd, aux_receive, rebufo);
            if (rebufo > 0) {
                // jesus, what are you sending through?
                // take aux, but no more (total max: MAX_RECV_LEN * 1.5)
                // what happens on the next receive, well, too bad

                // let's indicate that we overflowed
                len += rebufo;
            } else {
                // there was no more, which means the original receive
                // was exactly MAX_RECV_LEN, which is odd, but ok
                free(aux_receive);
            }
        }

        /***************/
        ////         ////
        //// PROCESS ////
        ////         ////
        /***************/

        // process received buffer
        if (len > MAX_RECV_LEN) {
            // exceeded original buffer, so realloc aux to hold the
            // entire message
            aux_receive = (char*)realloc(aux_receive, len);

            // move the extra data to the end of the buffer
            memcpy(aux_receive + MAX_RECV_LEN, aux_receive, (len - MAX_RECV_LEN));

            // copy the original buffer over
            memcpy(aux_receive, receive_buffer, MAX_RECV_LEN);

            status = http_request_parse(aux_receive, len, &rd);
        } else {
            status = http_request_parse(receive_buffer, len, &rd);
        }

        if (status == ERR) {
            print("Error processing request.");
            // send standard response
            ill_formed_request(sockfd);

            tcp_close_socket(sockfd);

            // decrease nconn and exit
            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);
            conc_exit();
        }

        // mark connection as short lived
        if (rd.version == 0) {
            client_alive = 0;
        }

        // C doesn't handle string cases, sadly, so let's work around it
        int verb;
        if (strcmp(rd.method, "GET") == 0) {
            verb = GET;
        } else if (strcmp(rd.method, "POST") == 0) {
            verb = POST;
        } else if (strcmp(rd.method, "OPTIONS") == 0) {
            verb = OPTIONS;
        } else {
            print("Client sent a %s request.", rd.method);
            // send standard response
            unsupported_verb(sockfd);

            tcp_close_socket(sockfd);

            // decrease nconn and exit
            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);
            conc_exit();
        }

        // answer each request in their own

        switch (verb) {
        case GET:
            status = get(sockfd, &rd);
            if (status == NOT_FOUND) {
                resource_not_found(sockfd);
            } else if (status == NO_MATCH) {
                unsupported_media_type(sockfd);
            } else if (status == ERR) {
                print("GET processing failed.");
                goto end_serve_http;
            }
            break;
        case POST:
            status = post(sockfd, &rd);
            if (status == NOT_FOUND) {
                resource_not_found(sockfd);
            } else if (status == NO_MATCH) {
                unsupported_media_type(sockfd);
            } else if (status == ERR) {
                print("POST processing failed.");
                goto end_serve_http;
            }
            break;
        case OPTIONS:
            status = options(sockfd, rd.version);
            if (status == ERR) {
                print("OPTIONS processing failed.");
                goto end_serve_http;
            }
            break;
        default:
            // we should never get here, but better safe than sorry
            client_alive = 0;
        }

        // we have processed: begin again (if the client is still there)

    } /* while client_alive */


end_serve_http:
    // finished: decrease the number of active connections
    mutex_lock(&nconn_lock);
    n_conn--;
    mutex_unlock(&nconn_lock);

    tcp_close_socket(sockfd);

    return NULL;
}

int main() {
    int status;

    // config related

    status = config_parse("server.conf", &so);
    if (status == ERR) {
        printf("Configuration file parsing failed.\n");
        exit(ERR);
    }

    // file finder

    status = finder_setup();
    if (status == ERR) {
        printf("Regex setup failed.\n");
        exit(ERR);
    }

    // always clean this memory
    atexit(finder_clean);

    // server setup

    status = server_setup(&so);
    if (status == ERR) {
        printf("Server intialization failed.\n");
        exit(ERR);
    }

    // and finally run

    status = server_accept_loop(serve_http);

    if (status == ERR) {
        print("Abrupt interruption of server.");
        return ERR;
    }

    return OK;
}

// small main to uncomment when I want to test something
//int main() {
//    int status;
//
//    // config related
//
//    status = config_parse("server.conf", &so);
//    if(status == ERR) {
//        printf("Configuration file parsing failed.\n");
//        exit(ERR);
//    }
//
//    config_print(&so);
//
//    return OK;
//}
