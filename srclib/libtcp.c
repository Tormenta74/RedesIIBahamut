#include <sys/socket.h>
#include <unistd.h>

#include "globals.h"
#include "libtcp.h"

int tcp_open_socket() {
    return socket(AF_INET, SOCK_STREAM, TCP);
}

int tcp_close_socket(int sockfd) {
    return close(sockfd);
}

int tcp_send(int sockfd, const void* msg, int length) {
    if(sockfd < 0 || msg == NULL || length <= 0)
        return ERR;
    return send(sockfd,msg,(size_t)length,0);
}

int tcp_receive(int sockfd, void *buffer, int size) {
    if(sockfd < 0 || buffer == NULL || size <= 0)
        return ERR;
    return recv(sockfd,buffer,(size_t)size,0);
}

int tcp_bind_port(int sockfd, uint32_t addr, uint16_t port) {
    if(sockfd < 0 || htons(port) < 0)
        return ERR;
    struct sockaddr_in sock;
    sock.sin_family=AF_INET;
    sock.sin_port=htons(port);
    sock.sin_addr.s_addr=htonl(addr);
    if(bind(sockfd, (struct sockaddr*)&sock,sizeof sock))
        return ERR;
    return OK;
}

int tcp_listen(int sockfd, int maxsize) {
    if(sockfd < 0 || maxsize <= 0)
        return ERR;
    return listen(sockfd,maxsize);
}

int tcp_accept(int sockfd, int* new_socket, struct sockaddr_in* addr) {
    if(sockfd < 0 || new_socket == NULL || addr == NULL)
        return ERR;
    socklen_t addr_len;
    addr_len=sizeof(struct sockaddr_in); /* en un principio */
    *new_socket=-1;
    if((*new_socket=accept(sockfd,(struct sockaddr*)addr,&addr_len))==ERR)
        return ERR;
    return OK;
}

int tcp_connect_to(int sockfd, uint32_t addr, uint16_t port) {
    if(sockfd < 0 || htons(port) < 0)
        return ERR;
    struct sockaddr_in sock;
    sock.sin_family=AF_INET;
    sock.sin_port=htons(port);
    sock.sin_addr.s_addr=addr;
    if(connect(sockfd,(struct sockaddr*)&sock,sizeof sock))
        return ERR;
    return OK;
}

