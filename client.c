#include <stdio.h>
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

int main(int argc, char **argv)
{
	int sd;
	struct sockaddr_in servaddr;
	char buffer[] = "Hello, world!";

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	//int srcport = 23320;
	int dstport = 32230;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(dstport);
	servaddr.sin_addr.s_addr = inet_addr("131.214.252.44");

	int loop_count;
	for (loop_count = 0; loop_count < 10; loop_count++) {
		printf("Sending...\n");
		if (sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			perror("sendto");
			return -1;
		}	
	}

	close(sd);

	return 0;
}
