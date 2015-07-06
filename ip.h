#ifndef _AFRL_RITD_IP_
#define _AFRL_RITD_IP_

#include <stdint.h>

#define PROTO_ICMP	1
#define PROTO_TCP	6
#define PROTO_UDP	17	
#define BUFFER_SIZE	8192

struct ip_header {
	uint8_t vers_ihl;  	// version: upper 4, ihl: lower 4
	uint8_t dscp_ecn;		// dscp: upper 6; ecn: lower 2
	uint16_t length;
	uint16_t id;
	uint16_t flags_offset;	// flags: upper 3; offset: lower 13
	uint8_t ttl;
	uint8_t proto;
	uint16_t checksum;
	uint32_t src;
	uint32_t dst;
};

struct udp_header {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
};

struct icmp_header {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t rest;
};


/*
 * Compute the internet checksum, as defined by RFC 1071
 *
 * \param[in]  buff	Buffer to compute checksum of
 * \param[in]  size	Size of buffer
 *
 * \return     Checksum of buffer (in network byte order)
 */
uint16_t csum(uint16_t *buff, int size);
uint16_t build_udp(char *buffer, uint8_t *msg, uint16_t msg_len);
uint16_t build_icmp_request(char *buffer);

#endif
