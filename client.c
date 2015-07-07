#include <stdio.h>
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
	struct sockaddr_in sin;
	char buffer[BUFFER_SIZE];

	sd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
	if (sd < 0) {
		perror("socket");
		return -1;
	}

	int srcport = 23320;
	int dstport = 32230;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(dstport);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	char *ip_buffer = buffer;
	struct ip_header *ip = (struct ip_header*)ip_buffer;

	char src_ip[] = "127.0.0.1";
	char dst_ip[] = "127.0.0.1";
	ip->vers_ihl = (0x04 << 4) | (0x05);
	ip->dscp_ecn = 0x10;
	ip->id = htons(1234);
	ip->flags_offset = 0;
	ip->ttl = 64;
	ip->proto = PROTO_UDP;
	ip->src = inet_addr(src_ip);
	ip->dst = inet_addr(dst_ip);
	ip->checksum = 0x0000;
	
	int one = 1;
	const int *val = &one;
	if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
		perror("setsockopt() error");
		return -1;
	}

	int loop_count;
	for (loop_count = 0; loop_count < 10; loop_count++) {
		uint16_t total_len = build_udp(ip_buffer, (uint8_t*)"Hello, world!", 13, srcport, dstport);
		ip->length = total_len;
		ip->checksum = csum((uint16_t*)ip_buffer, sizeof(struct ip_header));

		total_len += sizeof(struct ether_header);

		printf("Sending...\n");
		if (sendto(sd, buffer, total_len, 0, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
			perror("sendto");
			return -1;
		}	
	}

	close(sd);

	return 0;
}
