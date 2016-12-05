#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "8080"

//connections held in queue
#define BACKLOG 10

#define BUFSIZE 8096 // max number of bytes we can get at once 

#define ISspace(x) isspace((int)(x))

void processRequest(char* buf, int sockfd, int size)
{
	char method[5];
	char path[512];
	char version[9];
	int i = 0;
	int j = 0;
	int v = -1;
	while ((i < sizeof(buf) - 1) && (buf[i] != '\n'))
	{
		if(buf[i] == '\r')
		{
			i++;
			continue;
		}
		
		while (!ISspace(buf[i]) && (i < sizeof(method) - 1))
 		{
			method[j] = buf[i];
			i++; j++;
		}
		i++;
		method[j] = '\0';
		//printf("m: %s\n",method);
		j = 0;
		i++;
		while (!ISspace(buf[i]) && (i < sizeof(path) - 1))
 		{
			path[j] = buf[i];
			i++; j++;
		}
		i++;
		path[j] = '\0';
		//printf("p: %s\n",path);
		j = 0;

		while (!ISspace(buf[i]))
 		{
			version[j] = buf[i];
			i++; j++;
		}
		version[j] = '\0';
		//printf("v: %s\n",version);
		break;

	}
	if(!strcasecmp(version, "HTTP/1.0"))
	{
		v = 0;		
	}
	else if(!strcasecmp(version, "HTTP/1.1"))
	{
		v = 1;
	}
	else
	{
		//unkown htttp version
		char toSend[BUFSIZE];
		sprintf(toSend, "%s 400 BAD REQUEST\r\n\r\n",version);
		printf("%s",toSend);
		if (send(sockfd, toSend, sizeof(toSend), 0) == -1)
        	perror("send");
	}

	if(!strcasecmp(path, ""))
	{
		sprintf(path, "index.html");
		//printf("%s\n",path);
	}

	if (!strcasecmp(method, "GET"))
 	{
		//get here
		char data[BUFSIZE];
		int len = readFile(path, data);
		char toSend[BUFSIZE];

		if(len == -1)
		{
			//file not found
			sprintf(toSend, "%s 404 NOT FOUND\r\n\r\n",version);
			printf("%s",toSend);
			if (send(sockfd, toSend, sizeof(toSend), 0) == -1)
                perror("send");
			//printf("wrong path\n");
		}
		else
		{
			sprintf(toSend, "%s 200 OK\r\nContent-Length: %d\r\n\r\n%s",version,len,data);
			printf("data: %s", data);
			printf("%s",toSend);
			if (sendall(sockfd, toSend, &len) == -1) 
			{
				perror("sendall");
				printf("We only sent %d bytes because of the error!\n", len);
			} 
		}
		if(!v)
		{
			close(sockfd);
            exit(0);
		}
		
	}
	else if(!strcasecmp(method, "POST"))
	{
		//post here
		char toSend[BUFSIZE];
		sprintf(toSend, "%s 200 OK\r\n\r\n",version);
		printf("%s",toSend);
		if (send(sockfd, toSend, sizeof(toSend), 0) == -1)
        	perror("send");
		char buf[BUFSIZE];
		int recv_size = recv(sockfd, buf, BUFSIZE-1, 0);
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
		if(!v)
		{
			close(sockfd);
            exit(0);
		}
	}
	else
	{
		//not implemented
		char toSend[BUFSIZE];
		sprintf(toSend, "%s 501 NOT IMPLEMENTED\r\n\r\n",version);
		printf("%s",toSend);
		if (send(sockfd, toSend, sizeof(toSend), 0) == -1)
        	perror("send");
		if(!v)
		{
			close(sockfd);
            exit(0);
		}
	}

}

int readFile(char* fileName, char* buf)
{
    FILE *file = fopen(fileName, "rb");
    size_t n = 0;
    int c;

    if (file == NULL)
        return -1;

    while ((c = fgetc(file)) != EOF)
    {
        buf[n++] = (char)c;
    }


    buf[n] = '\0';
    return (int)n;
	/*
	FILE *file = fopen(fileName, "r");
    if (file == NULL)
        return -1;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	int rdSize;
	fseek(file, 0, SEEK_SET);

	while(!feof(file))
	{
		rdSize = fread(buf, 1, BUFSIZE -1, file);
	}
	fclose(file);
	return size;
	*/
}

int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len)
	{
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void error_die(const char *sc)
{
	perror(sc);
	exit(1);
}

int init_server()
{
    struct addrinfo hints, *servinfo, *p;
	int sockfd;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;       //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;   //TCP
    hints.ai_flags = AI_PASSIVE;       //use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) 
	{
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
	{
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
            perror("server: socket");
            continue;
        }

		//if the port already in use
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
            error_die("setsockopt");
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) 
	{
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
	{
        error_die("listen");
    }
	
	return sockfd;
}

int main()
{
 	int serverfd, clientfd;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;

	serverfd = init_server();
	
	//get rid of zombies
	sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	{
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) 
	{  
        sin_size = sizeof their_addr;
        clientfd = accept(serverfd, (struct sockaddr *)&their_addr, &sin_size);
        if (clientfd == -1) 
		{
            perror("accept");
            continue;
        }

        if (!fork()) //child process
		{
            close(serverfd); // no need for the listener here
			
			while(1)
			{
				fd_set set;
				struct timeval timeout;
				FD_ZERO(&set); // clear the set
				FD_SET(clientfd, &set); // add our file descriptor to the set 
				timeout.tv_sec = 3;
				timeout.tv_usec = 0;
				int rv = select(clientfd + 1, &set, NULL, NULL, &timeout);
				if (rv == -1)
				{
					perror("socket error");
				}
				else if (rv == 0)
				{
					// timeout, socket does not have anything to read
					perror("timeout");
					close(clientfd);
		        	exit(0);
				}
				else
				{
					char buf[BUFSIZE];
					// socket has something to read
					int recv_size = recv(clientfd, buf, BUFSIZE-1, 0);
					if (recv_size == -1)
					{
						// read failed...
						perror("readfail\n");
					}
					else if (recv_size == 0)
					{
						// peer disconnected...
						perror("disconnected");
						close(clientfd);
		        		exit(0);
					}
					else
					{
						// read successful...
						printf("%s", buf);
						processRequest(buf, clientfd, recv_size);
					
					}
				}
            }
            close(clientfd);
            exit(0);
        }
        close(clientfd);  // no need for client socket here
    }

    return 0;
}
