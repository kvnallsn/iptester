#include <time.h>						// Clock Info
#include <stdio.h>
#include <stdlib.h>
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

#define DEFAULT_SIZE		56			// In bytes
#define DEFAULT_PORT		9050		// In ports?
#define DEFAULT_PKT_COUNT	10			// In packets
#define DEFAULT_TIMEOUT		10			// In seconds

#define FLAG_NONE			0x00
#define FLAG_START			0x01

struct pkt_buffer {
	uint64_t id;						// Id of each packet sent (usually incremented)
	uint8_t	flags;						// Flags, 0x01 -> START
	int16_t timeout;					// Timeout between each packet (in seconds)
};

void send_packets(char *ip, short port, short size, int16_t timeout)
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

void print_help(char *progname)
{
     printf("usage: %s [OPTION]... <IP Address>\n"
            "A simple UDP client for testing packet throughput on a network\n\n"
            "Options:\n"
            "  -n    Number of packets to send (default = 1000)\n"
            "  -p    UDP port to use (default = 9050)\n"
			"  -s    Size of packets to send (default = 56 bytes)\n"
			"  -t    Timeout between each packet (default = 10 seconds)\n"
            "  -h    Help (Prints this document)\n"
            "\nExamples:\n"
            "%s -p 80 -n 200 10.0.2.5     UDP client running on port 80 sending to 10.0.2.5\n",
            progname, progname);

}

int main(int argc, char **argv)
{
	int opt;
	char *ip = NULL;
	short port = DEFAULT_PORT;
	short size = DEFAULT_SIZE;
	int16_t timeout = DEFAULT_TIMEOUT;

	while (optind < argc) {
		if((opt = getopt(argc, argv, "hn:p:s:t:")) != -1) {
			switch (opt) {
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case 'p':
				port = atoi(optarg);
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			default:
				print_help(argv[0]);
				exit(EXIT_FAILURE);
			}
		} else {
			ip = argv[optind++];
		}
	}

	if (ip == NULL) {
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}

	send_packets(ip, port, size, timeout);

	return 0;
}
