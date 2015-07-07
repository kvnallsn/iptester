#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>

#include "ip.h"

#define NO_FLAGS	0

int main(int argc, char **argv)
{
	int sd;
	struct sockaddr_storage addr;
	socklen_t socklen = sizeof(struct sockaddr_in);

	char buffer[BUFFER_SIZE];

	/*
	char port[] = "3223";
	struct addrinfo hints;
	struct addrinfo *srvinfo;
	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_UNSPEC;		// IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM;		// Datagram sockets only!
	hints.ai_flags = AI_PASSIVE;		
	int status = getaddrinfo(NULL, port, &hints, &srvinfo);
	sd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol);
	*/
	sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sd < 0) {
		perror("socket");
		return -1;
	}
	
	struct sockaddr_in bind_addr;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(3223);
	//bind_addr.sin_addr = 0;
	inet_pton(AF_INET, "127.0.0.1", &(bind_addr.sin_addr));
	//status = bind(sd, srvinfo->ai_addr, srvinfo->ai_addrlen);
	int status = bind(sd, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	if (status < 0) {
		perror("bind");
		return -1;
	}
	//freeaddrinfo(srvinfo);

	int loop_count;
	for (loop_count = 0; loop_count < 10; loop_count++) {
		printf("Waiting for packet...\n");
		if (recvfrom(sd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr*)&addr, &socklen) < 0) { 
			perror("recvfrom");
			return -1;
		} else {
			printf("Got packet!");
		}
	}

	close(sd);

	return 0;
}
