// test1-client.c
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MY_SERVER_PORT      5000
#define MAX_STRING_LENGTH   200  // 換行符號與結尾的0不算在內

main() {
    
    int sock;
    struct sockaddr_in  sa;
    int count, ret;
    char    buf[MAX_STRING_LENGTH+8];
    int i = 0;
   
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        printf("socket create error: %s\n", strerror(errno));
        return 1;
    }

    // connect to server 
    sa.sin_family = AF_INET;
    sa.sin_port = htons(MY_SERVER_PORT);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if(ret < 0) {
        printf("connect error: %s\n", strerror(errno));
        return 1;
    }
    
    // 無限迴圈:從鍵盤輸入一行字串，送到server，接收echo回來的訊息並印出
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

        // 接收echo回來的訊息
        // keep reading until '\n' appears
        i = 0;
        while(1) {
            // read one byte from the socket
            count = recv(sock, &buf[i], 1, 0);
        
            if(count <= 0) { // error case
                printf("recv error: %s\n", strerror(errno));
                close(sock);
                return 1;
            }
            
            if(buf[i]=='\n') break;
            i++;
        } // end while(1)
        
        // 印出訊息
        i++;
        buf[i] = '\0';
        printf("%s\n", buf);
    }
}