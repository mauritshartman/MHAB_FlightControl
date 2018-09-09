/*
 * File:   i2c.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:45
 *
 * Routines for handling I2C peripherals.
 */

#ifndef I2C_H
#define	I2C_H

#ifdef	__cplusplus
extern "C" {
#endif

// Includes:
#include "defs.h"

// General I2C defines:
#define I2C_READ                0x01
#define I2C_WRITE               0x00
#define I2C_ACK                 0
#define I2C_NACK                1

// Specific I2C EEPROM defines:
#define I2C_DS3231_ADDR         0xD0
#define I2C_BMP180_ADDR         0xEE
#define I2C_EEPROM_ADDR         0xA0    // 4-bit control code for 24XX1026 devices. Chip select bits A0 and A1 set to gnd
#define I2C_24LC1026_LOW_BLK    0x00    // The B0 bit of the address, selects between the lower or higher 512K block
#define I2C_24LC1026_HIGH_BLK   0x02    // The B0 bit of the address, selects between the lower or higher 512K block
#define I2C_24LC1026_PAGE_SIZE  128     // The page size for the 24LC1026 is 128 bytes
#define I2C_24LC1026_BLOCK_SIZE 65536   // See documentation (the 128K x 8 bit array is split into two block of 65K x 8 each)

// Prototypes:
void    init_i2c(void);
void    close_i2c(void);

void    i2c_idle( void );
void    i2c_ack(void);
void    i2c_nack(void);
void    i2c_start(void);
void    i2c_restart(void);
void    i2c_stop(void);
ubyte   i2c_write(const ubyte data);
ubyte   i2c_read(ubyte *buf);

// EEPROM function prototypes:
ubyte   i2c_eeprom_byte_write   (const uint16 addr, const ubyte high, const ubyte data);
ubyte   i2c_eeprom_page_write   (const uint16 addr, const ubyte high, const ubyte *buf, uint16 len, ubyte ack_poll);
ubyte   i2c_eeprom_random_read  (const uint16 addr, const ubyte high, ubyte *buf);
ubyte   i2c_eeprom_sequence_read(const uint16 addr, const ubyte high, ubyte *buf, uint16 len);
void    i2c_eeprom_ack_polling(ubyte ctl);

#ifdef	__cplusplus
}
#endif

#endif	/* I2C_H */

