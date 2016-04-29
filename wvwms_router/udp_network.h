/*
 * This is for UDPv6 raw socket network hadnler
 *
 *  Created on: 19 lis 2015
 *      Author: Artur Andrzejczak
 */

#ifndef UDP_NETWORK_H_
#define UDP_NETWORK_H_

#include "wvwms_types.h"

#define UDP_SENDER_PORT 6660

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP6_HDRLEN 40  // IPv6 header length
#define UDP_HDRLEN  8  // UDP header length, excludes data

uint16_t checksum (uint16_t *addr, int len);
uint16_t
udp6_checksum (struct ip6_hdr iphdr, struct udphdr udphdr, uint8_t *payload,
		int payloadlen);
char * allocate_strmem (int len);
uint8_t * allocate_ustrmem (int len);
int verify_crc(char *buffer);
int send_weight(uint32_t weight, uint32_t voltage);

#endif /* UDP_NETWORK_H_ */
