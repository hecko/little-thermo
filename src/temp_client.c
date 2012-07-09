/* tcpclient.c */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define DEBUG 1

int main()
{

	int sock, bytes_recieved, ret;
	char send_data[1024], recv_data[1024], straddr[INET6_ADDRSTRLEN];
	struct addrinfo req, *ans, *ai;
	struct sockaddr_in server_addr;
	void *inaddr;

	bzero(&req, sizeof(req));
	req.ai_family = AF_UNSPEC;
	req.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo("localhost", "15000", &req, &ans)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}
	ai = ans;
	while (ai) { /* try all found addresses */
#ifdef DEBUG
		if (ai->ai_family == AF_INET) {
			inaddr = &(((struct sockaddr_in *)ai->ai_addr)->sin_addr);
		} else if (ai->ai_family == AF_INET6) {
			inaddr = &(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr);
		} else {
			inaddr = NULL;
		}
		//fuh, is there a simpler way to do this?
		if (inet_ntop(ai->ai_family, inaddr, straddr, INET6_ADDRSTRLEN) == NULL) {
			perror("inet_ntop");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "Trying %s\n", straddr );
#endif

		if ((sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
			perror("Socket");
			ai = ans->ai_next;
			continue;
		}
		if (connect(sock, ai->ai_addr, sizeof(struct sockaddr)) == -1) {
			perror("Connect");
			ai = ans->ai_next;
			continue;
		}
	}
	if (!ai) {
		fprintf(stderr, "Could not connect to host.\n");
		exit(EXIT_FAILURE);
	}

	while (1) {

		bytes_recieved = recv(sock, recv_data, 1024, 0);
		recv_data[bytes_recieved] = '\0';

		//if (strcmp(recv_data , "q") == 0 || strcmp(recv_data , "Q") == 0)
		//{
		printf("%s", recv_data);
		close(sock);
		break;
		//}

		//else
		// printf("\n%s\n" , recv_data);

		//printf("\nSEND (q or Q to quit) : ");
		//gets(send_data);

		//if (strcmp(send_data , "q") != 0 && strcmp(send_data , "Q") != 0)
		// send(sock,send_data,strlen(send_data), 0); 

		//else
		//{
		//send(sock,send_data,strlen(send_data), 0);   
		// close(sock);
		// break;
		//}

	}
	return 0;
}
// vim: noet:sw=4:ts=4:sts=0
