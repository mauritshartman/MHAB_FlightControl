/*
 * File:   radio.c
 * Author: Maurits
 *
 * Created on 27 september 2015
 *
 * Routines for handling the radio modem.
 */

#include "radio.h"

#include <stdio.h>
#include <string.h>


static void rtty_send_byte(ubyte b, ubyte invert);


ubyte init_radio(void)
{
    // Make the radio enable pin an output and hold low (disables NTX2)
    RADIO_ENABLE_DIR = OUTPUT;
    RADIO_ENABLE_PIN = LOW;
    
    // Make the radio TX pin an output and hold low (logic low for NTX2)
    RADIO_TX_DIR = OUTPUT;
    RADIO_TX_PIN = LOW;

    printf("RADIO ");
    return TRUE;
}


void enable_radio(ubyte invert)
{
    RADIO_ENABLE_PIN = HIGH;
    if (invert) {
        RADIO_TX_PIN = LOW;    // Transmit mark
    }
    else {
        RADIO_TX_PIN = HIGH;    // Transmit mark
    }
    __delay_ms(30);              // NTX2 manual indicates at least 5ms startup time for full TX power
}


void disable_radio(void)
{
    RADIO_ENABLE_PIN = LOW;
}


void rtty_send(ubyte *msg, ubyte invert)
{
    uint16 i;
    
    ClrWdt();                   // Prevent reset during transmission
    for (i = 0; msg[i] != 0;i++) {
        rtty_send_byte(msg[i], invert);
    }
}


/*
 * Transmit the byte, starting with the LSB. We want to achieve 300 baud so
 * every bit should take 3.33ms
 */
/*
#define TTY_WAIT 20
static void rtty_send_byte(ubyte b, ubyte invert)
{
    ubyte i, j, val;
    
    if (invert) {
        RADIO_TX_PIN = HIGH;            // Start bit - send space
        __delay_ms(TTY_WAIT);
        
        for (i = 0; i < 8; i++) {
            val = (b & 0x01) ? LOW: HIGH;  // Sending LS-bit first
            if (val) {
                RADIO_TX_PIN = LOW;         // Low for 20 ms
                __delay_ms(TTY_WAIT);      // Delay for 20ms (50 baud)
            }
            else {
                // 75% pulse width: 100 iterations, each 2 * 100 us
                for (j = 0; j < 100; j++) {
                    RADIO_TX_PIN = HIGH;
                    __delay_us(20);
                    //RADIO_TX_PIN = LOW;
                    __delay_us(100);
                    __delay_us(80);
                }
            }
            b >>= 1;					// shift the data byte right for the next bit to send
        }
        
        RADIO_TX_PIN = LOW;            // Stop bit - send mark
        __delay_ms(TTY_WAIT);
    }
    else {
        RADIO_TX_PIN = LOW;            // Start bit - send space
        __delay_ms(TTY_WAIT);
        
        for (i = 0; i < 8; i++) {
            val = (b & 0x01) ? HIGH: LOW;  // Sending LS-bit first
            if (val) {
                RADIO_TX_PIN = LOW;         // Low for 20 ms
                __delay_ms(TTY_WAIT);      // Delay for 20ms (50 baud)
            }
            else {
                // 50% pulse width: 100 iterations, each 2 * 100 us
                for (j = 0; j < 100; j++) {
                    RADIO_TX_PIN = HIGH;
                    __delay_us(20);
                    //RADIO_TX_PIN = LOW;
                    __delay_us(100);
                    __delay_us(80);
                }
            }
            b >>= 1;					// shift the data byte right for the next bit to send
        }
        
        RADIO_TX_PIN = HIGH;            // Stop bit - send mark
        __delay_ms(TTY_WAIT);
    }
}
*/


static void rtty_send_byte(ubyte b, ubyte invert)
{
    ubyte i;
    ubyte val;
    
    if (invert) {
        RADIO_TX_PIN = HIGH;            // Start bit - send space
        __delay_ms(20);
        
        for (i = 0; i < 8; i++) {
            RADIO_TX_PIN = (b & 0x01) ? LOW: HIGH;  // Sending LS-bit first
            b >>= 1;					// shift the data byte right for the next bit to send
            __delay_ms(20);             // Delay for 20ms (50 baud)
        }
        
        RADIO_TX_PIN = LOW;            // Stop bit - send mark
        __delay_ms(20);
    }
    else {
        RADIO_TX_PIN = LOW;             // Start bit - send space
        __delay_ms(20);
        
        for (i = 0; i < 8; i++) {
            RADIO_TX_PIN = (b & 0x01) ? HIGH: LOW;  // Sending LS-bit first
            b >>= 1;					// shift the data byte right for the next bit to send
            __delay_ms(20);             // Delay for 20ms (50 baud)

        }
        
        RADIO_TX_PIN = HIGH;            // Stop bit - send mark
        __delay_ms(20);
    }
}


/*
 * Set the tone either high or low
 */
void rtty_tone(ubyte high)
{
    if (high) {
        RADIO_TX_PIN = HIGH;
    }
    else {
        RADIO_TX_PIN = LOW;
    }
}


#define RADIO_BUF 80
void send_record(record *rec)
{
    ubyte buf[RADIO_BUF];
    ubyte tele[40];
    ubyte lat[12];
    ubyte lon[13];
    sint16 temp_in, temp_ex;
    uint24 temp;
    sint32 pf24bfix;

    memset(buf, '\0', RADIO_BUF);
    memset(tele, '\0', sizeof(tele));
    memset(lat, '\0', sizeof(lat));
    memset(lon, '\0', sizeof(lon));
    
    // Process the temperature into internal and external temp:
    temp = rec->ru.telemetry.temperature; // Printf routine does not handle 24-bit types well.
    temp_in = ((sint16)(temp & 0x000fff)) / 10;
    temp_ex = ((sint16)(temp >> 12)) / 16;
    
    enable_radio(global_config.ru.config.radio_invert);

    lat[0] = rec->ru.telemetry.latitude[0];
    lat[1] = rec->ru.telemetry.latitude[1];
    lat[2] = ' ';
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
    lon[3] = ' ';
    lon[4] = rec->ru.telemetry.longitude[3];
    lon[5] = rec->ru.telemetry.longitude[4];
    lon[6] = '.';
    lon[7] = rec->ru.telemetry.longitude[5];
    lon[8] = rec->ru.telemetry.longitude[6];
    lon[9] = rec->ru.telemetry.longitude[7];
    lon[10] = rec->ru.telemetry.longitude[8];
    lon[11] = (rec->ru.telemetry.status2.lon_hemi == 1) ? 'W': 'E';
    
    pf24bfix = (sint32)rec->ru.telemetry.alt_gps;
    sprintf(tele, "  MHAB%u %u%u %ldm ", \
            global_config.ru.config.last_record, \
            global_config.ru.config.mode, \
            (ubyte)rec->status.status_byte, \
            pf24bfix \
        );
    strcat(buf, tele);
    strcat(buf, lat);
    strcat(buf, ",");
    strcat(buf, lon);
    
    memset(tele, '\0', sizeof(tele));
    pf24bfix = (sint32)rec->ru.telemetry.pressure; // Printf routine does not handle 24-bit types well.
    sprintf(tele, " %dTi %dTe %ldP\r\n", temp_in, temp_ex, pf24bfix);
    strcat(buf, tele);
    printf(buf);
    
    // Send message and disable radio:
    rtty_send(buf, global_config.ru.config.radio_invert);
    disable_radio();
}
