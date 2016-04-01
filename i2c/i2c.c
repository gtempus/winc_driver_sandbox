/*
 * twi.c
 *
 * Created: 25.08.2015 15:03:24
 *  Author: mebskjoensfjell
 */ 

#include "i2c.h"

static enum status_code _i2c_master_write_packet(struct i2c_module *module, struct i2c_master_packet *packet, bool send_stop)
{
	uint8_t status;
	uint16_t len = packet->data_length;
	
	// Transmit start condition
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	// Wait
	while(!(TWCR & (1 << TWINT)));
	// Write address + write
	TWDR = (packet->address << 1) | 1;
	// Clear interrupt
	TWCR = (1 << TWINT) | (1 << TWEN);
	// Wait for ack
	while(!(TWCR & (1 << TWINT)));
	
	status = TWSR;
	if (status != 0x18) {
		return ERR_BAD_ADDRESS;
	}

	while(len--) {
		// Write data
		TWDR = *packet->data++;
		// Clear interrupt
		TWCR = (1 << TWINT) | (1 << TWEN);
		while(!(TWCR & (1 << TWINT)));
		
		status = TWSR;
		if (status != 0x28 && len) {
			if (send_stop) {
				TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
			}
			return ERR_IO_ERROR;
		}
	}
	
	// Send stop
	if (send_stop) {
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
	}
	
	return STATUS_OK;
	
	
}
enum status_code i2c_master_write_packet_wait(struct i2c_module *module, struct i2c_master_packet *packet)
{
	return _i2c_master_write_packet(module, packet, true);
}

enum status_code i2c_master_write_packet_wait_no_stop(struct i2c_module *module, struct i2c_master_packet *packet)
{
	return _i2c_master_write_packet(module, packet, false);
	
}

enum status_code i2c_master_read_packet_wait(struct i2c_module *module, struct i2c_master_packet *packet)
{
		uint8_t status;
		uint16_t len = packet->data_length;

		// Transmit start condition
		TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
		// Wait for int
		while(!(TWCR & (1 << TWINT)));
		// Write address + read	
		TWDR = (packet->address << 1) | 0;
		// Clear interrupt
		TWCR = (1 << TWINT) | (1 << TWEN);
		// Wait for ack
		while(!(TWCR & (1 << TWINT)));
		
		status = TWSR;
		if (status == 0x48) {
			return ERR_BAD_ADDRESS;
		}
		
		while(len--) {
			
			while(!(TWCR & (1 << TWINT)));
			//read data
			*(packet->data++) = TWDR;
			// Clear interrupt
			if (!len == 1) {
				TWCR = (1 << TWINT) | (1 << TWEN);				
			} else {
				TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);				
			}
		}
		
		// Send stop
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
		return STATUS_OK;
}
