// talk-client.c
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

//#define MY_SERVER_PORT      5000
#define MAX_STRING_LENGTH   200  // 換行符號與結尾的0不算在內

// global variables
int sock;

// thread for receiving incoming strings
void * thread_recv(void *data) {
    
    char    buf[MAX_STRING_LENGTH+8];
    int     i, count, run = 1;

    while(1) {
        // 接收echo回來的訊息
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

    while(1) {
        //從鍵盤輸入一行字串
        gets(buf);
        i = strlen(buf);
        buf[i] = '\n';  //把換行符號塞到字串尾端
        buf[i+1] = '\0';  //在換行符號後面補上字串結尾符號
        
        // 將字串送到Server
        ret = send(sock, buf, strlen(buf), 0);
        if(ret < 0) { // error case
            printf("send error: %s\n", strerror(errno));
            break;
        }
    }
}

main(int argc, char **argv) {
    
    struct sockaddr_in  sa;
    int count, ret;
    pthread_t pth_recv, pth_send;
    struct hostent *ghbn;
    struct addrinfo hints, *result;
    
    // check program parameters
    if(argc!=3) {
        printf("Usage: %s <server name> <port>\n", argv[0]);
        return 1;
    }
    // argv[1]: server domain name or ip address
    // argv[2]: server port
    
    // create a new socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        printf("socket create error: %s\n", strerror(errno));
        return 1;
    }

    // obtain server's struct sockaddr by getaddrinfo()
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */
 
    ret = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (ret != 0) {
       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
       return 1;                                       
    }                                                             
    
    // print the ip address of first result
    printf("IP Address of %s: %s\n", 
        argv[1], inet_ntoa(( (struct sockaddr_in *)(result->ai_addr) )->sin_addr));
    
    // connect to server 
    //sa.sin_family = AF_INET;
    //sa.sin_port = htons(MY_SERVER_PORT);
    // sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    //ret = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    ret = connect(sock, result->ai_addr, result->ai_addrlen);
    if(ret < 0) {
        printf("connect error: %s\n", strerror(errno));
        return 1;
    }
    
    // create threads for sending and receiving
    ret = pthread_create(&pth_recv, NULL, thread_recv, NULL);
    if(ret!=0) {
        printf("create receiving thread failed: code = %d, %s", ret, strerror(ret));
        return 1;
    }
    
    ret = pthread_create(&pth_send, NULL, thread_send, NULL);
    if(ret!=0) {
        printf("create sending thread failed: code = %d, %s", ret, strerror(ret));
        return 1;
    }
    
    pthread_join(pth_recv, NULL);
    pthread_join(pth_send, NULL);

}    
