/*
 * File:   1wire.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:38
 *
 * Routines for handling 1-wire devices.
 */

#ifndef ONEWIRE_H
#define	ONEWIRE_H

#ifdef	__cplusplus
extern "C" {
#endif


// Includes:
#include "defs.h"

// General 1-Wire commands. See http://www.maxim-ic.com/ibuttonbook for further descriptions:
#define OW_READ_ROM             0x33        // Retrieve 64-bit serial code using search ROM (single device only)
#define OW_SEARCH_ROM           0xf0        // Search ROM
#define OW_MATCH_ROM            0x55
#define OW_SKIP_ROM             0xcc
#define OW_ALARM_SEARCH         0xec

// Defines for the DS18B20
#define OW_CONVERT_T            0x44
#define OW_WRITE_SCRATCHPAD     0x4e
#define OW_READ_SCRATCHPAD      0xbe
#define OW_COPY_SCRATCHPAD      0x48
#define OW_RECALL               0xb8
#define OW_POWER_SUPPLY         0xb4

// Prototypes
void    drive_OW_low(void);
void    drive_OW_high(void);
ubyte   read_OW(void);
void    OW_write_bit(ubyte write_data);
ubyte   OW_read_bit(void);
ubyte   OW_reset_pulse(void);
void    OW_write_byte(ubyte write_data);
ubyte   OW_read_byte(void);
ubyte   OW_detect_slave(void);


#ifdef	__cplusplus
}
#endif

#endif	/* 1WIRE_H */
