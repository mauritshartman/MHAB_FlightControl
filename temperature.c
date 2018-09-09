/*
 * File:   temperature.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:36
 *
 * Routines for handling temperature measurements.
 */

#include "temperature.h"
#include "digital_pressure.h"
#include "one_wire.h"

#include "util.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>


ubyte init_temperature(void)
{
    // 1. Check internal temperature sensor in the BMP180:
    if (read_bmp180_temperature() != SHRT_MAX) { printf("INT_TEMP "); }    // Max value indicates error
    else {
        printf("\r\nError initializing internal thermometer\r\n");
        return FALSE;
    }

    // 2. Check external temperature sensor (DS18B20):
    if (!OW_reset_pulse()) { printf("EXT_TEMP "); } // Low (zero) means thermometer is present
    else {
        printf("\r\nError initializing external thermometer\r\n");
        return FALSE;
    }

    return TRUE;
}


sint16  get_internal_temp(void)
{
    // In effect an alias for the same function of the RTC:
    return read_bmp180_temperature();
}


sint16  get_external_temp(void)
{
    sint16 result = 0xff;
    ubyte msb, lsb;

    OW_reset_pulse();
    OW_write_byte(OW_SKIP_ROM);
    OW_write_byte(OW_CONVERT_T);                    // Issue temperature conversion command
    drive_OW_high();                                // Apply strong pull-up during conversion
    delay_1sec();                                   // Delay for one second to wait for temperature conversion

    // Retrieve the temperature results:
    OW_reset_pulse();
    OW_write_byte(OW_SKIP_ROM);
    OW_write_byte(OW_READ_SCRATCHPAD);              // Start reading the 9 bytes from the scratchpad
    lsb = OW_read_byte();                           // Scratchpad byte 0: LSB of temperature
    msb = OW_read_byte();                           // Scratchpad byte 1: MSB of temperature

    result = ((sint16)msb << 8) | (sint16)lsb;
    return result;
}
