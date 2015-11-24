// test1.server2.c
// 可接受多個client連線進來，
// 各client傳來的字串均顯示在螢幕上
// 在Server輸入的字串，則傳到所有的client

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MY_SERVER_PORT      5000
#define MAX_STRING_LENGTH   200  // 換行符號與結尾的0不算在內

pthread_mutex_t socket_list_mutex;

// linked list for socket maintance
struct socket_node {
    int                 sock;
    struct socket_node  *next;
};

struct socket_node  *socket_list = NULL;   // point to the head of the linked list

// Utility functions of socket list
// Insert a new node on the front of socket list
void socket_list_insert(int new_sock) {
    struct socket_node *new_node;
    
    new_node = (struct socket_node *) malloc(sizeof(struct socket_node));
    new_node->sock = new_sock;
    new_node->next = socket_list;
    socket_list = new_node;
}

// delete the node of the given socket number
void socket_list_delete(int del_sock) {
    struct socket_node *del_node, *prev_node;
    
    if(socket_list == NULL) return; //空的list就不做任何事
    
    //檢查要殺掉的點是不是第一個
    if(socket_list->sock == del_sock) { //是第一個
       del_node = socket_list;
       socket_list = del_node->next;
       free(del_node);
       return;
    }
    
    // 要殺掉的點不是第一個，所以從頭開始找
    prev_node = socket_list; // 把prev_node設定為串列的第一個node
    del_node = socket_list->next; //把del_node設定為串列的第二個node
    while(del_node != NULL) {
        // 檢查del_node是否符合條件
        if(del_node->sock == del_sock) break; //如果符合，跳出迴圈
        
        //如果不符合，則移動到下一個node繼續找
        prev_node = del_node;
        del_node = del_node->next;
    }
    
    // 如果有找到，就把要殺掉的點從串列中移除
    if(del_node != NULL) {
        prev_node->next = del_node->next;
        free(del_node);
    }
}

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
                
                // 把此socket從socket list拿掉
                pthread_mutex_lock(&socket_list_mutex);
                socket_list_delete(sock);
                pthread_mutex_unlock(&socket_list_mutex);

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
    struct socket_node  *n;

    while(1) {
        //從鍵盤輸入一行字串
        gets(buf);
        i = strlen(buf);
        buf[i] = '\n';  //把換行符號塞到字串尾端
        buf[i+1] = '\0';  //在換行符號後面補上字串結尾符號
        
        // 將字串送到所有的Client
        pthread_mutex_lock(&socket_list_mutex);
        if(socket_list == NULL) {
            pthread_mutex_unlock(&socket_list_mutex);
            continue; //如果沒有client存在就不用送了
        }
        pthread_mutex_unlock(&socket_list_mutex);
        
        // 把字串傳給每一個Client
        pthread_mutex_lock(&socket_list_mutex);
        n = socket_list;
        while(n!=NULL) {
            ret = send(n->sock, buf, strlen(buf), 0);
            if(ret < 0) { // error case
                printf("send error to socket %d: %s\n", n->sock, strerror(errno));
            }
            n = n->next;
        } // while(n...
        pthread_mutex_unlock(&socket_list_mutex);
    }
}

main() {
    
    int                 sock, ret;
    struct sockaddr_in  sa;
    pthread_t pth_recv, pth_send;
    
    // Initial mutex
    pthread_mutex_init(&socket_list_mutex, NULL);
    
    // Create sending thread
    ret = pthread_create(&pth_send, NULL, thread_send, NULL);
    if(ret!=0) {
        printf("create sending thread failed: code = %d, %s", ret, strerror(ret));
        return 1;
    }

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
        
        length = sizeof(newsa);
        new_sock = accept(sock, (struct sockaddr *)&newsa, &length);
        if(new_sock < 0) {
            printf("accept error: %s\n", strerror(errno));
            return 1;
        }
        
        // print who makes this connection (New Connection from <IP Address:port>)
            printf("New Connection from: %s:%d\n", 
                inet_ntoa(newsa.sin_addr), ntohs(newsa.sin_port));
                
        // 把new_sock塞到socket list
        pthread_mutex_lock(&socket_list_mutex);
        socket_list_insert(new_sock);
        pthread_mutex_unlock(&socket_list_mutex);

        
        // create the thread for the new connection
        ret = pthread_create(&pth_recv, NULL, thread_recv, (void *) new_sock);
        if(ret!=0) {
            printf("create receiving thread failed: code = %d, %s", ret, strerror(ret));
            return 1;
        }
    }
}    
