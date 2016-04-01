/*
 * spi.c
 *
 * Created: 17.03.2015 12:59:48
 *  Author: mebskjoensfjell
 */ 

#include "spi.h"

/** \brief Write packet
 *
 * \param spi       unused
 
 * \param data      buffer to write
 * \param length    length of buffer
 */
status_code_t spi_write_packet(void *spi, uint8_t *data, uint8_t length)
{
	while (length--) {
		SPDR = *data++;
		while(!(SPSR & (1 << SPIF)));
	}
	return STATUS_OK;	
}

/** \brief Read one byte
 *
 * \param spi       unused
 * \param data      pointer to where to store read data
 */
status_code_t spi_read_single(void *spi, uint8_t *data)
{
	*data = SPDR;
	return STATUS_OK;
}
