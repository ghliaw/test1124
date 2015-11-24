#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define SERVER_NAME "www.isu.edu.tw"

int main()
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int ret;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = 0; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    ret = getaddrinfo(SERVER_NAME, NULL, &hints, &result);
    if (ret != 0) {                                                 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));    
        exit(EXIT_FAILURE);
    }
    
    printf("IP Address of %s: %s\n", 
            SERVER_NAME, inet_ntoa(((struct sockaddr_in *)(result->ai_addr))->sin_addr));

}