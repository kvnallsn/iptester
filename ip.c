#include <string.h>
#include <arpa/inet.h>

#include "ip.h"

/*
 * Compute the internet checksum, as defined by RFC 1071
 *
 * \param[in]  buff	Buffer to compute checksum of
 * \param[in]  size	Size of buffer
 *
 * \return     Checksum of buffer (in network byte order)
 */
uint16_t csum(uint16_t *buff, int size)
{
	// Break buff into 16-bit chunks and sum
	uint32_t i, sum = 0;
	for (i = 0; i < (size / 2); i++) {
		sum += htons(buff[i]);
	}

	// If an odd buffer size, pad last 8-bits with zeros
	if ((size & 1) == 1) {
		sum += (buff[size - 1] << 8) | 0x00;
	}

	// Handle carry's until there are no more
	do {
		sum = (sum >> 16) + (sum & 0xFFFF);
	} while (sum >> 16 != 0x00);

	// Return checksum, in network byte order
	uint16_t final_sum = htons((uint16_t)~sum);
	return final_sum;
}

uint16_t build_udp(char *buffer, uint8_t *msg, uint16_t msg_len)
{
	struct udp_header *udp = (struct udp_header*)(buffer + sizeof(struct ip_header));
	char *data = buffer + sizeof(struct ip_header) + sizeof(struct udp_header);

	strncpy(data, (char*)msg, msg_len);

	udp->src_port = htons(2332);
	udp->dst_port = htons(3223);
	udp->length = htons(sizeof(struct udp_header) + msg_len);

	return sizeof(struct udp_header) + sizeof(struct ip_header) + msg_len;
}

uint16_t build_icmp_request(char *buffer)
{
	char *icmp_buffer = buffer + sizeof(struct ip_header);
	struct icmp_header *icmp = (struct icmp_header*)(icmp_buffer);

	uint16_t id = 9876;
	uint16_t seq_num = 1;

	icmp->type = htons(8);	// Echo Request
	icmp->code = htons(0);
	icmp->rest = (htons(id) << 16) | (htons(seq_num));
	icmp->checksum = csum((uint16_t*)icmp_buffer, sizeof(struct icmp_header));

	return sizeof(struct icmp_header) + sizeof(struct ip_header);
}

