#include <sys/socket.h>
#include <unistd.h>

#include "globals.h"
#include "libtcp.h"

/***********************************************
 * CLIENT AND SERVER SIDE
 */

/*
 * Description: Opens a Unix socket with TCP/IP options.
 *
 * Return: The file descriptor of the opened socket. If it's
 *         not a non negative value, it's an error.
 */
int tcp_open_socket() {
    return socket(AF_INET, SOCK_STREAM, TCP);
}

/*
 * Description: Closes the socket (file descriptor) specified.
 *
 * In:
 * int sockfd: file descriptor number to be closed
 *
 * Return: 0 on success, -1 on error.
 */
int tcp_close_socket(int sockfd) {
    return close(sockfd);
}

/*
 * Description: Sends the contents of a generic buffer through a socket (in
 *              TCP mode, i.e. blocking).
 *
 * In:
 * int sockfd: file descriptor of the socket to send through
 * const void *msg: pointer to the memory zone in which lies the message
 * int length: length of that memory zone (or lower) in bytes
 *
 * Return: The number of bytes sent. -1 on error.
 */
int tcp_send(int sockfd, const void *msg, int length) {
    // sanity check
    if(sockfd < 0 || msg == NULL || length <= 0) {
        return ERR;
    }
    // blocking call
    return send(sockfd, msg, (size_t)length, 0);
}

/*
 * Description: Stores the message received through a socket of maximum
 *              length of "size" in the "buffer" memory address.
 *
 * In:
 * int sockfd: file descriptor of the socket to receive from
 * void *buffer: generic pointer to the memory address to store incoming
 *               data
 * int size: maximum length of the message
 *
 * Return: The number of bytes received. -1 on error.
 */
int tcp_receive(int sockfd, void *buffer, int size) {
    // sanity check
    if(sockfd < 0 || buffer == NULL || size <= 0) {
        return ERR;
    }
    // blocking call
    return recv(sockfd, buffer, (size_t)size, 0);
}

/***********************************************
 * SERVER SIDE
 */

/*
 * Description: "Links" the external port to a socket, so that connections
 *              to that port will happen in the specified socket. This is
 *              used to establish a "master" socket through which clients
 *              will connect with the server.
 *
 * In:
 * int sockfd: file descriptor of the socket to bind
 * uint32_t addr: IP address to be bound to the socket (generally INADDR_ANY)
 * uint16_t port: port number to be bound to the socket
 *
 * Return: ERR in case of failure. OK otherwise.
 */
int tcp_bind_port(int sockfd, uint32_t addr, uint16_t port) {
    struct sockaddr_in sock;

    // sanity check
    if(sockfd < 0 || htons(port) < 0) {
        return ERR;
    }

    // configure address
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);          // convert to network order (short)
    sock.sin_addr.s_addr = htonl(addr);   // convert to network order (long)
    if(bind(sockfd, (struct sockaddr*)&sock, sizeof sock)) {
        return ERR;
    }
    return OK;
}

/*
 * Description: Marks the socket as ready to accept clients. Call only in a bound
 *              port.
 *
 * In:
 * int sockfd: file descriptor of the socket
 * int maxsize: maximum number of clients that the server can enqueue
 *
 * Return: 0 in case of success. -1 on error.
 */
int tcp_listen(int sockfd, int maxsize) {
    // sanity check
    if(sockfd < 0 || maxsize <= 0) {
        return ERR;
    }
    return listen(sockfd, maxsize);
}

/*
 * Description: Blocking call that returns when a client attempts a connection.
 *              It establishes the socket to which the client has been redirected.
 *              Call only in a socked marked by listen.
 *
 * In:
 * int sockfd: file descriptor of the connection socket
 * int *new_socket: integer memory address to write the redirected socket
 * struct sockaddr_in *addr: address information struct that gets filled with
 *     the client's info (address, port)
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
int tcp_accept(int sockfd, int *new_socket, struct sockaddr_in *addr) {
    socklen_t addr_len;

    // sanity check
    if(sockfd < 0 || new_socket == NULL || addr == NULL) {
        return ERR;
    }

    addr_len = sizeof(struct sockaddr_in); /* en un principio */

    // assume error
    *new_socket = -1;
    // accept returns the new socket's file descriptor
    if((*new_socket = accept(sockfd, (struct sockaddr*)addr, &addr_len)) == ERR) {
        return ERR;
    }

    return OK;
}

/***********************************************
 * CLIENT SIDE
 */

/*
 * Description: Solicitates a connection to the server at "addr":"port" IP address.
 *
 * In:
 * int sockfd: file descriptor of the client's local socket
 * uint32_t addr: 32 bytes (network order) of the requested IP address
 *     (reasoning: it can be used directly in network order because it is used
 *      with DNS functions. Not too relevant to this project.)
 * uint16_t port: 16 bytes (host order) of the requested port
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
int tcp_connect_to(int sockfd, uint32_t addr, uint16_t port) {
    struct sockaddr_in sock;

    // sanity check
    if(sockfd < 0 || htons(port) < 0) {
        return ERR;
    }

    // configure desired address
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    sock.sin_addr.s_addr = addr;

    // blocking call
    if(connect(sockfd, (struct sockaddr*)&sock, sizeof sock)) {
        return ERR;
    }

    return OK;
}

