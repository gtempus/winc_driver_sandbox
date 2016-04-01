/*
 * twi.h
 *
 * Created: 25.08.2015 15:03:17
 *  Author: mebskjoensfjell
 */ 


#ifndef TWI_H_
#define TWI_H_

#include "main.h"

struct i2c_master_packet {
	uint16_t address;
	uint8_t data_length;
	uint8_t *data;
	uint8_t ten_bit_address;
	uint8_t high_speed;
	uint8_t hs_master_code;
};

struct i2c_module {
	uint16_t address;
};

void i2c_master_enable();
void i2c_master_init();
void i2c_write();
enum status_code i2c_master_write_packet_wait(struct i2c_module *module, struct i2c_master_packet *packet);
enum status_code i2c_master_write_packet_wait_no_stop(struct i2c_module *module, struct i2c_master_packet *packet);
enum status_code i2c_master_read_packet_wait(struct i2c_module *module, struct i2c_master_packet *packet);


#endif /* TWI_H_ */