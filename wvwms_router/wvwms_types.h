/*
 * Definitions of WVWMS types
 *
 *  Created on: 19 lis 2015
 *      Author: Artur Andrzejczak
 */

#ifndef WVWMS_TYPES_H_
#define WVWMS_TYPES_H_

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

#define CONFIGURATION_FILE "wvwms_router.config"
#define BUFFERED_MEASUREMENT_FILE "buffer_measurement.txt"
#define NODE_CONFIGURATION_FILE "wvwms_config.bin"

#define OK 0
#define FAIL -1

enum WvwmsState {IDLE, MEASUREMENT};
enum MeasurementMode {NORMAL, FAST, SAMPLE_PACK, CUTOFF, CUTOFF_PACK,
					 PEAK, PEAK_SIMPLE, TRUCK, TRUCK_SIMPLE, BUFFER, CUTOFF_BUFFER};

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
	uint32_t peakSamples;
	uint32_t samplesBufferSize;
};

typedef struct wvwms_router_config{
	uint32_t zero_level;
	uint32_t voltage_mul;
	uint32_t voltage_div;
	uint32_t weight_mul;
	uint32_t weight_div;
} wvwms_router_config_t;

typedef struct  __attribute__((packed)){
	uint32_t node_id;
	struct timespec time;
	uint32_t raw;
	uint32_t weight;
	uint32_t voltage;
	uint32_t status;
	uint32_t battery_voltage;
} upload_frame_t ;


#endif /* WVWMS_TYPES_H_ */
