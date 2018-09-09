/*
 * File:   gsm.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:25
 *
 * Routines for handling the GSM modem.
 */

#include "gsm.h"
#include "serial.h"
#include "util.h"

#include <stdio.h>
#include <string.h>


static void delay_xsec(ubyte secs);


// Initialize the GSM
ubyte init_gsm(void)
{
    GSM_ENABLE_DIR = OUTPUT;
    GSM_PWR_DIR = OUTPUT;
    
    GSM_ENABLE_PIN = HIGH;          // Enable power supply permanently (also keep cap charged)
    GSM_PWR_PIN = LOW;
    printf("GSM ");
    return TRUE;
}

/**
 * Turn on power to the GSM.
 */
void enable_gsm(void)
{
    ubyte i;
    ubyte pin[SIZE_PHONE_PIN + 1];  // Ensure null-termination

    memset(pin, '\0', sizeof(pin));
    for (i = 0; i < SIZE_PHONE_PIN; i++) {
        pin[i] = global_config.ru.config.phone_pin[i];
    }

    // Turn on power to the GSM subsystem, wait 20 seconds:
    printf("Power on GSM modem and sending PIN...");
    serial_channel(SELECT_GSM);
    GSM_PWR_PIN = HIGH;                 // Turn VIO pin high (3.3V), which effectively enables the SIM800H
    delay_xsec(3);                      // Wait for 3 seconds. See SIM800 HD documentation.

    printf("AT+CPIN=\"%s\"\r\n", pin);    // Send PIN number
    delay_xsec(1);
    printf("AT+CMGF=1\r\n");              // Put GSM modem in text mode
    delay_xsec(10);                     // Allow for GSM to make connection to network

    serial_channel(SELECT_PC);
    printf("OK\r\n");
    
    global_config.status.gsm_on = 1;
}

/**
 * Turn off power to the GSM.
 */
void disable_gsm(void)
{
    // Simply turn off power to the GSM module:
    printf("Powering down GSM modem...");
    GSM_PWR_PIN = LOW;
    printf("OK\r\n");
    
    global_config.status.gsm_on = 0;
}


/**
 * Send SMS message in the given NULL-TERMINATED buffer.
 * @param msg Message to send - must be null-terminated.
 */
void send_sms(ubyte *msg)
{
    // Switch serial mux to GSM:
    printf("Send SMS message to %s: '%s'...", global_config.ru.config.cell_number, msg);
    serial_channel(SELECT_GSM);

    // Send sms:
    printf("AT+CMGS=\"%s\"\r\n", global_config.ru.config.cell_number);
    delay_xsec(1);
    printf("%s%c", msg, CTRLZ);
    delay_1sec();
    //TODO: error checking if SMS was accepted?

    serial_channel(SELECT_PC);
    printf("OK\r\n");
}


/**
 * Send a telemetry record by SMS message
 * @param rec
 */
void send_sms_record(record *rec)
{
    ubyte lat[12];
    ubyte lon[13];
    sint16 temp_in, temp_ex;
    uint24 temp;
    sint32 pf24bfix;

    memset(lat, '\0', sizeof(lat));
    memset(lon, '\0', sizeof(lon));
    
    // Switch serial mux to GSM:
    printf("Send SMS message to %s...", global_config.ru.config.cell_number);
    serial_channel(SELECT_GSM);

    // Send sms:
    printf("AT+CMGS=\"%s\"\r\n", global_config.ru.config.cell_number);
    delay_xsec(1);

    
    // Process the temperature into internal and external temp:
    temp = rec->ru.telemetry.temperature;   // Printf routine does not handle 24-bit types well.
    temp_in = ((sint16)(temp & 0x000fff)) / 10;
    temp_ex = ((sint16)(temp >> 12)) / 16;

    lat[0] = rec->ru.telemetry.latitude[0];
    lat[1] = rec->ru.telemetry.latitude[1];
    lat[2] = '+';
    lat[3] = rec->ru.telemetry.latitude[2];
    lat[4] = rec->ru.telemetry.latitude[3];
    lat[5] = '.';
    lat[6] = rec->ru.telemetry.latitude[4];
    lat[7] = rec->ru.telemetry.latitude[5];
    lat[8] = rec->ru.telemetry.latitude[6];
    lat[9] = rec->ru.telemetry.latitude[7];
    lat[10] = (rec->ru.telemetry.status2.lat_hemi == 1) ? 'N': 'S';
    
    lon[0] = rec->ru.telemetry.longitude[0];
    lon[1] = rec->ru.telemetry.longitude[1];
    lon[2] = rec->ru.telemetry.longitude[2];
    lon[3] = '+';
    lon[4] = rec->ru.telemetry.longitude[3];
    lon[5] = rec->ru.telemetry.longitude[4];
    lon[6] = '.';
    lon[7] = rec->ru.telemetry.longitude[5];
    lon[8] = rec->ru.telemetry.longitude[6];
    lon[9] = rec->ru.telemetry.longitude[7];
    lon[10] = rec->ru.telemetry.longitude[8];
    lon[11] = (rec->ru.telemetry.status2.lon_hemi == 1) ? 'W': 'E';
    
    pf24bfix = (sint32)rec->ru.telemetry.alt_gps;
    printf("MHAB%u %u%u %ldm http://maps.google.com/maps?q=%s,%s %dTi %dTe", \
            global_config.ru.config.last_record, \
            global_config.ru.config.mode, \
            (ubyte)rec->status.status_byte, \
            pf24bfix, \
            lat, \
            lon, \
            temp_in, \
            temp_ex \
        );
    
    // Send ctrl-z character to close sms and switch serial channel:
    printf("%c", CTRLZ);
    delay_1sec();
    serial_channel(SELECT_PC);
    printf("OK\r\n");
}


/**
 * Delay for x seconds, while clearing the watchdog timer.
 * @param secs The number of seconds to delay
 */
static void delay_xsec(ubyte secs)
{
    ubyte i;
    for (i = 0; i < secs; i++) {
        delay_1sec();
        ClrWdt();
    }
}


/**
 * Test whether the GSM modem is ready for SMS sending or not.
 * @return true iff the GSM modem is turned on and is connected to the network
 */
ubyte sms_ready(void)
{
    ubyte buf[GSM_MODEM_BUFSIZE];
    memset(buf, '\0', sizeof(buf));

    // 1. First test whether the GSM power is turned on:
    if (GSM_ENABLE_PORT == HIGH && GSM_PWR_PORT == HIGH) {
#ifdef DEBUG_ON
        printf("GSM modem is enabled.\r\n");
#endif
    }
    else {
#ifdef DEBUG_ON
        printf("GSM modem is not enabled.\r\n");
#endif
        return FALSE;
    }

    return TRUE;

    // 2. Then test the network status: TODO: fix this, not working yet.
    /*serial_channel(SELECT_GSM);
    printf("AT+CREG?\r");
    alt_gets(buf, sizeof(buf));
    delay_1sec();
    serial_channel(SELECT_PC);
    printf("AT+CREG response: %s\r\n", buf);

    return TRUE;*/
}
