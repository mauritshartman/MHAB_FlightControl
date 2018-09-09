/*
 * File:   1wire.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:38
 *
 * Routines for handling 1-wire devices.
 */

#include "one_wire.h"

// Configure the OW_PIN as Output and drive the OW_PIN LOW.
void drive_OW_low(void)
{
    OW_PIN_DIR = OUTPUT;
    OW_WRITE_PIN = LOW;
}


// Configure the OW_PIN as Output and drive the OW_PIN HIGH.
void drive_OW_high(void)
{
    OW_PIN_DIR = OUTPUT;
    OW_WRITE_PIN = HIGH;
}


// Configure as Input pin and Read the status of OW_PIN
ubyte read_OW(void)
{
    ubyte read_data = 0;

    OW_PIN_DIR = INPUT;
    if (OW_READ_PIN == HIGH) {
        read_data = SET;
    }
    else {
        read_data = CLEAR;
    }
    return read_data;
}


// Initialization sequence start with reset pulse. This code generates reset sequence as per the protocol
ubyte OW_reset_pulse(void)
{
    ubyte presence_detect = HIGH;   // High means no presence detected

    drive_OW_low(); 				// Drive the bus low...
    __delay_us(480);                            // ... for 480 microseconds (us)
    drive_OW_high();  				// ... and release the bus
    __delay_us(70);                             // Delay 70 microsecond (us)
    presence_detect = read_OW();	        // Sample for presence pulse (pulled low) from slave
    __delay_us(410);                            // Delay 410 microsecond (us)
    drive_OW_high();		    	        // Release the bus

    return presence_detect;
}


// This function used to transmit a single bit to slave device.
void OW_write_bit(ubyte write_bit)
{
    if (write_bit) {
        //writing a bit '1'
        drive_OW_low(); 		// Drive the bus low
        __delay_us(6);                  // delay 6 microsecond (us)
        drive_OW_high();  		// Release the bus
        __delay_us(64);                 // delay 64 microsecond (us)
    }
    else {
        //writing a bit '0'
        drive_OW_low();                 // Drive the bus low
        __delay_us(60);                 // delay 60 microsecond (us)
        drive_OW_high();  		// Release the bus
        __delay_us(10);                 // delay 10 microsecond for recovery (us)
    }
}


// This function used to read a single bit from the slave device.
ubyte OW_read_bit(void)
{
    ubyte read_data;

    //reading a bit
    drive_OW_low();                     // Drive the bus low
    __delay_us(6);			// delay 6 microsecond (us) Tinit timing
    drive_OW_high ();  			// Release the bus
    __delay_us(9);			// delay 9 microsecond (us) Trc timing

    read_data = read_OW();		//Read the status of OW_PIN

    __delay_us(55);			// delay 55 microsecond (us)
    return read_data;
}


// This function used to transmit a complete byte to slave device.
void OW_write_byte(ubyte write_data)
{
    ubyte i;

    for (i = 0; i < 8; i++) {
        OW_write_bit(write_data & 0x01); 	//Sending LS-bit first
        write_data >>= 1;					// shift the data byte for the next bit to send
    }
}


// This function used to read a complete byte from the slave device.
ubyte OW_read_byte(void)
{
    ubyte i, result = 0x0;

    // with 1-wire, LSB will arrive first
    for (i = 0; i < 8; i++) {
        result >>= 1; 				        // shift the result to get it ready for the next bit to receive
        if (OW_read_bit()) result |= 0x80;	// if result is one, then set MS-bit
    }
    return result;
}


// Check the presence of slave device.
ubyte OW_detect_slave(void)
{
    if (!OW_reset_pulse()) return HIGH;
    else return LOW;
}