#include "EchoServerHandler.hh"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include "common.h"

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

EchoServerHandler::EchoServerHandler()
: Handler(-1), accept_fd(-1)
{
}

int EchoServerHandler::init()
{
	int sockfd;  // listen on sock_fd
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			handle_error("socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			handle_error("setsockopt reuse addr");
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &yes,
					sizeof(int)) == -1) {
			handle_error("setsockopt reuse port");
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			handle_error("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		handle_error("server: failed to bind");
	}

	if (listen(sockfd, BACKLOG) == -1) {
		handle_error("listen");
	}

	printf("server: waiting for connections... (pid: %u)\n", getpid());

        accept_fd = sockfd;

        return accept_fd;
}

void EchoServerHandler::handle()
{
    int new_fd;  // new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    sin_size = sizeof their_addr;
    new_fd = accept(accept_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        handle_error("accept");
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s (pid: %u)\n", s, getpid());

    if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");
    close(new_fd);
}

