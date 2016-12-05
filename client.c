/*
** client.c -- a stream socket client demo
*/

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

#define PORT "8080"

#define BUFSIZE 8096

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[BUFSIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

	char req[1024];
	sprintf(req, "GET /index.html HTTP/1.1\r\n");
	printf("%s\n",req);

    if (send(sockfd, req, sizeof req, 0) == -1)
                perror("send");

	int recv_size = recv(sockfd, buf, BUFSIZE-1, 0);
	printf("%s\n",buf);
	if (recv_size == -1)
	{
		// read failed...
		perror("readfail\n");
	}
	else if (recv_size == 0)
	{
		// peer disconnected...
		perror("disconnected");
		close(sockfd);
		exit(0);
	}


 	sprintf(req, "GET /path/file.html HTTP/1.1\r\n");
	printf("%s\n",req);

    if (send(sockfd, req, sizeof req, 0) == -1)
                perror("send");

	recv_size = recv(sockfd, buf, BUFSIZE-1, 0);
	printf("%s\n",buf);
	if (recv_size == -1)
	{
		// read failed...
		perror("readfail\n");
	}
	else if (recv_size == 0)
	{
		// peer disconnected...
		perror("disconnected");
		close(sockfd);
		exit(0);
	}

    close(sockfd);

    return 0;
}
