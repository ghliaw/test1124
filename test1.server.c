// test1.server.c
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MY_SERVER_PORT      5000
#define MAX_STRING_LENGTH   200  // 換行符號與結尾的0不算在內

void * thread_echo(void *data) {
    int sock;
    int count, ret;
    char    buf[MAX_STRING_LENGTH+8];
    int i = 0;
    
    sock = (int) data;
    
    while(1) {
        
        strcpy(buf, "echo :");
        i = strlen(buf);
        
        // keep reading until '\n' appears
        while(1) {
            // read one byte from the socket
            count = recv(sock, &buf[i], 1, 0);
        
            if(count <= 0) { // error case
                printf("recv error: %s\n", strerror(errno));
                close(sock);
                return;
                //pthread_exit(1);
            }
            
            if(buf[i]=='\n') break;
            i++;
        } // end while(1)
        
        // Append string end ('\0') in the end of the string in buf
        i++;
        buf[i] = '\0';
        
        // send the string back to the client
        ret = send(sock, buf, strlen(buf), 0);
        if(ret < 0) { // error case
            printf("send error: %s\n", strerror(errno));
            break;
        }
    }
    
    close(sock);
}

main() {
    
    int                 sock, ret;
    struct sockaddr_in  sa;
    pthread_t pth;
    
    // Create a stream socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        printf("socket create error: %s\n", strerror(errno));
        return 1;
    }
    
    // bind at server port
    sa.sin_family = AF_INET;
    sa.sin_port = htons(MY_SERVER_PORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sock, (struct sockaddr *)&sa, sizeof(sa));
    if(ret < 0) {
        printf("bind error: %s\n", strerror(errno));
        return 1;
    }
    
    // enter listening mode
    ret = listen(sock, 5);
    if(ret < 0) {
        printf("listen error: %s\n", strerror(errno));
        return 1;
    }
    
    // accept new conenctions and spawn one thread for each new connection
    while(1) {
        struct sockaddr_in  newsa;
        int                 length;
        int                 new_sock;
        
        new_sock = accept(sock, (struct sockaddr *)&newsa, &length);
        if(new_sock < 0) {
            printf("accept error: %s\n", strerror(errno));
            return 1;
        }
        
        // print who makes this connection (New Connection from <IP Address:port>)
        printf("New Connection from: %s:%d\n", 
            inet_ntoa(newsa.sin_addr), ntohs(newsa.sin_port));
        
        // create the thread for the new connection
        ret = pthread_create(&pth, NULL, thread_echo, (void *) new_sock);
        if(ret!=0) {
            printf("pthread_create() failed: code = %d, %s", ret, strerror(ret));
            return 1;
        }
    }
}    
