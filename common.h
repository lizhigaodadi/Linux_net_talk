#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/select.h>

#define IS_DEBUG 1


void sys_err(char *msg,int flag) {   // my error 
    perror(msg);
    exit(flag);
}

void debug(char *msg) {
    if (IS_DEBUG) printf("%s\n",msg);
}


