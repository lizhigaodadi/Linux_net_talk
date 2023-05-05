#include "../common.h"

#define MAX_CLIENT 1024  //max can accept clients
#define MSG_LENGTH 1024


typedef struct client_data {   // link table for clients
    int sockfd;
    int uuid;
    struct client_data *next;
    char *client_name;    // client name
} client_data_t;

static client_data_t head;  // head of linkedlist 
static volatile int client_num = 0;
static volatile int uuid = 0;
static volatile fd_set read_fds;  // for select to use
static volatile int max_fd = 0; 


typedef struct send_info {
    char *msg;
    int msg_len;
    client_data_t *client_data;
} send_info_t;

static int add_client_data(client_data_t *client_data) {
    // get head of linked list
    client_data_t *node = &head;
    while (node->next != NULL) {
        node = node->next;
    }

    //add 
    node->next = client_data;

    return 0;   // 0 means success -1 means failure
}


static void for_client_data(void (*func)(client_data_t *,void *arg),void *arg) {  // for exec func
    client_data_t *node = &head;
    
    while (node->next != NULL) {

        func(node->next,arg);
        node = node->next;
    }


}

void resend_msg_to_others(client_data_t *client_data, void *arg) {
    printf("now: resend_msg_to_others()\n");
    send_info_t *from = (send_info_t *)arg;
    int fd = client_data->sockfd;
    int uuid = client_data->uuid;
    int err;

    char *msg = NULL;
    printf("try to send messages to others\n");
    //check is the fd
    if (uuid == from->client_data->uuid) {  // check uuid
        printf("found uuid is the same as client_data->uuid");
        return;
    }

    //send msg to others

    msg = from->msg;  // there is some error
    
    printf("msg:%s\n",msg);
    int len = from->msg_len;
    printf("send msg to %d\n",uuid);
    //send message to here
    err = write(fd,msg,len);

    if (err < 0) {
        perror("write error happened");
    }


}


void insert_head(char *str, int str_len,char *target, int target_len) {
    int i;
    for (i=target_len; i>=0; i--) {  // offset target
        *(target + i + str_len) =  *(target + i);
    }

    // insert into target

    for ( i=0; i<target_len; i++) {
        *(target + i) = *(str + i);
    }


    printf("new msg:%s\n",target);

}


void handle_read_event(client_data_t *client_data) {
    //resend these message
    printf("received message from client\n");
    int fd = client_data->sockfd;
    // char *msg = NULL;
    // msg = malloc(MSG_LENGTH);
    char msg[MSG_LENGTH];
    char tmp_msg[MSG_LENGTH];
    int msg_len;

    // if (msg == NULL) {
    //     perror("malloc error happened");
    //     return;
    // }
    memset(msg,0,MSG_LENGTH);
    send_info_t *send_info = malloc(sizeof(send_info_t));

    send_info->client_data = client_data;
    
    int n = read(fd,msg,MSG_LENGTH);
    if (n <= 0) {  //error
        perror("read error");
        return;
    }
    // msg[n-1] = '\0';


    printf("send length:%ld message:%s |\n",strlen(msg),msg);

    if (*msg == '[') {  // this is a name message for the client
        client_data->client_name = malloc(32);
        memset(client_data->client_name, 0,32);
        strcpy(client_data->client_name,msg);
        printf("name_len: %ld client name: %s\n",strlen(client_data->client_name),client_data->client_name);
        return;
    }
    printf("client name: %s\n",client_data->client_name);
    *(msg + strlen(msg) - 1) = '\0';
    strcpy(tmp_msg,client_data->client_name);
    printf("tmp_msg: %s,msg: %s\n",tmp_msg,msg);
    strcat(tmp_msg,msg);

    send_info->msg = tmp_msg;
    send_info->msg_len = strlen(tmp_msg);


    printf("the last message is: %s\n",tmp_msg);

    //resend message
    for_client_data(resend_msg_to_others,send_info);
    

    printf("free msg,send_info~\n");
    // free(msg);  // free memory
    free(send_info);
}

void read_for_fds(client_data_t *client_data,void *arg) {
    //check if need to read
    int fd = client_data->sockfd;
    fd_set *tmp_set = (fd_set *)arg;

    if (FD_ISSET(fd,tmp_set)) {
        handle_read_event(client_data);
    }
}

void *client_handler(void *arg) {
    //epoll io handle
    int err;
    struct timeval time; 

    //init timeval
    bzero(&time,sizeof(time));
    time.tv_sec = 1;
    time.tv_usec = 0;

    //begin to listen
    printf("begin to select socket_fd\n");
    while (1) {
        // printf("waiting for socket\n");
        fd_set tmp_read_set = read_fds; //copy fdset
        err = select(max_fd,&tmp_read_set,NULL,NULL,&time);

        if (err == -1) {
            perror("select error");
            continue;
        }

        if (err == 0) {
            
            continue;
        }

        //find which fd need to read
        for_client_data(read_for_fds,&tmp_read_set);


    }

    
}



static void gracefully_shutdown() {  // close all the file descriptors
    client_data_t *node = &head;
    client_data_t *pre;

    while (node->next != NULL) {
        //close fd
        close(node->sockfd);
        //free
        pre = node;
        node = node->next;
        free(pre);
    }

    head.next = NULL;


    exit(EXIT_SUCCESS);

}


int main(int argc, char **argv) {
    int listen_fd,client_fd;   // client_fd will be repeatedly used
    struct sockaddr_in client_addr,server_addr;  // server and client 's socket addresses
    socklen_t client_addr_len = sizeof(client_addr);  // get client for ipv4 lenght of address
    pthread_mutex_t mutex_for_client_num = PTHREAD_MUTEX_INITIALIZER;
    pthread_t pids;

    //init read_fds
    FD_ZERO(&read_fds);

    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd < 0) {
        sys_err("socket() failed",EXIT_FAILURE);
    }

    bzero(&server_addr,sizeof(server_addr));  // initialize server_addr
    //init server_addr
    server_addr.sin_family = AF_INET;   //set ipv4
    server_addr.sin_port = htons(8080);  //set port to listen
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //accept any ipnet

    //bind to listen
    if (bind(listen_fd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        sys_err("bind failed",EXIT_FAILURE);
    }

    //begin to listen
    if (listen(listen_fd,1) < 0) {
        sys_err("listen failed",EXIT_FAILURE);
    }

    printf("everything is ready\n");

    //new a new thread to listen to message and resend
    pthread_create(&pids,NULL,client_handler,NULL);

    // while(1) to accept message
    while (1) {
        if ((client_fd = accept(listen_fd,(struct sockaddr *)&server_addr,&client_addr_len)) < 0) {
            // contains some error
            perror("accept failed");
            continue;  //
        }
        if (client_num >= MAX_CLIENT) {
            perror ("client num has get max");
            continue;
        }
        // printf("hello world\n");



        //now a new client gets
        pthread_mutex_lock(&mutex_for_client_num);
        client_num++;
        uuid++;   // to means the only one
        max_fd = client_fd + 1;
        printf("client_num: %d\n",client_num);
        pthread_mutex_unlock(&mutex_for_client_num);

        //new a pthread to deal these fd
        printf("a new client is created\n");


        client_data_t *cur_client_data = malloc(sizeof(client_data_t));
        

        if (cur_client_data == NULL) {
            perror("malloc client_data_t failed");
            continue;
        }
        memset(cur_client_data, 0, sizeof(client_data_t));

        cur_client_data->sockfd = client_fd;
        cur_client_data->uuid = uuid;


        // //now get the name of client
        // read(client_fd,cur_client_data->client_name,32);

        

        // printf("get client_name:%s\n",cur_client_data->client_name);

        //add the link list
        if (add_client_data(cur_client_data) < 0) {
            perror("add_client_data failed");
            continue;
        }
        //add fd to read_fds
        FD_SET(client_fd, &read_fds);  // add
        printf("add client_data to link list\n");

    }


    return 0;
}