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

// thread for receiving incoming strings
void * thread_recv(void *data) {
    
    char    buf[MAX_STRING_LENGTH+8];
    int     i, count, run = 1;
    int     sock;

    sock = (int) data;
    
    while(1) {
        // 接收進來的訊息
        // keep reading until '\n' appears
        i = 0;
        while(1) {
            // read one byte from the socket
            count = recv(sock, &buf[i], 1, 0);
        
            if(count <= 0) { // error case
                printf("recv error: %s\n", strerror(errno));
                close(sock);
                run = 0;
                break;
            }
            
            if(buf[i]=='\n') break;
            i++;
        } // end while(1)
        if (run == 0) break;
        
        // 印出訊息
        i++;
        buf[i] = '\0';
        printf("%s\n", buf);
    } // while(run)
}

// thread for sending inputed string
void * thread_send(void *data) {

    char    buf[MAX_STRING_LENGTH+8];
    int     i = 0, ret;
    int     sock;

    sock = (int) data;

    while(1) {
        //從鍵盤輸入一行字串
        gets(buf);
        i = strlen(buf);
        buf[i] = '\n';  //把換行符號塞到字串尾端
        buf[i+1] = '\0';  //在換行符號後面補上字串結尾符號
        
        // 將字串送到Client
        ret = send(sock, buf, strlen(buf), 0);
        if(ret < 0) { // error case
            printf("send error: %s\n", strerror(errno));
            break;
        }
    }
}

main() {
    
    int                 sock, ret;
    struct sockaddr_in  sa;
    pthread_t pth_recv, pth_send;
    
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
        ret = pthread_create(&pth_recv, NULL, thread_recv, (void *) new_sock);
        if(ret!=0) {
            printf("create receiving thread failed: code = %d, %s", ret, strerror(ret));
            return 1;
        }
        ret = pthread_create(&pth_send, NULL, thread_send, (void *) new_sock);
        if(ret!=0) {
            printf("create sending thread failed: code = %d, %s", ret, strerror(ret));
            return 1;
        }
    }
}    
