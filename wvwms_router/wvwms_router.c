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

#include "mspcom.h"
#include "wvwms_display.h"
#include "wvwms_types.h"
#include "udp_network.h"
#include "ini.h"


#define MAXBUF 255
#define PACKET_CNTR 1

// Function prototypes
void process_packet(char *buffer, fDispPtr* fArr);
void process_outgoing_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr);
int is_config_upload(char *buffer, short length);
void process_incoming_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr, fDispPtr* fArr);
int is_config_download(char *buffer, short length);
int save_config(char *buffer, unsigned char length);
int read_config_file(wvwms_router_config_t * router_cfg);
static int config_handler(void* user, const char* section, const char* name,
                   const char* value);
int calculate(uint32_t sample, float * voltage, float *weight,
		wvwms_router_config_t* pconfig);
int ss;

int main (int argc, char **argv)
{
  ssize_t rval;
  int sd;
  char buffer[MAXBUF];
  wvwms_router_config_t router_cfg;
  fDispPtr fArr[256];

  // Submit request for a raw socket descriptor.
  printf("Wireless Vehicle Weight Measurement System v2\n");
  printf("WVWMS Packet Router v2\n");
  printf("Compiled: %s  %s\n", __DATE__, __TIME__);

  sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL));

  if (sd  < 0)
  {
    perror("Creating receiver socket() failed\n");
    return FAIL;
  }
  else
  {
	  printf("Creating receiver socket() ok\n");
  }

  if (read_config_file(&router_cfg))
  {
	perror("No configuration, exiting\n");
	return FAIL;
  }

  if(EXIT_SUCCESS != init_sender())
  {
	perror("Failed to create sender socket, exiting\n");
	return FAIL;
  }
  else
  {
	  printf("Sender socket initialized successfully\n");
  }

  register_data_display_functions(fArr);

  while(1)
  {
	  rval = recvfrom(sd, buffer, MAXBUF, 0, NULL, NULL);
	  if (rval > 0)
	  {
		process_packet(buffer, fArr);
  	  }
	  if(rval < 0)
	  {
		perror("recvfrom() returned error\n");
	  }
  }
  perror("Exiting program for unknown reason\n");
  close (sd);
  return (EXIT_SUCCESS);
}

void process_packet(char *buffer, fDispPtr* fArr)
{
	struct ip6_hdr *iphdr;
	struct udphdr *udphdr;
	iphdr = (struct ip6_hdr *) (buffer + ETH_HDRLEN);
	udphdr = (struct udphdr *) (buffer + ETH_HDRLEN + sizeof (struct ip6_hdr));
	if(ntohs(udphdr->dest) == WVWMS_PORT){
		if(verify_crc(buffer)) {
			printf("Receiving with CRC error \n");
			return;
		}
		process_incoming_data(buffer, iphdr, udphdr, fArr);
	}
	if(ntohs(udphdr->dest) == WVWMS_COMMAND_PORT){
		if(verify_crc(buffer)) {
			printf("Sending with CRC error \n");
			return;
		}
		process_outgoing_data(buffer, iphdr, udphdr);
	}
}

void process_outgoing_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr)
{
	unsigned short length = ntohs(udphdr->len) - sizeof(struct udphdr);
	unsigned short offset =
			ETH_HDRLEN + sizeof (struct ip6_hdr) + sizeof(struct udphdr);
	if(!display_outgoing_command(buffer+offset, *(buffer+offset))) return;
	if(!is_config_upload(buffer+offset, (short) *(buffer+offset))) return;
	display_outgoing_raw_data(buffer+offset, length);
}

int is_config_upload(char *buffer, short length)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	if(*(buffer+3) == CMD_WRITE_CONFIG) {
		printf("%d-%02d-%02d %02d:%02d:%02d Uploaded config: \n", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		display_wvwms_config((struct wvwms_configuration *)((char *)buffer+4));
		return OK;
	}
	else return FAIL;
}

void process_incoming_data(char *buffer, struct ip6_hdr *iphdr, struct udphdr *udphdr, fDispPtr* fArr)
{
	unsigned short length = ntohs(udphdr->len) - sizeof(struct udphdr);
	unsigned short offset =
			ETH_HDRLEN + sizeof (struct ip6_hdr) + sizeof(struct udphdr);
	if(!process_mesaurement_data(buffer+offset, (short) *(buffer+offset), fArr)) return;
	if(!is_config_download(buffer+offset, (short) *(buffer+offset))) return;
	if(!display_incoming_messages(buffer+offset, (short) *(buffer+offset)))  return;
	display_incomming_raw_data(buffer+offset, length);
}

int is_config_download(char *buffer, short length)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	if(*(buffer+1) == CMD_READ_CONFIG) {
		printf("%d-%02d-%02d %02d:%02d:%02d Downloaded config: \n", tm.tm_year + 1900, 
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		save_config(buffer+2, *buffer);
		display_wvwms_config((struct wvwms_configuration *)((char *)buffer+2));
		return OK;
	}
	else return FAIL;
}

/**
 * Saves configuration registers of WVWMS node
 * @param buffer
 * @param length
 * @return
 */
int save_config(char *buffer, unsigned char length)
{
	unsigned int i;
	int rval;

	printf("Received binary: ");
	FILE *handleWrite = fopen(NODE_CONFIGURATION_FILE, "wb");
	for(i=0;i<length;i++)
	{
		printf("%02x|", *(buffer+i)); 
	}
	printf("\nSaving file ");
	rval = fwrite(buffer, sizeof(char), length, handleWrite);
	if (length != rval)
	{
		printf("failed! fwrite returned %i, expected %i \n", rval, length);
		perror("Error: ");
	}
	else
	{
		printf("ok!\n");
	}

	fclose(handleWrite);
	printf("\n");
	return OK;
}

/**
 * Loads program configuration from file
 * @param router_cfg - pointer to configuration structure
 * @return OK if succeed
 */
int read_config_file(wvwms_router_config_t * router_cfg)
{
	if(ini_parse(CONFIGURATION_FILE, config_handler, router_cfg) < 0)
	{
		perror("Failed to load configuration from " CONFIGURATION_FILE "\n");
		return FAIL;
	}
	else
	{
		printf("Loaded configuration from " CONFIGURATION_FILE ":\n");
		display_router_cfg(router_cfg);
		return OK;
	}
}

/**
 * This is callback function for INI library
 * @param user
 * @param section
 * @param name
 * @param value
 * @return
 */
static int config_handler(void* user, const char* section, const char* name,
                   const char* value)
{
	wvwms_router_config_t* pconfig = (wvwms_router_config_t*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("wvwms_config", "zero_level")) {
        pconfig->zero_level = atoi(value);
    } else if (MATCH("wvwms_config", "voltage_mul")) {
        pconfig->voltage_mul = atoi(value);
    } else if (MATCH("wvwms_config", "voltage_div")) {
        pconfig->voltage_div = atoi(value);
    } else if (MATCH("wvwms_config", "weight_mul")) {
        pconfig->weight_mul = atoi(value);
    } else if (MATCH("wvwms_config", "weight_div")) {
        pconfig->weight_div = atoi(value);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

/**
 * Calculates
 * @param sample
 * @param voltage
 * @param weight
 * @param pconfig
 * @return
 */
int calculate(uint32_t sample, float *voltage, float *weight,
		wvwms_router_config_t* pconfig)
{
	if (pconfig != NULL)
	{
		*voltage = pconfig->voltage_mul * sample;
		*voltage = *voltage/(pconfig->voltage_div);
		*weight = pconfig->weight_mul * sample;
		*weight = *weight/(pconfig->weight_div);
		return OK;
	}
	return FAIL;
}







