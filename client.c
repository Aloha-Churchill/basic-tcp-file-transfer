/*
** client.c -- a stream socket client demo
*/

#include "helpers.h"



/*
Send request to server with filename, then recieve filesize, then recieve and write copy of file
*/
void get_file(int sockfd, char* filename){
    int res;
    int headersize = 256;

    // send request for filename
    res = sendall(sockfd, filename, &headersize);
    if(res < 0){
        error("Could not sendall\n");
    }

    // get filesize expected
    char recvbuf[HEADER_SIZE];
    recieve_header(sockfd, recvbuf);
    int filesize = atoi(recvbuf);
    printf("Number of bytes expected: %d\n", filesize);

    
    // create file for reading under correct directory
    char pathname[HEADER_SIZE];
    bzero(pathname, HEADER_SIZE);
    strcpy(pathname, "files_from_server/");
    strcat(pathname, filename);

    FILE* fp = fopen(pathname, "w+");

    // recieve remote file and write to local file
    res = recv_write_file(sockfd, fp, filesize);
    if(res != filesize){
        error("Did not recieve entire file\n");
    }
    else{
        printf("Successfully saved copy of file of length %d bytes.\n", res);
    }

    fclose(fp);
}


// ./client [IP_ADDR] [PORT] [FILENAME]
int main(int argc, char *argv[])
{
    int sockfd;  
    struct addrinfo hints, *servinfo;
    int ret;

    if (argc != 4) {
        error("Invalid format. Should be: ./client [IP_ADDR] [PORT] [FILENAME]\n");
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo(argv[1], argv[2], &hints, &servinfo);
    if (ret != 0) {
        error("Could not resolve address information\n");
    }

    // loop through all the results and connect to the first we can
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd == -1){
        error("Could not create socket\n");
    }

    ret = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (ret  == -1) {
        error("Could not connect to server\n");
    }

    freeaddrinfo(servinfo); // all done with this structure

    get_file(sockfd, argv[3]);

    close(sockfd);

    return 0;
}