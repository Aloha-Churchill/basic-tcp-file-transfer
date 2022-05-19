/*
** server.c -- a stream socket server demo
*/

#include <sys/wait.h>
#include <signal.h>
#include "helpers.h"

#define BACKLOG 10   // how many pending connections queue will hold


void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
Recieves request from client, then sends back filesize and finally the file to the client
*/
void send_file(int sockfd){
    int headersize = 256;
    int ret;

    // first recieve request from client
    char recvbuf[HEADER_SIZE];
    recieve_header(sockfd, recvbuf);

    printf("Client request: %s\n", recvbuf);

    // open file that client has requested for reading
    FILE* fp = fopen(recvbuf, "r");
    if(fp == NULL){
        error("Could not open file in server. Ensure that file exists.\n");
    }

    // transmit file size to client, so that client knows how much to recieve for file
    int filesize = get_file_size(fp);

    char filesize_header[HEADER_SIZE];
    bzero(filesize_header, HEADER_SIZE);
    sprintf(filesize_header, "%d", filesize);

    ret = sendall(sockfd, filesize_header, &headersize);

    // send the file to client one piece at a time
    ret = read_file_send(sockfd, fp, filesize);
    if(ret < filesize){
        error("Could not send entire file\n");
    }
    else{
        printf("Successfully sent file of size %d to client\n", filesize);
    }

}

// ./server PORT
int main(int argc, char *argv[])
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int ret;

    if(argc != 2){
        error("Invalid format. Should be: ./server [PORT]\n");
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((ret = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }

    // loop through all the results and connect to the first we can
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd == -1){
        error("Could not create socket\n");
    }
    
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if(ret == -1){
        error("Could not setsockopt\n");
    }

    ret = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (ret  == -1) {
        error("Could not bind\n");
    }

    freeaddrinfo(servinfo); // all done with this structure


    if (listen(sockfd, BACKLOG) == -1) {
        error("Listen failed\n");
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            send_file(new_fd);
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}