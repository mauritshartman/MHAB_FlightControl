/*
 * File:   serial.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:21
 *
 * Routines for selecting the right serial (RX/TX) channel.
 */

#ifndef SERIAL_H
#define	SERIAL_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "defs.h"

// Global variables:
extern volatile ubyte global_uart_new_data;

// Defines:
#define SELECT_GPS      0b00    // Selected by setting the COM SEL0 and COM SEL1 lines
#define SELECT_GSM      0b01
#define SELECT_APRS     0b10    // Not used anymore
#define SELECT_PC       0b11    // This is the default as both COM SEL lines have pull-ups

// See PIC18F4550 manual (page 250), formula is Fosc / (4 * (sbrg + 1)) for HS mode
// Settings for BRGH = 1 and BRG16 = 1 are used.
#define BAUD300         16665
#define BAUD1200        4165
#define BAUD2400        2082
#define BAUD4800        1041
#define BAUD9600        520
#define BAUD19200       259
#define BAUD57600       86
#define BAUD115200      42

// Specific defines:
#define enable_serial()  COM_ENABLE_PIN = LOW;
#define disable_serial() COM_ENABLE_PIN = HIGH;
#define data_rdy_uart() (global_uart_new_data == (ubyte)SET)

// Function prototypes:
ubyte   init_serial(void);
void    serial_channel(ubyte channel);
ubyte   getc_uart(void);
void    putch(ubyte c);


#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_H */
