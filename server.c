#include <time.h>					// for time(...) and timer functions
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
#define TIMEOUT_SEC	10

struct pkt_buffer {
	uint64_t id;
	uint64_t total;
};

void print_stats(struct pkt_buffer *pkt, uint64_t pkt_count)
{
	printf("=================\n");
	printf("Last Pkt Id:  %ld\n", pkt->id);
	printf("Packets Sent: %ld\n", pkt_count);
	printf("Packets Recv: %ld\n", pkt->total);
	printf("Rate: \n");
	printf("=================\n");
}

int main(int argc, char **argv)
{
	int sd;
	struct sockaddr_storage addr;
	socklen_t socklen = sizeof(struct sockaddr_in);

	char buffer[BUFFER_SIZE];

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return -1;
	}
	
	struct sockaddr_in bind_addr;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(32230);
	bind_addr.sin_addr.s_addr = htons(INADDR_ANY);
	int status = bind(sd, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	if (status < 0) {
		perror("bind");
		return -1;
	}

	// Set the timeout on the socket
	struct timeval tv;
	tv.tv_sec = TIMEOUT_SEC;
	tv.tv_usec = 0;				/* Init to 0 to avoid random data being filled in */
	
	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

	uint64_t pkt_count = 0;
	struct pkt_buffer *pkt;
	for (;;) {
		if (recvfrom(sd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr*)&addr, &socklen) < 0) { 
			perror("recvfrom");
			return -1;
		}

		pkt_count++;
		// receive until timeout or reach packet count (if spec'd)
		// probably a better way to do this...but quick n' dirty :)
		time_t stop = time(NULL) + TIMEOUT_SEC;
		for (;;) {
			if (recvfrom(sd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr*)&addr, &socklen) < 0) { 
				perror("recvfrom");
				return -1;
			} 

			pkt = (struct pkt_buffer*)buffer;

			// If he hit the timeout wall, stop
			if ((++pkt_count == pkt->total) || (time(NULL) >= stop))
				break;
			
		}

		print_stats(pkt, pkt_count);

		// Reset Count
		pkt_count = 0;
	}

	close(sd);

	return 0;
}
