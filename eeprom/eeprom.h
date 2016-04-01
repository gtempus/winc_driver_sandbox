/*
 * eeprom.h
 *
 * Created: 22.06.2015 13:48:48
 *  Author: mebskjoensfjell
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_


char EEPROM_GetChar( unsigned int addr );
void EEPROM_PutChar( unsigned int addr, char new_value );




#endif /* EEPROM_H_ */