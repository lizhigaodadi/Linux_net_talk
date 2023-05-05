#include "../common.h"
#include <pthread.h>

#define MSG_LEN 1024

static char msg_buf[MSG_LEN] = "hello world"; //msg_buf

void *handle_server(void *arg) {  //server's fd
    int *server_fd = (int *)arg;
    char accept_msg[MSG_LEN];
    bzero(accept_msg, MSG_LEN);
    while (1) {  // listen for message
        if (read(*server_fd,accept_msg,MSG_LEN-1) <= 0) {
            continue;
        }

        //show message
        printf("%s\n",accept_msg);

    }


}

int main(int argc,char **argv) {   // as client to connect server
    int server_fd;
    struct sockaddr_in server_addr;
    int n;  // number of send message
    pthread_t tidp;
    char name[32];
    //malloc msg_len
    *name =  '[';

    //from terminal get some message

    while (1) {
        printf("please input your name:(ensure your name not over 30!)\n");
        fgets(name + 1,32,stdin);
        if (strlen(name + 1) > 30) {
            printf("your name is too long!\n");
        } else {
            break;
        }
    }
    name[strlen(name) -1] = ']';

    printf("len: %ld your name is %s|\n",strlen(name),name);

    //

    server_fd = socket(AF_INET,SOCK_STREAM ,0);
    if (server_fd < 0) {
        sys_err("socket() failed",EXIT_FAILURE);
    }

    if (msg_buf == NULL) {  // alloc memory failure
        sys_err("malloc failed",EXIT_FAILURE);
    }

    //initialize
    bzero(&server_addr,sizeof(struct sockaddr_in));

    //set client address
    server_addr.sin_family = AF_INET;  // set ipv4
    server_addr.sin_port = htons(8080);  
    inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr);

    if (connect(server_fd,(struct sockaddr *) &server_addr,sizeof(struct sockaddr_in)) < 0) {
        sys_err("connect() failed",EXIT_FAILURE);
    }

    //send our name to server to contain

    printf("we successfully connected\n");

    //send your name to server 
    n = write(server_fd,name,strlen(name));
    if (n < 0) {
        sys_err("send name failed",EXIT_FAILURE);
    }


    //we need to prepare two thread to deal with server
    //create a new thread
    pthread_create(&tidp, NULL, handle_server,&server_fd);

    printf("please input your message to send to the server,input eixt() means you want to stop\n");
    while (1) {
        //send message to server 
        bzero(msg_buf,MSG_LEN);
        fgets(msg_buf,MSG_LEN-1,stdin);

        n = write(server_fd,msg_buf,strlen(msg_buf));
        // printf("successfully send a message to the server\n");
        if (n <= 0) {
            perror("write() failed");
        }
    }


    //now we connect to the server successfully
    // n = write(server_fd,msg_buf,strlen(msg_buf));
    // if (n <  0) {
    //     sys_err("write() failed",EXIT_FAILURE);
    // }
    // printf("send message to server %s \n",msg_buf);

    //close and free memory
    close(server_fd);

}