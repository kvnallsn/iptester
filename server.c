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
#define DEFAULT_TIMEOUT	5
#define WAIT_FOR_CONN	-1

struct pkt_buffer {
	uint64_t id;
	uint64_t total;
};

//void print_stats(struct pkt_buffer *pkt, uint64_t pkt_count)
void print_stats(uint64_t last_pkt_id, uint64_t last_pkt_count, uint64_t pkt_count)
{
	double recv_rate = (pkt_count / (1.0 * last_pkt_count));
	printf("=================\n");
	printf("Last Pkt Id:  %ld\n", last_pkt_id);
	printf("Packets Sent: %ld\n", last_pkt_count);
	printf("Packets Recv: %ld\n", pkt_count);
	printf("Recv Rate: %f%%\n", recv_rate);
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

	uint64_t last_pkt_id = 0;	
	uint64_t last_pkt_count = 0;
	int64_t last_pkt_wait = WAIT_FOR_CONN;
	time_t last_pkt_recv;

	uint64_t pkt_count = 0;
	struct pkt_buffer *pkt;
	for (;;) {
		if (recvfrom(sd, buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr*)&addr, &socklen) < 0) { 
			// Check to see if we timed out 
			if (errno == EAGAIN) {
				// Are we waiting to start a connection?
				if (last_pkt_wait == WAIT_FOR_CONN)
					continue;
				// Has our time for this test expired?
				if (time(NULL) > (last_pkt_recv + last_pkt_wait))
					goto reset_conn;
				else
					continue;
			}
			
			perror("recvfrom");
			return;
		}

		pkt = (struct pkt_buffer*)buffer;
		last_pkt_id = pkt->id;
		last_pkt_count = pkt->total;
		last_pkt_wait = DEFAULT_TIMEOUT;
		last_pkt_recv = time(NULL);

		if (++pkt_count < pkt->total)
			continue;

reset_conn:
		print_stats(last_pkt_id, last_pkt_count, pkt_count);
		pkt_count = 0;
		last_pkt_count = 0;
		last_pkt_id = 0;
		last_pkt_wait = WAIT_FOR_CONN;
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
