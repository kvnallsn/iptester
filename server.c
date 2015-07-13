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

#define DEFAULT_PORT	9050
#define WAIT_FOR_CONN	0
#define TEST_RUNNING	1

#define FLAG_START		0x1

struct pkt_buffer {
	uint64_t id;
	uint8_t flags;
	int16_t timeout;
};

//void print_stats(struct pkt_buffer *pkt, uint64_t pkt_count)
void print_stats(uint64_t last_pkt_id, uint64_t pkt_count)
{
	double recv_rate = (pkt_count / (1.0 * pkt_count));
	printf("=================\n");
	printf("Last Pkt Id:  %ld\n", last_pkt_id);
	printf("Packets Recv: %ld\n", pkt_count);
	printf("Recv Rate: %f%%\n", recv_rate * 100);
	printf("=================\n");
}

void run_server(short port)
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

	uint64_t last_pkt_id = 0;	

	clock_t start_clock;
	clock_t last_pkt_clock;
	float clock_timeout;

	uint64_t pkt_count = 0;
	struct pkt_buffer *pkt;
	for (;;) {
		if (recvfrom(sd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, &socklen) < 0) { 
			perror("recvfrom");
			return;
		}

		pkt = (struct pkt_buffer*)buffer;
		if ((pkt->flags & FLAG_START) != FLAG_START)
			continue;
	
		clock_timeout = (pkt->timeout * 1.0);

		start_clock = clock();
		// Handle the connection we received
		for (;;) {
			if (recvfrom(sd, buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr*)&addr, &socklen) < 0) { 
				// Check to see if we timed out 
				if (errno == EAGAIN) {
					// Has our time for this test expired?
					float time_running = (1.0 * ((float)clock() - (float)start_clock)) / CLOCKS_PER_SEC;
					if (time_running > clock_timeout) {
						print_stats(last_pkt_id, pkt_count);

						pkt_count = 0;
						last_pkt_id = 0;
						start_clock = 0;
						last_pkt_clock = 0;

						break;
					}
				}

				continue;
			}

			pkt = (struct pkt_buffer*)buffer;
			last_pkt_clock = clock();
			last_pkt_id = pkt->id;
			++pkt_count;
		}
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
		   "  -h	Help (Prints this document)\n"
		   "\nExamples:\n"
		   "%s		A default server running on UDP port 9050\n"
		   "%s -p 80 		UDP server running on port 80\n",
		   progname, progname, progname);
}

int main(int argc, char **argv)
{
	int opt;
	short port = DEFAULT_PORT;			// UDP port number

	while ((opt = getopt(argc, argv, "hp:")) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'p':
			port = atoi(optarg);
			break;
		default:
			printf("Usage: %s\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	run_server(port);

	return 0;
}
