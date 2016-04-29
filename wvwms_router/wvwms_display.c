/*
 * This module provides functions responsible for displaying wvwms data in console.
 *
 *  Created on: 19 lis 2015
 *      Author: Artur Andrzejczak
 */

#include "mspcom.h"
#include "wvwms_display.h"
#include <math.h>

int unknown_function(uint8_t *buffer, short length);
int handle_cmd_measurement(uint8_t *buffer, short length);
int handle_cmd_measurement_fast(uint8_t *buffer, short length);
int handle_cmd_measurement_sample_pack(uint8_t *buffer, short length);
int handle_cmd_measurement_cutoff(uint8_t *buffer, short length);
int handle_cmd_measurement_cutoff_pack(uint8_t *buffer, short length);
int handle_cmd_measurement_peak(uint8_t *buffer, short length);
int handle_cmd_measurement_peak_simple(uint8_t *buffer, short length);
int handle_cmd_measurement_cutoff_pack(uint8_t *buffer, short length);
int handle_cmd_measurement_buffer(uint8_t *buffer, short length);
int log_buffer_to_file(uint8_t *buffer, short length);

void display_register(char *reg, size_t size)
{
	int i;
	printf("0x");
	for(i = 0; i<size; i++)
		printf("%02x", *(reg+i));
	printf("\n");
}

int display_outgoing_command(char *buffer, size_t length)
{
	int cutoff;
		switch(*(buffer+3))
		{
			case CMD_MEASUREMENT:
				display_time();
				printf("measurement ");
				if(*(buffer+4))
					printf("start\n");
				else
					printf("stop\n");
			break;
			case CMD_VAA_POWER:
				display_time();
				printf("vaa power ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_VDD_POWER:
				display_time();
				printf("vdd power ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_ADC_POWER:
				display_time();
				printf("adc power (don't trust it) ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_SET_ACX:
				display_time();
				printf("adc acx ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_SET_CHOP:
				display_time();
				printf("adc chop ");
				if(*(buffer+4))
					printf("on\n");
				else
					printf("off\n");
			break;
			case CMD_READ_REGISTER:
				display_time();
				printf("read register with length %u, addr 0x%02x\n",
						*(buffer+5), *(buffer+4));
			break;
			case CMD_WRITE_REGISTER:
				display_time();
				printf("write register with length %u, addr 0x%02x ",
						*(buffer+5), *(buffer+4));
				display_register(buffer+6, *(buffer+5));
			break;
			case CMD_CONSOLE_PRINT:
				display_time();
				printf("console print\n");
			break;
			case CMD_CALIBRATE:
				display_time();
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
					default:
						printf("uknown");
					break;
				}
				printf(" channel ");
				switch (*(buffer+5))
				{
					case 0:
						printf("0");
					break;
					case 1:
						printf("1");
					break;
					case 2:
						printf("2");
					break;
					case 3:
						printf("3");
					break;
					case 4:
						printf("4");
					break;
					case 5:
						printf("5");
					break;
					case 6:
						printf("6");
					break;
					case 7:
						printf("7");
					break;
					default:
						printf("uknown %u", *(buffer+5));
					break;
				}
				printf("\n");
			break;
			case CMD_SELECT_CHANNEL:
				display_time();
				printf("select channel %u\n", (unsigned short) *(buffer+4));
			break;
			case CMD_SET_CUTOFF:
				display_time();
				cutoff = *(int*)(buffer+4);
				printf("set cutoff to %u\n", cutoff);
			break;
			case CMD_SET_BUFFER_SIZE:
				display_time();
				cutoff = *(int*)(buffer+4);
				printf("set buffer size to %u\n", cutoff);
			break;
			case CMD_SET_PEAK_SAMPLE:
				display_time();
				cutoff = *(int*)(buffer+4);
				printf("set peak samples to %u\n", cutoff);
			break;
			case CMD_RANGE_SETUP:
				display_time();
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
					case G_312mV:
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
					default:
						printf("prohibited value!\n");
					break;
				}
			break;
			case CMD_ADC_POWER_MODE:
				display_time();
				printf("adc power mode ");
				if(*(buffer+4)==IDLE_MODE){
					printf("idle\n");
				}
				else if(*(buffer+4)==POWER_DOWN_MODE){
					printf("power down\n");
				}
				else{
					printf("unknown\n");
				}
			break;
			case CMD_MODE:
				display_time();
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
					case BUFFER:
						printf("buffer measurement\n");
					break;
					default:
						printf("unknown\n");
					break;
				}
			break;
			case CMD_LOAD_CONFIG_FLASH:
				display_time();
				printf("load config from flash\n");
			break;
			case CMD_SAVE_CONFIG_FLASH:
				display_time();
				printf("save config to flash\n");
			break;
			case CMD_ADC_TO_ARM:
				display_time();
				printf("read adc config to arm\n");
			break;
			case CMD_ARM_TO_ADC:
				display_time();
				printf("write adc config from arm\n");
			break;
			case CMD_GET_STATUS_REG:
				display_time();
				printf("get adc status register\n");
			break;
			case CMD_SOFT_RESET_ADC:
				display_time();
				printf("soft reset of adc\n");
			break;
			case CMD_ADC_SINGLE:
				display_time();
				printf("single conversion\n");
			break;
			case CMD_CHECK_ADC:
				display_time();
				printf("check adc\n");
			break;
			case CMD_GET_ADC_TEMP:
				display_time();
				printf("get adc temperature\n");
			break;
			case CMD_SET_DEFAULT:
				display_time();
				printf("set default settings in arm\n");
			break;
			case CMD_READ_CONFIG:
				display_time();
				printf("download config\n");
			break;
			case CMD_WRITE_CONFIG:
				display_time();
				printf("upload config:\n");
				display_wvwms_config((struct wvwms_configuration *)
						((char *)buffer+4));
				break;
			default:
				return -1;
			break;
		}
	return 0;
}

void display_raw_data(char *buffer, short length)
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

int display_incoming_messages(char *buffer, short length)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	if(*(buffer+1) == CMD_REPLY)
	{
		printf("%d-%02d-%02d %02d:%02d:%02d Message from module: ",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
				tm.tm_min, tm.tm_sec);
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
			case PEAK_ACTIVE:
				printf("peak detection active\n");
				break;
			default:
				printf("unrecognized\n");
				return -1;
				break;
		}
	}
	else if(*(buffer+1) == CMD_CHECK_ADC)
	{
		printf("%d-%02d-%02d %02d:%02d:%02d ADC check ID ", tm.tm_year + 1900,
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
	else if(*(buffer+1) == CMD_READ_REGISTER)
	{
		printf("%d-%02d-%02d %02d:%02d:%02d Read register ", tm.tm_year + 1900,
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		printf("addr %02x ",*(buffer+2));
		display_register((buffer+3), *(buffer)-2);
	}
	else return -1;
	return 0;
}

void display_wvwms_config(struct wvwms_configuration *config)
{
	printf("WVWMS configuration:\n");
	printf("SampleCntAvg: %u\n", (unsigned int)config->sampleCntAvg);
	printf("SamplePackSize: %u\n", (unsigned int)config->samplePackSize);
	printf("Measurement mode: %u\n", (unsigned int)config->meas_mode);
	printf("Cut off value: %u\n", (unsigned int)config->sampleCutOff);
	printf("Peak margin %u\n", (unsigned int)config->peakMargin);
	printf("Peak samples %u\n\r", (unsigned int)config->peakSamples);
	display_adc_config(&(config->ad7195_config));
}

void display_adc_config(struct ad7190_configuration *config)
{
	printf("AD7190 configuration: \n");
	printf("MODE REGISTER           0x%08x \n", config->mode_register);
	printf("CONFIGURATION REGISTER  0x%08x \n", config->configuration_register);
	printf("OFFSET REGISTER         0x%08x \n", config->offset_register);
	printf("FULL SCALE REGISTER     0x%08x \n", config->full_scale_register);
	printf("GPOCON REGISTER         0x%08x \n", config->gpocon_register);
}

int unknown_function(uint8_t *buffer, short length)
{
	printf("Unrecognized data type to be displayed!\n");
	return 0;
}

void register_data_display_functions(fDispPtr* fArr)
{
	uint8_t i;
	for(i=0; i < 255; i++)
	{
		fArr[i] = (fDispPtr)&unknown_function;
	}
	fArr[CMD_MEASUREMENT] = (fDispPtr)&handle_cmd_measurement;
	fArr[CMD_MEAS_FAST] = (fDispPtr)&handle_cmd_measurement_fast;
	fArr[CMD_MEAS_SAMPLE_PACK] = (fDispPtr)&handle_cmd_measurement_sample_pack;
	fArr[CMD_MEAS_CUTOFF] = (fDispPtr)&handle_cmd_measurement_cutoff;
	fArr[CMD_MEAS_CUTOFF_PACK] = (fDispPtr)&handle_cmd_measurement_cutoff_pack;
	fArr[CMD_MEAS_PEAK] = (fDispPtr)&handle_cmd_measurement_peak;
	fArr[CMD_MEAS_PEAK_SIMPLE] = (fDispPtr)&handle_cmd_measurement_peak_simple;
	fArr[CMD_MEASURE_BUFFER] = (fDispPtr)&handle_cmd_measurement;
}

int handle_cmd_measurement(uint8_t *buffer, short length)
{
	unsigned int data;

	if(length != 5) return FAIL;
	display_time();
	data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
	printf("CMD_MEASUREMENT: %16u\n", data);
	return OK;
}

int handle_cmd_measurement_fast(uint8_t *buffer, short length)
{
	unsigned int data;

	if(length != 5) return FAIL;
	display_time();
	data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
	printf("CMD_MEAS_FAST: %16u\n", data);
	return OK;
}

int handle_cmd_measurement_sample_pack(uint8_t *buffer, short length)
{
	unsigned int data;

	uint8_t i;
	if(length > 5)
	{
		i = 0;
		while(length>i+2){
			display_time();
			data = (*(buffer+4+i)<<16) | (*(buffer+3+i)<<8) | (*(buffer+2+i));
			printf("CMD_MEAS_SAMPLE_PACK(%u): %16u\n", i/4, data);
			i+=4;
		}
		return OK;
	}
	else return FAIL;
}

int handle_cmd_measurement_cutoff(uint8_t *buffer, short length)
{
	unsigned int data;

	if(length != 5) return FAIL;
	display_time();
	data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
	printf("CMD_MEAS_CUTOFF: %16u\n", data);

	return OK;
}

int handle_cmd_measurement_cutoff_pack(uint8_t *buffer, short length)
{
	unsigned int data;
	uint8_t i;

	if(length > 5)
	{
		i = 0;
		while(length>i+2){
			display_time();
			data = (*(buffer+4+i)<<16) | (*(buffer+3+i)<<8) | (*(buffer+2+i));
			printf("CMD_MEAS_CUTOFF_PACK(%u): %16u\n", i/4, data);
			i+=4;
		}
		return OK;
	}
	else return FAIL;
}

int handle_cmd_measurement_peak(uint8_t *buffer, short length)
{
	unsigned int data;
	if(length != 5) return FAIL;
	display_time();
	data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
	printf("CMD_MEAS_PEAK: %16u\n", data);
	return OK;
}

int handle_cmd_measurement_peak_simple(uint8_t *buffer, short length)
{
	unsigned int data;

	if(length != 5) return FAIL;
	display_time();
	data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
	printf("CMD_MEAS_PEAK_SIMPLE: %16u\n", data);
	return OK;
}

int handle_cmd_measurement_buffer(uint8_t *buffer, short length)
{
	uint8_t i;
	unsigned int data;
	unsigned int sampleId;

	if(length >5)
	{
		i = 0;
		sampleId = (*(buffer+5)<<24) | (*(buffer+4)<<16) | (*(buffer+3)<<8)
						| (*(buffer+2));
		while(length>i+2){
			display_time();
			data = (*(buffer+8+i)<<16) | (*(buffer+7+i)<<8) | (*(buffer+6+i));
			printf("BUFFER %u %16u\n", sampleId++, data);
			i+=8;
		}
		return log_buffer_to_file(buffer, length);
	}
	else return FAIL;
}

int log_buffer_to_file(uint8_t *buffer, short length)
{
	unsigned int data;
	unsigned int sampleId;
	int i = 0;
	FILE *fp;
    struct timespec spec;
    time_t t;

	fp = fopen(BUFFERED_MEASUREMENT_FILE, "a");
	if(NULL == fp)
	{
		printf("Failed to append the buffer measurement file "
				BUFFERED_MEASUREMENT_FILE);
		return FAIL;
	}

	if(length >5)
	{
		i = 0;
		sampleId = (*(buffer+5)<<24) | (*(buffer+4)<<16) | (*(buffer+3)<<8)
						| (*(buffer+2));
		while(length>i+2){
			clock_gettime(CLOCK_REALTIME, &spec);
			t = spec.tv_sec;
			struct tm tm = *localtime(&t);
			fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d:%03d", tm.tm_year + 1900,
					tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
					(int)lround(spec.tv_nsec / 1.0e6));
			data = (*(buffer+8+i)<<16) | (*(buffer+7+i)<<8) | (*(buffer+6+i));
			fprintf(fp, "%u %16u\n", sampleId++, data);
			i+=8;
		}
		fclose(fp);
		return OK;
	}
	else
	{
		fclose(fp);
		return FAIL;
	}
}

int process_mesaurement_data(char *buffer, short length, fDispPtr* fArr)
{
	return fArr[(uint8_t)*buffer](buffer, length);
}

//	unsigned int data;
//	unsigned int sampleId;
//	int i = 0;
//
//	if(*(buffer+1) == CMD_MEASUREMENT && length == 5) {
//		display_time();
//		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
//		printf("CMD_MEASUREMENT: %16u\n", data);
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEAS_FAST && length == 5) {
//		display_time();
//		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
//		printf("CMD_MEAS_FAST: %16u\n", data);
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEAS_SAMPLE_PACK  && length >5) {
//		i = 0;
//		while(length>i+2){
//			display_time();
//			data = (*(buffer+4+i)<<16) | (*(buffer+3+i)<<8) | (*(buffer+2+i));
//			printf("CMD_MEAS_SAMPLE_PACK(%u): %16u\n", i/4, data);
//			i+=4;
//		}
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEAS_CUTOFF && length == 5) {
//		display_time();
//		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
//		printf("CMD_MEAS_CUTOFF: %16u\n", data);
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEAS_CUTOFF_PACK  && length >5) {
//		i = 0;
//		while(length>i+2){
//			display_time();
//			data = (*(buffer+4+i)<<16) | (*(buffer+3+i)<<8) | (*(buffer+2+i));
//			printf("CMD_MEAS_CUTOFF_PACK(%u): %16u\n", i/4, data);
//			i+=4;
//		}
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEAS_PEAK && length == 5) {
//		display_time();
//		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
//		printf("CMD_MEAS_PEAK: %16u\n", data);
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEAS_PEAK_SIMPLE && length == 5) {
//		display_time();
//		data = (*(buffer+4)<<16) | (*(buffer+3)<<8) | (*(buffer+2));
//		printf("CMD_MEAS_PEAK_SIMPLE: %16u\n", data);
//		return 0;
//	}
//	else if(*(buffer+1) == CMD_MEASURE_BUFFER  && length >5) {
//		i = 0;
//		sampleId = (*(buffer+5)<<24) | (*(buffer+4)<<16) | (*(buffer+3)<<8)
//						| (*(buffer+2));
//		while(length>i+2){
//			display_time();
//			data = (*(buffer+8+i)<<16) | (*(buffer+7+i)<<8) | (*(buffer+6+i));
//			printf("BUFFER %u %16u\n", sampleId++, data);
//			i+=8;
//		}
//		return 0;
//	}
//	else return -1;


void display_values(uint32_t sample, float voltage, float weight)
{

	printf("%u\t%fmV/V\t%fkg\n",sample, voltage, weight);
}

void display_time(void)
{
    struct timespec spec;
    time_t t;
    clock_gettime(CLOCK_REALTIME, &spec);
	t = spec.tv_sec;
	struct tm tm = *localtime(&t);
	printf("%d-%02d-%02d %02d:%02d:%02d:%03d", tm.tm_year + 1900,
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
			(int)lround(spec.tv_nsec / 1.0e6));
}

void display_router_cfg(wvwms_router_config_t * cfg)
{
	printf("Zero level: %u\n", cfg->zero_level);
	printf("Voltage multiplier: %u \n", cfg->voltage_mul);
	printf("Voltage divider: %u \n", cfg->voltage_div);
	printf("Weight multiplier: %u \n", cfg->weight_mul);
	printf("Weight divider: %u \n", cfg->weight_div);
}



