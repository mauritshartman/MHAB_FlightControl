/*
 * File:   analog_pressure.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:41
 *
 * Routines for handling the analog pressure sensor.
 */

#include "analog_pressure.h"

#include "storage.h"

#include <plib/adc.h>
#include <stdio.h>


ubyte init_analog_pressure(void)
{
    // Configure AD converter on channel 0 (AN0):
    OpenADC(    ADC_FOSC_64 &       // Slow AD clock: 20MHz div 64: 3.2us (= Tad period)
                ADC_RIGHT_JUST &    // Results in least significant bits
                ADC_20_TAD,         // Take 20 Tads for conversion (acquisition is always 11 Tads)
                ADC_CH0 &           // Channel 0 = AN0
                ADC_INT_OFF &       // Disable interrupts
                ADC_VREFPLUS_VDD &  // Vref+ at Vdd, no external Vref+ provided
                ADC_VREFMINUS_VSS,  // Vref- at Vss, no external Vref- provided
                0b1110);            // Configure only AN0 as an AD converter, rest (AN1-12) digital

    __delay_us(5);     // Delay for 5 us
    printf("ANA_BARO ");
    return TRUE;
}


uint24 read_analog_pressure(void)
{
    ubyte counter = 0;
    uint24 result = 0;
    uint32 tmp = 0;

    // 1. Perform an AD conversion
    ConvertADC();                           // Start conversion
    while (BusyADC() && counter++ < (ubyte)100) {  // Wait until conversion is done, or for a maximum of 500ms
        __delay_us(5);
    }
    tmp = (uint32)ReadADC(); // Get the result (10 bit - will be contained in the LSB)

    // 2. Convert to Pascals (and compensate for offset errors in the ASDX015):
    tmp = ((tmp + global_config.ru.config.apc) * 103421) / 1024;  // 103421 Pa = 15PSI (max value of the ASDX015)
    result  = (uint24)tmp;
    return result;
}


void close_analog_pressure(void)
{
    CloseADC();
}

