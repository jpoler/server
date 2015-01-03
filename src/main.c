#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define handle_error(msg)					\
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define BUF_SIZE 500
#define LISTEN_QUEUE_LEN 10

const char response[] = "HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html; charset=UTF-8\r\n\r\n"
	"<doctype !html><html><body><h1>Tada!</h1></body></html>\r\n";

int main(int argc, char *argv[])
{
	int sockfd, peersockfd, res; // socket descriptor
	struct sockaddr_storage peer_addr; // will be filled in by accept when peer connects
	socklen_t peer_addr_len;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	size_t nread;
	char buf[BUF_SIZE];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	memset(&hints, 0, sizeof(struct sockaddr_in)); // clear address structure
	hints.ai_family = AF_UNSPEC; // IPV4 or IPV6
	hints.ai_socktype = SOCK_STREAM; // Needs to be SOCK_STREAM for TCP
	hints.ai_flags = AI_PASSIVE; // wildcard IP address
	
	res = getaddrinfo(NULL, argv[1], &hints, &result); // get a list of potential addresses for the server

	if (res != 0) {
		handle_error("getaddrinfo");
	}

	for (rp = result; rp != NULL; rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol); // get a socket for the given family and type
		if (sockfd == -1) // quit if socket didnt work
			continue;
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) // bind to addr and port given in sockaddr
			break;
		close(sockfd); // got a socket but couldn't bind it, close and try next
	}

	if (rp == NULL) {
		handle_error("Could not bind");
	}

	if (listen(sockfd, LISTEN_QUEUE_LEN) == -1) { // listen with a queue of length LISTEN_QUEUE_LEN
		handle_error("listen");
	}

	/* This is a terrible hack, future servers must parse request from client, 
	   and also ensure that the entire response writes to the socket, which this doesn't
	 */
	
	while (1) {
		peersockfd = accept(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
		if (peersockfd < 0) {
			handle_error("accept");
		}
		/* Error check and make sure that the whole message writes! */
		send(peersockfd, (void *)response, sizeof(response), 0);
	}
	return 0;
}
