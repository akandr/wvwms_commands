/*  Copyright (C) 2011-2015  P.D. Buchan (pdbuchan@yahoo.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Send an IPv6 UDP packet via raw socket at the link layer (ethernet frame).
// Need to have destination MAC address.
// Includes some UDP data.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_UDP, INET6_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <netinet/ip6.h>      // struct ip6_hdr
#include <netinet/udp.h>      // struct udphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>

#include <errno.h>            // errno, perror()

#define WVWMS_PORT 3000
#define MAXBUF 255

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP6_HDRLEN 40  // IPv6 header length
#define UDP_HDRLEN  8  // UDP header length, excludes data

// Function prototypes
uint16_t checksum (uint16_t *, int);
uint16_t udp6_checksum (struct ip6_hdr, struct udphdr, uint8_t *, int);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
void process_packet(char *buffer);
void process_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr);
void display_data(char *buffer, short length);
int verify_crc(char *buffer);
int save_config(char *buffer, unsigned char length);
int is_config_download(char *buffer, short length);
int is_measurement_data(char *buffer, short length);

int ss;

int main (int argc, char **argv)
{
  int sd;
  char buffer[MAXBUF];
  // Submit request for a raw socket descriptor.
  if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("reciving socket() failed ");
    exit (EXIT_FAILURE);
  }

  while(1){
	  if (recvfrom(sd, buffer, MAXBUF, 0, NULL, NULL) > 0){
		process_packet(buffer);
  	  }
  }
  // Close socket descriptor.
  close (sd);
  return (EXIT_SUCCESS);
}

void process_packet(char *buffer)
{
	struct ip6_hdr *iphdr;
	struct udphdr *udphdr;
	iphdr = (struct ip6_hdr *) (buffer + ETH_HDRLEN);
	udphdr = (struct udphdr *) (buffer + ETH_HDRLEN + sizeof (struct ip6_hdr));
	if(ntohs(udphdr->source) == WVWMS_PORT && ntohs(udphdr->dest) == WVWMS_PORT){
		if(verify_crc(buffer)) {
			printf("CRC error \n");
			return;
		}
		process_data(buffer, iphdr, udphdr);
	}
}

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

void process_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr)
{
	unsigned short length = ntohs(udphdr->len) - sizeof(struct udphdr);
	unsigned short offset =
			ETH_HDRLEN + sizeof (struct ip6_hdr) + sizeof(struct udphdr);
	is_config_download(buffer+offset, (short) *(buffer+offset));
	if(!is_measurement_data(buffer+offset, *(buffer+offset) ));
	display_data(buffer+offset, length);
}

int is_measurement_data(char *buffer, short length)
{
	unsigned int data;
	/*
	if(*(buffer+1) == 0x0B && length == 5) {
		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*buffer+2);
		printf("M: %16u\n", data);
		return 0;
	}
	*/
	else return -1;
}



int is_config_download(char *buffer, short length)
{
	if(*(buffer+1) == 0x0C) {
		save_config(buffer+2, *buffer);
		return 0;
	}
	else return -1;
}

int save_config(char *buffer, unsigned char length)
{
	unsigned int i;
	printf("Saving file: ");
	FILE *handleWrite = fopen("wvwms_config.bin", "wb");		
	for(i=0;i<length;i++)
	{
		printf("%02x|", *(buffer+i)); 
	}
	fwrite(buffer, length, sizeof(char), handleWrite);
	fclose(handleWrite);
	printf("\n");
}

void display_data(char *buffer, short length)
{
	int i = 0;
	printf("Packet length %d: ", length);
	for(i=0;i<length;i++)
		printf("%02x|", 0xFF & *(buffer+i));
	printf("\n");
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

// Build IPv6 UDP pseudo-header and call checksum function (Section 8.1 of RFC 2460).
uint16_t
udp6_checksum (struct ip6_hdr iphdr, struct udphdr udphdr, uint8_t *payload, int payloadlen)
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
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
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
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
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
