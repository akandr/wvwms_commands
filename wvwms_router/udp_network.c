/*
 * This is for UDPv6 raw socket network hadnler
 *
 *  Created on: 19 lis 2015
 *      Author: Artur Andrzejczak
 */

#include "udp_network.h"

static int sock;

int verify_crc(char *buffer)
{
	struct ip6_hdr *iphdr = (struct ip6_hdr *) (buffer + ETH_HDRLEN);
	struct udphdr *udphdr =
			(struct udphdr *) (buffer + ETH_HDRLEN + sizeof (struct ip6_hdr));
	unsigned short length = ntohs(udphdr->len) - sizeof(struct udphdr);
	unsigned short offset =
			ETH_HDRLEN + sizeof (struct ip6_hdr) + sizeof(struct udphdr);

	if (ntohs(udphdr->check) !=
			ntohs(udp6_checksum(*iphdr, *udphdr,
					(uint8_t *)(buffer+offset), length))){
			return -1;
	}
	return 0;
}

// Build IPv6 UDP pseudo-header and call checksum function (Section 8.1 of RFC 2460).
uint16_t
udp6_checksum (struct ip6_hdr iphdr, struct udphdr udphdr, uint8_t *payload,
		int payloadlen)
{
  char buf[IP_MAXPACKET];
  char *ptr;
  int chksumlen = 0;
  int i;

  ptr = &buf[0];  // ptr points to beginning of buffer buf

  // Copy source IP address into buf (128 bits)
  memcpy (ptr, &iphdr.ip6_src.s6_addr, sizeof (iphdr.ip6_src.s6_addr));
  ptr += sizeof (iphdr.ip6_src.s6_addr);
  chksumlen += sizeof (iphdr.ip6_src.s6_addr);

  // Copy destination IP address into buf (128 bits)
  memcpy (ptr, &iphdr.ip6_dst.s6_addr, sizeof (iphdr.ip6_dst.s6_addr));
  ptr += sizeof (iphdr.ip6_dst.s6_addr);
  chksumlen += sizeof (iphdr.ip6_dst.s6_addr);

  // Copy UDP length into buf (32 bits)
  memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
  ptr += sizeof (udphdr.len);
  chksumlen += sizeof (udphdr.len);

  // Copy zero field to buf (24 bits)
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 3;

  // Copy next header field to buf (8 bits)
  memcpy (ptr, &iphdr.ip6_nxt, sizeof (iphdr.ip6_nxt));
  ptr += sizeof (iphdr.ip6_nxt);
  chksumlen += sizeof (iphdr.ip6_nxt);

  // Copy UDP source port to buf (16 bits)
  memcpy (ptr, &udphdr.source, sizeof (udphdr.source));
  ptr += sizeof (udphdr.source);
  chksumlen += sizeof (udphdr.source);

  // Copy UDP destination port to buf (16 bits)
  memcpy (ptr, &udphdr.dest, sizeof (udphdr.dest));
  ptr += sizeof (udphdr.dest);
  chksumlen += sizeof (udphdr.dest);

  // Copy UDP length again to buf (16 bits)
  memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
  ptr += sizeof (udphdr.len);
  chksumlen += sizeof (udphdr.len);

  // Copy UDP checksum to buf (16 bits)
  // Zero, since we don't know it yet
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 2;

  // Copy payload to buf
  memcpy (ptr, payload, payloadlen * sizeof (uint8_t));
  ptr += payloadlen;
  chksumlen += payloadlen;

  // Pad to the next 16-bit boundary
  for (i=0; i<payloadlen%2; i++, ptr++) {
    *ptr = 0;
    ptr++;
    chksumlen++;
  }

  return checksum ((uint16_t *) buf, chksumlen);
}

// Allocate memory for an array of chars.
char *
allocate_strmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr,
	"ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of unsigned chars.
uint8_t *
allocate_ustrmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr,
	"ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum does not preclude collisions.
uint16_t
checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

int init_sender(void)
{
	int status, sinlen;
	int yes = 1;
	struct sockaddr_in sock_in;

	sinlen = sizeof(struct sockaddr_in);
	memset(&sock_in, 0, sinlen);

	sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sock_in.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_in.sin_port = htons(0);
	sock_in.sin_family = PF_INET;

	status = bind(sock, (struct sockaddr *)&sock_in, sinlen);
	if(!status)
	{
		fprintf(stderr,"Bind failed\n");
		return EXIT_FAILURE;
	}
	status = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(int) );
	if(!status)
	{
		fprintf(stderr,"Setsockopt failed\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int udp_send(void *buf, size_t buflen)
{
	int status, sinlen;
	struct sockaddr_in sock_in;

	/* -1 = 255.255.255.255 this is a BROADCAST address,
	 a local broadcast address could also be used.
	 you can comput the local broadcat using NIC address and its NETMASK
	*/
	sinlen = sizeof(struct sockaddr_in);
	sock_in.sin_addr.s_addr=htonl(-1); /* send message to 255.255.255.255 */
	sock_in.sin_port = htons(UDP_SENDER_PORT); /* port number */
	sock_in.sin_family = PF_INET;

	status = sendto(sock, buf, buflen, 0, (struct sockaddr *)&sock_in, sinlen);
	if(!status)
	{
		fprintf (stderr,"Sendto failed\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int send_measurement_frame(upload_frame_t * frame)
{
	return udp_send(frame, sizeof(upload_frame_t));
}

int send_weight(uint32_t weight, uint32_t voltage)
{
	upload_frame_t frame;
	struct timespec time;

	clock_gettime(CLOCK_REALTIME, &time);
	frame.node_id = 0;
	frame.status = 0;
	frame.time = time;
	frame.weight = weight;
	frame.voltage = voltage;
	frame.battery_voltage = 300;

	return send_measurement_frame(&frame);
}

