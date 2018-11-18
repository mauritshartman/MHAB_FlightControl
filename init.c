/*
 * File:   init.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:10
 */

#include "init.h"
#include "defs.h"
#include "record.h"

#include "serial.h"
#include "gsm.h"
#include "radio.h"
#include "storage.h"
#include "i2c.h"
#include "analog_pressure.h"
#include "digital_pressure.h"
#include "temperature.h"

#include <stdio.h>
#include <pic18f4550.h>


static void init_ports(void);


void init(void)
{
    // Initialize interrupts
    INTCONbits.GIE = SET;   // Enable global interrupts
    INTCONbits.PEIE = SET;  // Enable peripheral interrupts
    RCONbits.IPEN = CLEAR;  // Disable interrupt priority: all interrupts will be vectored from address 0x0008h

    // Set all ports to input (save power):
    init_ports();

    // Initialize serial communication & make sure COM SEL0 and COM SEL1 are pulled up:
    init_serial();
    printf("\r\nDaedalus Flight Controller  -  Version 1.0 (c) 2018, MA Hartman\r\n");
    init_i2c();

    // Initialize sensors:
    if (!init_gsm()) { return; }
    if (!init_radio()) { return; }
    if (!init_bmp180_pressure()) { return; }
    if (!init_temperature()) { return; }

    // Initialize storage and retrieve last saved configuration:
    if (!init_storage()) { return; }

    printf("OK\r\n");
}


static void init_ports(void)
{
    // All ports as inputs (to save power):
    TRISA = 0xFF;
    TRISB = 0xFF;
    TRISC = 0xFF;
    TRISD = 0xFF;
    
    // Specifically leave GSM-related pins as outputs:
    TRISEbits.TRISE0 = OUTPUT;  // GSM_ENABLE_PIN
    TRISEbits.TRISE1 = OUTPUT;  // GSM_PWR_PIN
    TRISEbits.TRISE2 = INPUT;
}
