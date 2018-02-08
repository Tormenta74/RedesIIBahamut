#ifndef _LIBTCP_H
#define _LIBTCP_H

#include <arpa/inet.h>
#include <stdint.h>

#define TCP 6
#define UDP 17

/* Universal functions */

int tcp_open_socket();
int tcp_close_socket(int sockfd);
int tcp_send(int sockfd, const void* msg, int length);
int tcp_receive(int sockfd, void *buffer, int size);

/* Server functions */

int tcp_bind_port(int sockfd, uint32_t addr, uint16_t port);
int tcp_listen(int sockfd, int maxsize);
int tcp_accept(int sockfd, int* new_socket, struct sockaddr_in* addr);

/* Client functions */

int tcp_connect_to(int sockfd, uint32_t addr, uint16_t port);

#endif /*_LIBTCP_H*/
