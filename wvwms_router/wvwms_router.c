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
#include <time.h>
#include <errno.h>            // errno, perror()

#define WVWMS_PORT 3001
#define WVWMS_COMMAND_PORT 3000

#define MAXBUF 255
#define PACKET_CNTR 1

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP6_HDRLEN 40  // IPv6 header length
#define UDP_HDRLEN  8  // UDP header length, excludes data

enum MeasurementMode {NORMAL, FAST, SAMPLE_PACK, CUTOFF, CUTOFF_PACK, PEAK,
	PEAK_SIMPLE, TRUCK, TRUCK_SIMPLE};

struct ad7190_configuration{
	unsigned int mode_register; //= DEFAULT_MODE_REGISTER;
	unsigned int configuration_register; //= DEFAULT_CONFIGURATION_REGISTER;
	unsigned int offset_register; //= DEFAULT_OFFSET_REGISTER;
	unsigned int full_scale_register; //= DEFAULT_FULL_SCALE_REGISTER;
	unsigned int gpocon_register; //= DEFAULT_GPCON_REGISTER;
};

struct wvwms_configuration{
	unsigned int flash_key;
	struct ad7190_configuration ad7195_config;
	uint32_t sampleCntAvg;
	enum MeasurementMode meas_mode;
	uint32_t samplePackSize;
	uint32_t sampleCutOff;
	uint32_t peakMargin;
};

// Function prototypes
void process_packet(char *buffer);
int verify_crc(char *buffer);
void process_outgoing_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr);
void display_register(char *reg, size_t size);
int is_command(char *buffer, size_t length);
int is_config_upload(char *buffer, short length);
void process_incoming_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr);
int is_measurement_data(char *buffer, short length);
int is_config_download(char *buffer, short length);
int is_message(char *buffer, short length);
void print_wvwms_config(struct wvwms_configuration *config);
void print_adc_config(struct ad7190_configuration *config);
int save_config(char *buffer, unsigned char length);
void display_data(char *buffer, short length);
void display_outgoing_data(char *buffer, short length);
uint16_t
checksum (uint16_t *addr, int len);
uint16_t
udp6_checksum (struct ip6_hdr iphdr, struct udphdr udphdr, uint8_t *payload, 
		int payloadlen);
char *
allocate_strmem (int len);
uint8_t *
allocate_ustrmem (int len);


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
  printf("Wireless Vehicle Weight Measurement System v2\n");
  printf("WVWMS Packet Router v2\n");
  printf("Compiled: %s  %s\n", __DATE__, __TIME__);
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
	if(ntohs(udphdr->dest) == WVWMS_PORT){
		if(verify_crc(buffer)) {
			printf("CRC error \n");
			return;
		}
		process_incoming_data(buffer, iphdr, udphdr);
	}
	if(ntohs(udphdr->dest) == WVWMS_COMMAND_PORT){
		if(verify_crc(buffer)) {
			printf("Sending with CRC error \n");
			return;
		}
		process_outging_data(buffer, iphdr, udphdr);
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

void process_outgoing_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr)
{
	unsigned short length = ntohs(udphdr->len) - sizeof(struct udphdr);
	unsigned short offset =
			ETH_HDRLEN + sizeof (struct ip6_hdr) + sizeof(struct udphdr);
	if(is_command(buffer+offset, *(buffer+offset) ) == 0) return;
	if(is_config_upload(buffer+offset, (short) *(buffer+offset)) == 0) return;
	display_outgoing_data(buffer+offset, length);
}

void display_register(char *reg, size_t size)
{
	int i;
	printf("0x");
	for(i = 0; i<size; i++)
		printf("%02x", *(reg+i));
	printf("\n");
}

int is_command(char *buffer, size_t length)
{
		printf("%d-%d-%d %d:%d:%d Command to module: ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		switch(*(buffer+3))
		{
			case CMD_MEASUREMENT:
				printf("measurement ");
				if(*(buffer+4))
					printf("start\n");
				else
					printf("stop\n");
			break;
			case CMD_VAA_POWER:
				printf("vaa power ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_VDD_POWER:
				printf("vdd power ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_ADC_POWER:
				printf("adc power (don't trust it) ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_SET_ACX:
				printf("adc acx ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_SET_CHOP:
				printf("adc chop ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_READ_REGISTER:
				printf("read register with length %u, addr 0x%02x\n", *(buffer+5), *(buffer+4));
			break;
			case CMD_WRITE_REGISTER:
				printf("write register with length %u, addr 0x%02x ", *(buffer+5), *(buffer+4));
				display_register(buffer+6, *(buffer+5));
			break;
			case CMD_CONSOLE_PRINT:
				printf("console print\n");
			break;			
			case CMD_CALIBRATE:
				printf("calibrate mode ");
				switch (*(buffer+4))
				{
					case INTERNAL_ZERO_SCALE:
						printf("internal zero scale");
					break;
					case INTERNAL_FULL_SCALE:
						printf("internal full scale");
					break;
					case SYSTEM_ZERO_SCALE:
						printf("system zero scale");
					break;
					case SYSTEM_FULL_SCALE:
						printf("system full scale");
					break;
					case default:
						printf("uknown");
					break;
				}
				printf(" channel ");
				switch (*(buffer+5))
				{
					case 1:
						printf("0");
					break;
					case 2:
						printf("1");
					break;
					case 4:
						printf("2");
					break;
					case 8:
						printf("3");
					break;
					case 16:
						printf("4");
					break;
					case 32:
						printf("5");
					break;
					case 64:
						printf("6");
					break;
					case 128:
						printf("7");
					break;
					case default:
						printf("uknown");
					break;
				}
				printf("\n");
			break;
			case CMD_SELECT_CHANNEL:
				printf("select channel %u", (unsigned short) *(buffer+4));
			break;
			case CMD_SET_CUTOFF:
				int cutoff = *(int*)(buffer+4);
				printf("set cutoff to %u", cutoff);
			break;
			case CMD_RANGE_SETUP:
				printf("set polarity to ");
				if(*(buffer+4)) printf("unipolar ");
				else printf("bipolar ");
				printf(" and range to ");
				switch (*(buffer+5))
				{
					case G_5V:
						printf("+-5V\n");
					break;
					case G_625mV:
						printf("+-625mV\n");
					break;
					case G_321mV:
						printf("+-312.5mV\n");
					break;
					case G_156mV:
						printf("+-156.2mV\n");
					break;
					case G_78mV:
						printf("+-78.125mV\n");
					break;
					case G_39mV:
						printf("+-39.06mV\n");
					break;
					case default:
						printf("prohibited value!\n");
					break;
				}
			break;
			case CMD_ADC_POWER_MODE:
				printf("adc power mode ")
				if(*(buffer+4)==IDLE_MODE) printf("idle\n");
				else if(*(buffer+4)==POWER_DOWN_MODE) printf("power down\n");
				else printf("unknown\n");
			break;				
			case CMD_MODE:
				printf("set mode ");
				switch (*(buffer+4))
				{
					case NORMAL:
						printf("normal\n");
					break;
					case FAST:
						printf("fast\n");
					break;
					case SAMPLE_PACK:
						printf("sample pack\n");
					break;
					case CUTOFF:
						printf("cutoff\n");
					break;
					case CUTOFF_PACK:
						printf("cutoff pack\n");
					break;
					case PEAK:
						printf("peak\n");
					break;
					case PEAK_SIMPLE:
						printf("peak simple\n");
					break;
					case TRUCK:
						printf("truck\n");
					break;
					case TRUCK_SIMPLE:
						printf("truck simple\n");
					break;	
					case default:
						printf("unknown\n");
					break;
				}
			break;
			case CMD_LOAD_CONFIG_FLASH:
				printf("load config from flash\n");
			break;
			case CMD_SAVE_CONFIG_FLASH:
				printf("load config from flash\n");
			break;
			case CMD_ADC_TO_ARM:
				printf("read adc config to arm\n");
			break;
			case CMD_ARM_TO_ADC:
				printf("write adc config from arm\n");
			break;
			case CMD_GET_STATUS_REG:
				printf("get adc status register\n");
			break;			
			case CMD_SOFT_RESET_ADC:
				printf("soft reset of adc\n");
			break;
			case CMD_ADC_SINGLE:
				printf("single conversion\n");
			break;
			case CMD_CHECK_ADC:
				printf("check adc\n");
			break;
			case CMD_GET_ADC_TEMP:
				printf("get adc temperature\n");
			break;
			case CMD_SET_DEFAULT:
				printf("set default settings in arm\n");
			break;			
		}
	else return -1;
}

int is_config_upload(char *buffer, short length)
{
	if(*(buffer+3) == CMD_WRITE_CONFIG) {
		printf("%d-%d-%d %d:%d:%d Uploaded config: \n", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		print_wvwms_config((struct wvwms_configuration *)((char *)buffer+4));
		return 0;
	}
	else return -1;
}

void process_incoming_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr)
{
	unsigned short length = ntohs(udphdr->len) - sizeof(struct udphdr);
	unsigned short offset =
			ETH_HDRLEN + sizeof (struct ip6_hdr) + sizeof(struct udphdr);
	if(is_measurement_data(buffer+offset, (short) *(buffer+offset) ) == 0) return;
	if(is_config_download(buffer+offset, (short) *(buffer+offset)) == 0) return;
	if(is_message(buffer+offset, (short) *(buffer+offset)) == 0)
	display_data(buffer+offset, length);
}

int is_measurement_data(char *buffer, short length)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	unsigned int data;
	int i = 0;

	if(*(buffer+1) == CMD_MEASUREMENT && length == 5) {
		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
		printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		printf("CMD_MEASUREMENT: %16u\n", data);
		return 0;
	}
	else if(*(buffer+1) == CMD_MEAS_FAST && length == 5) {
		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
		printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		printf("CMD_MEAS_FAST: %16u\n", data);
		return 0;
	}
	else if(*(buffer+1) == CMD_MEAS_SAMPLE_PACK  && length >5) {
		i = 0;
		while(length>i+2){
			data = (*(buffer+4+i)<<16) | (*(buffer+3+i)<<8) | (*(buffer+2+i));
			printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
				tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("CMD_MEAS_SAMPLE_PACK(%u): %16u\n", i/4, data);
			i+=4;
		}
		return 0;
	}
	else if(*(buffer+1) == CMD_MEAS_CUTOFF && length == 5) {
		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
		printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		printf("CMD_MEAS_CUTOFF: %16u\n", data);
		return 0;
	}
	else if(*(buffer+1) == CMD_MEAS_CUTOFF_PACK  && length >5) {
		i = 0;
		while(length>i+2){
			data = (*(buffer+4+i)<<16) | (*(buffer+3+i)<<8) | (*(buffer+2+i));
			printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
				tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("CMD_MEAS_CUTOFF_PACK(%u): %16u\n", i/4, data);
			i+=4;
		}
		return 0;
	}
	else if(*(buffer+1) == CMD_MEAS_PEAK && length == 5) {
		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
		printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		printf("CMD_MEAS_PEAK: %16u\n", data);
		return 0;
	}
	else if(*(buffer+1) == CMD_MEAS_PEAK_SIMPLE && length == 5) {
		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
		printf("%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		printf("CMD_MEAS_PEAK_SIMPLE: %16u\n", data);
		return 0;
	}
	else return -1;
}



int is_config_download(char *buffer, short length)
{
	if(*(buffer+1) == CMD_READ_CONFIG) {
		printf("%d-%d-%d %d:%d:%d Downloaded config: \n", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		save_config(buffer+2, *buffer);
		print_wvwms_config((struct wvwms_configuration *)((char *)buffer+2));
		return 0;
	}
	else return -1;
}

int is_message(char *buffer, short length)
{
	if(*(buffer+1) == CMD_REPLY)
	{
		printf("%d-%d-%d %d:%d:%d Message from module: ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		switch(*(buffer+2))
		{
			case REPLY_OK:
				printf("ok\n");
				break;
			case ERROR_CODE_UNKNOWN_CMD:
				printf("unknown command\n");
				break;
			case ERROR_CODE_IDLE:
				printf("idle\n");
				break;	
			case ERROR_CODE_FAILED:
				printf("command failed\n");
				break;
			case ERROR_WRONG_PARAM:
				printf("wrong parameter\n");
				break;
			case ERROR_UNEXPECTD_ISR:
				printf("unexpected ISR from ADC\n");
				break;	
			case ERROR_FLASH_KEY:
				printf("error reading from flash\n");
				break;		
			case ERROR_ADC_ID:
				printf("wrong ADC ID\n");
				break;
			case default:
				printf("unrecognized\n");
				return -1;
				break;			
		}
	}
	else if(*(buffer+1) == CMD_CHECK_ADC)
	{
		printf("%d-%d-%d %d:%d:%d ADC check ID ", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		if(*(buffer+2))
		{
			printf("ok\n");
		}
		else
		{
			printf("failed\n");
		}
	}
	else return -1;
}

void print_wvwms_config(struct wvwms_configuration *config)
{
	printf("WVWMS configuration:\n");
	printf("SampleCntAvg: %u\n", (unsigned int)config->sampleCntAvg);
	printf("SamplePackSize: %u\n", (unsigned int)config->samplePackSize);
	printf("Measurement mode: %u\n", (unsigned int)config->meas_mode);
	printf("Cut off value: %u\n", (unsigned int)config->sampleCutOff);
	printf("Peak margin %u\n", (unsigned int)config->peakMargin);
	print_adc_config(&(config->ad7195_config));
}

void print_adc_config(struct ad7190_configuration *config)
{
	printf("AD7190 configuration: \n");
	printf("MODE REGISTER           0x%08x \n", config->mode_register);
	printf("CONFIGURATION REGISTER  0x%08x \n", config->configuration_register);
	printf("OFFSET REGISTER         0x%08x \n", config->offset_register);
	printf("FULL SCALE REGISTER     0x%08x \n", config->full_scale_register);
	printf("GPOCON REGISTER         0x%08x \n", config->gpocon_register);
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
	return 0;
}

void display_data(char *buffer, short length)
{
	int i = 0;
	printf("Unrecognized incoming packet, length %d: ", length);
	for(i=0;i<length;i++)
		printf("%02x|", 0xFF & *(buffer+i));
	printf("\n");
}

void display_outgoing_data(char *buffer, short length)
{
	int i = 0;
	printf("Unrecognized outgoing packet, length %d: ", length);
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
