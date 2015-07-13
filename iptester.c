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

#define DEFAULT_SIZE		56			// In bytes
#define DEFAULT_PORT		9050		// In ports?
#define DEFAULT_PKT_COUNT	10			// In packets
#define DEFAULT_TIMEOUT		10			// In seconds

#define TYPE_UNKNOWN		0			// Default, causes prog to fail
#define TYPE_SERVER			1			// Server
#define TYPE_CLIENT			2			// Client

#define FLAG_NONE			0x00
#define FLAG_START			0x01

struct pkt_buffer {
	uint64_t id;						// Id of each packet sent (usually incremented)
	uint8_t	flags;						// Flags, 0x01 -> START
	int16_t timeout;					// Timeout between each packet (in seconds)
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

void run_client(char *ip, short port, short size, int16_t timeout)
{
	int sd;
	struct sockaddr_in servaddr;
	char buffer[BUFFER_SIZE];
	struct pkt_buffer *pkt = (struct pkt_buffer*)buffer;
	pkt->timeout = timeout;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(ip); 

	clock_t start_clock = clock();
	clock_t stop_clock = start_clock + (timeout * CLOCKS_PER_SEC);

	// Send initial packet
	pkt->id = 0;
	pkt->flags = FLAG_START;
	sendto(sd, buffer, sizeof(struct pkt_buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));

	pkt->flags = FLAG_NONE;
	int loop_count;
	//for (loop_count = 0; loop_count < num_pkts; loop_count++) {
	for (loop_count = 1; clock() < stop_clock; loop_count++) {
		pkt->id = loop_count;
		if (sizeof(pkt) < size) {
			// Fill in random info until we reach the appropriate size
			short padd_data_size = size - sizeof(struct pkt_buffer);
			FILE *urandom = fopen("/dev/urandom", "r");
			fread(buffer + sizeof(struct pkt_buffer), padd_data_size, 1, urandom);
			fclose(urandom);
		}

		if (sendto(sd, buffer, size, 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			perror("sendto");
			return;
		}	
	}

	printf("Sent %d Packets\n", loop_count);

	close(sd);
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
	printf("usage: %s [REQURIED] [OPTION]...\n"
		   "A simple UDP client/server for testing packet throughput on a network\n\n"
           "Required:\n"
		   "  -c IP_ADDR\tRun in client mode\n"
           "  -s\t\tRun in server mode\n\n"
		   "Universal Options:\n"
		   "  -p\t\tUDP port to use (default = 9050)\n"
		   "  -h\t\tHelp (Prints this document)\n\n"
           "Client Options\n"
		   "  -t\t\tTime to transmit packets (default = 10 seconds)\n"
		   "  -b\t\tSize of packet to send, in bytes (default = 56 bytes)\n"
		   "\nExamples:\n"
		   "%s -s\t\t\tA default server running on UDP port 9050\n"
		   "%s -s -p 80\t\tUDP server running on port 80\n"
		   "%s -c 10.0.0.5 -t 5\tClient conencting to 10.0.2.5 that runs for 5 seconds\n",
		   progname, progname, progname, progname);
}

int main(int argc, char **argv)
{
	int opt;
	short port = DEFAULT_PORT;			// UDP port number
	short type = TYPE_UNKNOWN;
	short size = DEFAULT_SIZE;
	uint16_t timeout = DEFAULT_TIMEOUT;	
	char *ip_addr;

	while ((opt = getopt(argc, argv, "hb:c:p:st:")) != -1) {
		switch (opt) {
		case 'b':
			size = atoi(optarg);
			break;
		case 'c':
			type = TYPE_CLIENT;
			ip_addr = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			type = TYPE_SERVER;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			printf("Usage: %s\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (type == TYPE_SERVER)
		run_server(port);
	else if (type == TYPE_CLIENT)
		run_client(ip_addr, port, size, timeout);
	else
		print_help(argv[0]);

	return 0;
}
