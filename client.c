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

#define DEFAULT_PORT		9050
#define DEFAULT_PKT_COUNT	10

struct pkt_buffer {
	uint64_t id;
	uint64_t total;
};

void send_packets(char *ip, short port, int num_pkts)
{
	int sd;
	struct sockaddr_in servaddr;
	char buffer[BUFFER_SIZE];
	struct pkt_buffer *pkt = (struct pkt_buffer*)buffer;
	pkt->total = num_pkts;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(ip); 

	int loop_count;
	for (loop_count = 0; loop_count < num_pkts; loop_count++) {
		pkt->id = loop_count;
		if (sendto(sd, buffer, sizeof(struct pkt_buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			perror("sendto");
			return;
		}	
	}

	close(sd);
}

void print_help(char *progname)
{
     printf("usage: %s [OPTION]...\n"
            "A simple UDP client for testing packet throughput on a network\n\n"
            "Options:\n"
			"  -a	 IP Address to send packets to"
            "  -n    Number of packets to send (default = 1000)\n"
            "  -p    UDP port to use (default = 9050)\n"
            "  -h    Help (Prints this document)\n"
            "\nExamples:\n"
            "%s -a 10.0.2.5 -p 80     UDP client running on port 80 sending to 10.0.2.5\n",
            progname, progname);

}

int main(int argc, char **argv)
{
	int opt;
	char ip[] = "255.255.255.255";
	short port = DEFAULT_PORT;
	int num_pkts = DEFAULT_PKT_COUNT;

	while ((opt = getopt(argc, argv, "a:h::n::p::")) != -1) {
		switch (opt) {
		case 'a':
			strncpy(ip, optarg, strlen(ip));
			break;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'n':
			num_pkts = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			print_help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	send_packets(ip, port, num_pkts);

	return 0;
}
