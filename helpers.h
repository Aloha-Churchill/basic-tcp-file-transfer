#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define FILE_SIZE_PART 1000
#define HEADER_SIZE 256


/*
Wrapper function for error messages
*/
void error(char* message){
    perror(message);
    exit(1);
}

/*
Stores header buf into header
*/
void recieve_header(int sockfd, char* header_buf){
    bzero(header_buf, HEADER_SIZE);

    int n = 0;
    while(n < HEADER_SIZE){
        n += recv(sockfd, header_buf + n, HEADER_SIZE, 0);
    }
}

/*
helper function from Beej's guide to network programming
*/
int sendall(int s, char *buf, int *len){
    int total = 0; // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n = 0;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    *len = total;
    return n==-1?-1:0; // return -1 on failure, 0 on success
}

/*
helper function to get file size
*/
int get_file_size(FILE* file_fp){
    int filesize = -1;
    fseek(file_fp, 0L, SEEK_END);
    filesize = ftell(file_fp);
    fseek(file_fp, 0L, SEEK_SET);

    if(filesize == -1){
        error("Could not get length of file");
    }

    return filesize;    
}

/*
helper function to read from a local file and then send it to client/server over tcp
*/
int read_file_send(int sockfd, FILE* file_fp, int filesize){
    int total_num_sent = 0;

    int num_sends = filesize/FILE_SIZE_PART + ((filesize % FILE_SIZE_PART) != 0); 
    printf("Number of file pieces to send to client: %d. Each file segment is: %d bytes\n", num_sends, FILE_SIZE_PART);
    char file_contents[FILE_SIZE_PART];
    int n = 0;
    int res;

    for(int i=0; i < num_sends; i++){
        bzero(file_contents, FILE_SIZE_PART);
        // send remainder of file
        if(i == num_sends - 1){
            n = fread(file_contents, 1, filesize%FILE_SIZE_PART, file_fp);
        }
        else{
            n = fread(file_contents, 1, FILE_SIZE_PART, file_fp);
        }
        
        if( n < 0){
            error("Could not fread\n");
        }

        res = sendall(sockfd, file_contents, &n);
        if(res < 0){
            error("error in sendall\n");
        }

        total_num_sent += n;
    }

    return total_num_sent;
}

/*
helper function to recieve from client/server and then write to local file
*/
int recv_write_file(int sockfd, FILE* fp, int filesize){
	int total_num_written = 0;
    
    char recvbuf[FILE_SIZE_PART];
    int n;

    while(total_num_written < filesize){
        bzero(recvbuf, FILE_SIZE_PART);
        n = recv(sockfd, recvbuf, FILE_SIZE_PART, 0);
        
        if(n < 0){
            error("Error in recv\n");
        }
        if(fwrite(recvbuf, 1, n, fp) < 0){
            error("Error in fwrite\n");
        }		
        total_num_written += n;
    } 

    return total_num_written;
}