/*
Client for interacting with temperme.com hardware
Copyright (C) 2011  Ihsan Kehribar <ihsan@kehribar.me>
Copyright (C) 2012  Marcel Hecko <maco@blava.net> 
Copyright (C) 2012  Michal Belica 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


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

int main(int argc, char *argv[])
{

	int sock, bytes_recieved, ret, i;
	char send_data[1024], recv_data[1024], straddr[16];
	struct addrinfo req, *ans, *ai;
	struct sockaddr_in server_addr;
	void *inaddr;

	if (argc <= 1) {
		fprintf(stderr,"Please define the IP address of tempermed server.\n");
		exit(1);
	}

	strcpy(straddr,argv[1]);

	bzero(&req, sizeof(req));
	req.ai_family = AF_UNSPEC;
	req.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo(straddr, "15000", &req, &ans)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	ai = ans;

	if ((sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
		perror("Socket");
	}
	if (connect(sock, ai->ai_addr, sizeof(struct sockaddr)) == -1) {
		perror("Connect");
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
