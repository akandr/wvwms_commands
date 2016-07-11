/*
 * This module provides functions responsible for displaying wvwms data in console.
 *
 *  Created on: 19 lis 2015
 *      Author: Artur Andrzejczak
 */

#ifndef WVWMS_DISPLAY_H_
#define WVWMS_DISPLAY_H_

#include "wvwms_types.h"

typedef int (*fDispPtr)(char *, short);

void display_register(char *reg, size_t size);
void display_register_reversed(char *reg, size_t size);
int display_outgoing_command(char *buffer, size_t length);
int display_incoming_messages(char *buffer, short length);
void display_wvwms_config(struct wvwms_configuration *config);
void display_adc_config(struct ad7190_configuration *config);
void display_incomming_raw_data(char *buffer, short length);
void display_outgoing_raw_data(char *buffer, short length);
void display_time(void);
void display_router_cfg(wvwms_router_config_t * cfg);
void display_values(uint32_t sample, float voltage, float weight);
int process_mesaurement_data(char *buffer, short length, fDispPtr* fArr);
void register_data_display_functions(fDispPtr* fArr);


#endif /* WVWMS_DISPLAY_H_ */
