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
#include "cgi.h"
#include "finder.h"
#include "picohttpparser.h"
#include "headers.h"
#include "http.h"
#include "server.h"

#define GET     1000
#define HEAD    1001
#define POST    1002
#define OPTIONS 1003


extern int active;                  // libserver.c
extern pthread_mutex_t nconn_lock;  // libserver.c
extern int n_conn;                  // libserver.c

struct server_options so;
int tout_seconds;

/*****************************************************************
 * AUXILIARY FUNCTIONS
 *****************************************************************/

/*
 * Description: Generalizes the process of creating server error responses, based on several arguments.
 *
 * In:
 * int version: HTTP version to create the response with
 * int errcode: error code associated with the server response desired
 * char *err: short identifying message to define the error ("Method Not Implemented", "Resource Not Found", etc)
 * size_t errlen: length of err
 * char *err_extended: long description of the error to show in the html page
 * int sockfd: socket identifier to send the message to the client
 */
void error_response(int version, int errcode, char *err, size_t errlen, char *err_extended, int sockfd) {
    int num_headers, status;
    struct http_pairs res_headers[MAX_HEADERS];
    char body[MAX_CHAR];
    void *response;
    size_t response_len;

    /* builds the body; every error code (with its long description) generates a default html page to show to the client */
    sprintf(body, "<HTML><HEAD><title>%d Error Page</title></HEAD><BODY><p align=\"center\"><h1>Error %d</h1><br>%s<p></BODY></HTML>", errcode, errcode, err_extended);

    /* builds the header, in this case res_flag=1, check_flag=0, options_flag=0 so that the response contains date, server name, length of the html page
       and content of the html page */
    header_build(so, NULL, "text/html", (long)strlen(body), 1, 0, 0, res_headers, &num_headers);

    /* builds the response including the html page as the body */
    status = http_response_build(&response, &response_len, version, errcode, err, errlen, num_headers, res_headers, (void *)body, strlen(body));

    if (status == ERR) {
        print("Error building response (error_response).");
        if (response) {
            free(response);
        }
    }

    /* in this case, we don't need a loop to send the message as it will always be small enough */
    if (tcp_send(sockfd, (const void*)response, (int)response_len) < 0) {
        print("could not send response (%s:%d).", __FILE__, __LINE__);
        print("errno (send): %s.", strerror(errno));

        if (response) {
            free(response);
        }

        /* decrease the number of active connections and exit */
        tcp_close_socket(sockfd);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    if (response) {
        free(response);
    }
}

/*****************************************************************
 * ERROR CODES IMPLEMENTATIONS
 * This functions send a prebuilt response when an error occurs,
 * closing the connection to the client afterwards while they figure
 * out what went wrong.
 * (See the relevant HTTP status codes)
 *****************************************************************/

/*
 * Description: Sends the 400 error code response, including an HTML page.
 *
 * In:
 * int sockfd: file descriptor of bad requester socket
 * int version: protocol minor version
 */
void ill_formed_request(int sockfd) {
    // we will assume the requester accepts HTTP/1.1
    error_response(1, 400, "Bad Request", 12, "The server could not understand the request due to malformed syntax.", sockfd);
}

/*
 * Description: Sends the 404 error code response, including an HTML page.
 *
 * In:
 * int sockfd: file descriptor of way too entitled socket
 * int version: protocol minor version
 */
void resource_not_found(int sockfd, int version) {
    error_response(version, 404, "Not Found", 10, "The server has not found any resource matching the request URI.", sockfd);
}

/*
 * Description: Sends the 415 error code response, including an HTML page.
 *
 * In: int sockfd: file descriptor of extra picky socket
 * int version: protocol minor version
 */
void unsupported_media_type(int sockfd, int version) {
    error_response(version, 415, "Unsupported Media Type", 23, "The requested resource has a media type not supported by the server.", sockfd);
}

/*
 * Description: Sends the 501 error code response, including an HTML page.
 *
 * In:
 * int sockfd: file descriptor of very intransigent socket
 * int version: protocol minor version
 */
void internal_server_error(int sockfd, int version) {
    error_response(version, 500, "Internal Server Error", 23, "There was an error processing the request.", sockfd);
}

/*
 * Description: Sends the 501 error code response, including an HTML page.
 *
 * In:
 * int sockfd: file descriptor of weird speech socket
 * int version: protocol minor version
 */
void unsupported_verb(int sockfd, int version) {
    error_response(version, 501, "Not Implemented", 16, "This method is not implemented by the server.", sockfd);
}

/*****************************************************************
 * MAIN VERBS PROCESSING
 *****************************************************************/

/*
 * Description: GET request processing. Uses the finder module to locate the resource
 * and serves it to the client, getting possible parameters from the URI.
 *
 * In:
 * int sockfd: file descriptor of requesting client
 * struct http_req_data *rd: pointer to the structure loaded with the relevant information
 *
 * Return: ERR if there is an error at any point. OK if all went well.
 *         NOT_FOUND, NO_MATCH or TIMEOUT if resources were unavailable in any way
 * Note: The function has the privilege to exit if there is a failure
 * in critical sections.
 */
int get(int sockfd, struct http_req_data *rd) {
    int status, num_headers, check_flag;
    long file_len;
    struct http_pairs res_headers[MAX_HEADERS];
    char real_path[MAX_CHAR];
    char *path_aux, *args_aux, *content_type;
    void *response, *response_aux, *file_content;
    size_t args_len, response_len;

    // sanity check
    if (!rd) {
        print("get: Unallocated request data.");
        return ERR;
    }

    // fetch possible arguments from the request path
    status = http_request_get_split(rd->path, rd->path_len, &path_aux, &args_aux, &args_len);
    if (status == ERR) {
        print("Error while splitting path in GET request.");
        return ERR;
    }

    // concat server path with resource path
    strcpy(real_path, so.server_root);
    strcat(real_path, path_aux);

    // attempt to read / exec
    file_len = finder_load(real_path, args_aux, args_len, &file_content, &content_type, &check_flag);

    if (file_len == ERR) {
        // very bad things happened somewhere
        print("get: Unexpected error loading resource.");

        // decrease the number of active connections and exit
        tcp_close_socket(sockfd);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    // no such luck
    if (file_len == NOT_FOUND) {
        if (args_len != 0) {
            // arguments were found and loaded from the URI
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);

        // 404, no such resource
        return NOT_FOUND;

        // found the file, but we cannot determine Content-Type
    } else if (file_len == NO_MATCH) {
        if (args_len != 0) {
            // arguments were found and loaded from the URI
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);

        // 415, we don't know what type it is
        return NO_MATCH;

        // script was executed but timed out
    } else if (file_len == TIMEOUT) {

        // 500, script takes too long to respond
        return TIMEOUT;
    }

    // see headers.c
    status = header_build(so, real_path, content_type, file_len, check_flag, check_flag, 0, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for GET response.");
        if (args_len != 0) {
            // remember to free extra reserved memory
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);
        return ERR;
    }

    // see http.c
    status = http_response_build(&response, &response_len, rd->version, 200, "OK", 2, num_headers, res_headers, file_content, (size_t)file_len);
    if (status == ERR) {
        print("Error while creating GET response.");
        if (args_len != 0) {
            // remember to free extra reserved memory
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);
        return ERR;
    }

    // use a pointer to run through the response
    response_aux = response;
    status = 0;

    do {
        status = tcp_send(sockfd, (const void *)(response_aux + status), (int)response_len - status);
        if (status < 0) {
            print("could not send response (%s:%d).", __FILE__, __LINE__);
            print("errno (send): %s.", strerror(errno));

            if (args_len != 0) {
                // remember to free extra reserved memory
                free(path_aux);
                free(args_aux);
            }
            free(file_content);
            free(content_type);
            free(response);

            // decrease the number of active connections and exit
            tcp_close_socket(sockfd);

            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);

            conc_exit();
        }

        // status indicates the number of bytes sent, so if it is less than the actual
        // length of the response, we know we have to send the rest of the response, so
        // we prepare the pointer so that it starts sending from the new position and decrease
        // the amount of bytes that we indicate to be sending
        response_aux += status;
        response_len -= status;

        // and we are not done until all bytes have been sent
    } while (response_len > 0);

    if (args_len != 0) {
        // remember to free extra reserved memory
        free(path_aux);
        free(args_aux);
    }
    free(response);
    free(file_content);
    free(content_type);

    return OK;
}

/*
 * Description: HEAD request processing. Identical to the GET processing, with the
 * difference that the response is built without a body.
 *
 * In:
 * int sockfd: file descriptor of way too entitled socket
 * struct http_req_data *rd: pointer to the structure loaded with the relevant information
 *
 * Return: ERR if there is an error at any point. OK if all went well.
 *         NOT_FOUND, NO_MATCH or TIMEOUT if resources were unavailable in any way
 * Note: The function has the privilege to exit if there is a failure
 * in critical sections.
 */
int head(int sockfd, struct http_req_data *rd) {
    int status, num_headers, check_flag;
    long file_len;
    struct http_pairs res_headers[MAX_HEADERS];
    char real_path[MAX_CHAR];
    char *path_aux, *args_aux, *content_type;
    void *response, *response_aux, *file_content;
    size_t args_len, response_len;

    // sanity check
    if (!rd) {
        print("get: Unallocated request data.");
        return ERR;
    }

    // fetch possible arguments from the request path
    status = http_request_get_split(rd->path, rd->path_len, &path_aux, &args_aux, &args_len);
    if (status == ERR) {
        print("Error while splitting path in GET request.");
        return ERR;
    }

    // concat server path with resource path
    strcpy(real_path, so.server_root);
    strcat(real_path, path_aux);

    // attempt to read / exec
    file_len = finder_load(real_path, args_aux, args_len, &file_content, &content_type, &check_flag);

    if (file_len == ERR) {
        // very bad things happened somewhere
        print("head: Unexpected error loading resource.");

        // decrease the number of active connections and exit
        tcp_close_socket(sockfd);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    // no such luck
    if (file_len == NOT_FOUND) {
        if (args_len != 0) {
            // arguments were found and loaded from the URI
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);

        // 404, no such resource
        return NOT_FOUND;

        // found the file, but we cannot determine Content-Type
    } else if (file_len == NO_MATCH) {
        if (args_len != 0) {
            // arguments were found and loaded from the URI
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);

        // 415, we don't know what type it is
        return NO_MATCH;

        // script was executed but timed out
    } else if (file_len == TIMEOUT) {

        // 500, script takes too long to respond
        return TIMEOUT;
    }

    // see headers.c
    status = header_build(so, real_path, content_type, file_len, check_flag, check_flag, 0, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for GET response.");
        if (args_len != 0) {
            // remember to free extra reserved memory
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);
        return ERR;
    }

    // reuse the response_build functionality: instead, we pass an empty body (see function for a better explanation)
    //                                                                                                          vvvv  v
    status = http_response_build(&response, &response_len, rd->version, 200, "OK", 2, num_headers, res_headers, NULL, 0);
    if (status == ERR) {
        print("Error while creating GET response.");
        if (args_len != 0) {
            // remember to free extra reserved memory
            free(path_aux);
            free(args_aux);
        }
        free(file_content);
        free(content_type);
        return ERR;
    }

    /////////////////////////////////////////////////
    // Refer to the get function for documentation //
    // on this algorithm                           //
    /////////////////////////////////////////////////

    response_aux = response;
    status = 0;

    do {
        status = tcp_send(sockfd, (const void *)(response_aux + status), (int)response_len - status);
        if (status < 0) {
            print("could not send response (%s:%d).", __FILE__, __LINE__);
            print("errno (send): %s.", strerror(errno));

            if (args_len != 0) {
                free(path_aux);
                free(args_aux);
            }
            free(file_content);
            free(content_type);
            free(response);

            // decrease the number of active connections and exit
            tcp_close_socket(sockfd);

            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);

            conc_exit();
        }
        response_aux += status;
        response_len -= status;
    } while (response_len > 0);

    /////////////////////////
    // Algorithm ends here //
    /////////////////////////

    if (args_len != 0) {
        // remember to free extra reserved memory
        free(path_aux);
        free(args_aux);
    }

    free(response);
    free(file_content);
    free(content_type);

    return OK;
}

/*
 * Description: POST request processing. Very similar to the GET processing,
 * but instead of looking for parameters in the path, we scan the body.
 *
 * In:
 * int sockfd: file descriptor of way too entitled socket
 * struct http_req_data *rd: pointer to the structure loaded with the relevant information
 *
 * Return: ERR if there is an error at any point. OK if all went well.
 *         NOT_FOUND, NO_MATCH or TIMEOUT if resources were unavailable in any way
 * Note: The function has the privilege to exit if there is a failure
 * in critical sections.
 */
int post(int sockfd, struct http_req_data *rd) {
    char real_path[MAX_CHAR];
    char *content_type;
    void *response, *response_aux, *file_content;
    size_t response_len;
    long file_len;
    int check_flag, status, num_headers;
    struct http_pairs res_headers[MAX_HEADERS];

    // sanity check
    if (!rd) {
        print("post: Unallocated request data.");
        return ERR;
    }

    // concat server path with resource path
    strcpy(real_path, so.server_root);
    strcat(real_path, rd->path);

    // attempt to read / exec
    file_len = finder_load(real_path, rd->body, rd->body_len, &file_content, &content_type, &check_flag);

    if (file_len == ERR) {
        // very bad things happened somewhere
        print("post: Unexpected error loading resource.");

        // decrease the number of active connections and exit
        tcp_close_socket(sockfd);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    // no such luck
    if (file_len == NOT_FOUND) {
        free(file_content);
        free(content_type);

        // 404, no such resource
        return NOT_FOUND;

        // found the file, but we cannot determine Content-Type
    } else if (file_len == NO_MATCH) {
        free(file_content);
        free(content_type);

        // 415, we don't know what type it is
        return NO_MATCH;

        // script was executed but timed out
    } else if (file_len == TIMEOUT) {

        // 500, script takes too long to respond
        return TIMEOUT;
    }

    // see headers.c
    status = header_build(so, real_path, content_type, file_len, check_flag, check_flag, 0, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for POST response.");
        free(file_content);
        free(content_type);
        return ERR;
    }

    // see http.c
    status = http_response_build(&response, &response_len, rd->version, 200, "OK", 2, num_headers, res_headers, file_content, file_len);
    if (status == ERR) {
        print("Error while creating POST response.");
        free(file_content);
        free(content_type);
        return ERR;
    }

    /////////////////////////////////////////////////
    // Refer to the get function for documentation //
    // on this algorithm                           //
    /////////////////////////////////////////////////

    response_aux = response;
    status = 0;

    do {
        status = tcp_send(sockfd, (const void *)(response_aux + status), (int)response_len - status);
        if (status < 0) {
            print("could not send response (%s:%d).", __FILE__, __LINE__);
            print("errno (send): %s.", strerror(errno));

            free(response);
            free(file_content);
            free(content_type);

            // decrease number of active connections and exit
            tcp_close_socket(sockfd);

            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);

            conc_exit();
        }
        response_aux += status;
        response_len -= status;
    } while (response_len > 0);

    /////////////////////////
    // Algorithm ends here //
    /////////////////////////

    free(response);
    free(file_content);
    free(content_type);

    return OK;
}

/*
 * Description: Sends the very smol response informing of the services
 *              offered by our humble server.
 *
 * In:
 * int sockfd: file descriptor of very polite socket
 * int version: HTTP version
 *
 * Return: ERR if there is an error at any point. OK if all went well.
 * Note: The function has the privilege to exit if there is a failure
 * in critical sections.
 */
int options(int sockfd, int version) {
    int status, num_headers;
    struct http_pairs res_headers[MAX_HEADERS];
    void *response = NULL;
    size_t response_len;

    // construct headers
    status = header_build(so, NULL, NULL, 0, 0, 0, 1, res_headers, &num_headers);
    if (status == ERR) {
        print("Error while creating headers for OPTIONS response.");
        return ERR;
    }

    // construct determined response
    status = http_response_build(response, &response_len, version, 200, "OK", 2, num_headers, res_headers, NULL, 0);
    if (status == ERR) {
        print("Error while creating OPTIONS response.");
        return ERR;
    }

    // no need to cycle through the response, as it will be short enough
    status = tcp_send(sockfd, (const void *)response, (int)response_len);
    if (status < 0) {
        print("could not send response (%s:%d).", __FILE__, __LINE__);
        print("errno (send): %s.", strerror(errno));

        // decrease number of active connections and exit
        tcp_close_socket(sockfd);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    free(response);

    return OK;
}

/*****************************************************************
 * MAIN FUNCTIONS
 *****************************************************************/

/*
 * Description: Runs through our entire implementation of the HTTP protocol.
 * Decomposes a client's requests while they are connected, and serves the
 * resources accordingly.
 *
 * In:
 * void *args: (void*) casted file descriptor of the client's socket
 *
 * Return: No real return code is used, only exiting (as thread).
 */
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

        conc_exit();
    }

    // TODO: establish connection's end
    while (client_alive && active) {
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
            http_request_data_free(&rd);

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

            tcp_close_socket(sockfd);
            http_request_data_free(&rd);

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

        // error came from processing a request, so we will assume it's ill formed
        if (status == ERR) {
            print("Error processing request.");
            // send standard response
            ill_formed_request(sockfd);

            tcp_close_socket(sockfd);
            http_request_data_free(&rd);

            // decrease nconn and exit
            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);
            conc_exit();
        }

        // mark connection as short lived if HTTP version is 1.0
        if (rd.version == 0) {
            client_alive = 0;
        }

        // C doesn't handle string cases, sadly, so let's work around it
        int verb;
        if (strcmp(rd.method, "GET") == 0) {
            verb = GET;
        } else if (strcmp(rd.method, "HEAD") == 0) {
            verb = HEAD;
        } else if (strcmp(rd.method, "POST") == 0) {
            verb = POST;
        } else if (strcmp(rd.method, "OPTIONS") == 0) {
            verb = OPTIONS;
        } else {
            print("Client sent a %s request.", rd.method);
            // send standard response
            unsupported_verb(sockfd, rd.version);

            http_request_data_free(&rd);

            // decrease nconn and exit
            tcp_close_socket(sockfd);

            mutex_lock(&nconn_lock);
            n_conn--;
            mutex_unlock(&nconn_lock);

            conc_exit();
        }

        // answer each request in their own

        // for each entry, error returns will make the logic skip
        // the special status code cases and directly closing
        // the connection with the client
        switch (verb) {
        case GET:
            status = get(sockfd, &rd);
            if (status == ERR) {
                print("GET processing failed.");
                goto end_serve_http;
            }
            break;
        case HEAD:
            status = head(sockfd, &rd);
            if (status == ERR) {
                print("GET processing failed.");
                goto end_serve_http;
            }
            break;
        case POST:
            status = post(sockfd, &rd);
            if (status == ERR) {
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

        // check for incomplete processing
        if (status == NOT_FOUND) {
            resource_not_found(sockfd, rd.version);
        } else if (status == NO_MATCH) {
            unsupported_media_type(sockfd, rd.version);
        } else if (status == TIMEOUT) {
            internal_server_error(sockfd, rd.version);
        }

        // we have processed: begin again (if the client is still there)

    } /* while client_alive && active */


end_serve_http:
    // finished: decrease the number of active connections
    mutex_lock(&nconn_lock);
    n_conn--;
    mutex_unlock(&nconn_lock);

    tcp_close_socket(sockfd);
    http_request_data_free(&rd);

    return NULL;
}

/*******************************
 * CLEANUP FUNCTIONS
 * Compatible with atexit call.
 *******************************/

/*
 * Description: Simply frees the global struct server_options.
 */
void clean_server_options() {
    config_free(&so);
}

/*****************************/
/* Entry point to the server */
/*****************************/
int main(int argc, char *argv[]) {
    int status;

    // config related

    if (argc < 2) {
        status = config_parse("server.conf", &so);
    } else {
        status = config_parse(argv[1], &so);
    }

    // tout_seconds = so->timeout;
    tout_seconds = 3;

    if (status == ERR) {
        printf("Configuration file parsing failed.\n");
        exit(ERR);
    }

    //config_print(&so);

    // always clean this memory
    atexit(clean_server_options);

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
