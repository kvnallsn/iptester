#include <time.h>					// For time(...) and timer functions
#include <errno.h>					// For errno (the variable), EAGAIN, and EWOUDLBLOCK
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>					// For exit
#include <stdlib.h>					// For exit
#include <net/if.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>

#include "ip.h"

#define NO_FLAGS		0
#define DEFAULT_PORT	9050
#define DEFAULT_TIMEOUT	10

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

void run_server(short port, long timeout)
{
	int sd;
	struct sockaddr_storage addr;
	socklen_t socklen = sizeof(struct sockaddr_in);

	char buffer[BUFFER_SIZE];

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return;
	}
	
	struct sockaddr_in bind_addr;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(port);
	bind_addr.sin_addr.s_addr = htons(INADDR_ANY);
	int status = bind(sd, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	if (status < 0) {
		perror("bind");
		return;
	}

	// Set the timeout on the socket
	struct timespec ts;
	ts.tv_sec = timeout;
	ts.tv_nsec = 0;

	uint64_t pkt_count = 0;
	struct pkt_buffer *pkt;
	for (;;) {
		if (recvfrom(sd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr*)&addr, &socklen) < 0) { 
			perror("recvfrom");
			return;
		}

		printf("received connection\n");
		pkt_count++;

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(sd, &rfds);
	
		for (;;) {	
			int retval = pselect(1, &rfds, NULL, NULL, &ts, NULL);
			if (retval == -1) {
				// Error
				perror("select");
				return;
			} else if (retval) {
				// Pkt recv
				if (recvfrom(sd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr*)&addr, &socklen) < 0) {
					perror("recvfrom");
					return;
				}
				pkt = (struct pkt_buffer*)buffer;
				if (++pkt_count == pkt->total)
					break;
			} else {
				// Timeout
				printf("timeout received\n");
				break;
			}
		}
	
		print_stats(pkt, pkt_count);

		// Reset Count
		pkt_count = 0;
	}

	close(sd);

	return;
}

/**
 * Print the help document
 */
void print_help(char *progname)
{
	printf("usage: %s [OPTION]...\n"
		   "A simple UDP server for testing packet throughput on a network\n\n"
		   "Options:\n"
		   "  -p	UDP port to use (default = 9050)\n"
		   "  -t	Timeout in seconds (default = 10)\n"
		   "  -h	Help (Prints this document)\n"
		   "\nExamples:\n"
		   "%s			A default server running on UDP port 9050 with a timeout of 10 seconds\n"
		   "%s -p 80 -t 25		UDP server running on port 80 with a timeout of 25 seconds\n",
		   progname, progname, progname);
}

int main(int argc, char **argv)
{
	int opt;
	short port = DEFAULT_PORT;			// UDP port number
	long timeout = DEFAULT_TIMEOUT;		// In seconds

	while ((opt = getopt(argc, argv, "hp:t:")) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			timeout = atol(optarg);
			break;	
		default:
			printf("Usage: %s\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	run_server(port, timeout);

	return 0;
}
