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
static uint16 crc16_checksum(ubyte *string);
static uint16 _crc_xmodem_update (uint16 crc, ubyte data);



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
    uint16 i, len_msg;
    
    ClrWdt();                   // Prevent reset during transmission
    
    len_msg = strlen(msg);
    for (i = 0; i < len_msg; i++) {
        rtty_send_byte(msg[i], invert);
    }
}


static void rtty_send_byte(ubyte b, ubyte invert)
{
    ubyte i;
    
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
    
    } else {
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
    } else {
        RADIO_TX_PIN = LOW;
    }
}

/*
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
    lat[10] = (rec->ru.telemetry.status2.north_hemi == 1) ? 'N': 'S';
    
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
    lon[11] = (rec->ru.telemetry.status2.east_hemi == 1) ? 'E': 'W';
    
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
}*/

// Based on https://ukhas.org.uk/communication:protocol
#define RADIO_BUF_UKHAS 90
void send_record(record *rec)
{
    ubyte out[RADIO_BUF_UKHAS];
    ubyte buf[16];
    ubyte i;
    
    sint16 temp_in, temp_ex;
    uint24 temp;
    sint32 pf24bfix;

    // Callsign and sentence_id, using CALLSIGN with SSID based on http://www.aprs.org/aprs11/SSIDs.txt
    memset(out, '\0', RADIO_BUF_UKHAS);
    sprintf(out, "$$PD8MAH-11,%u,", global_config.ru.config.last_record);
    
    // Time
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "%u:%u:%u,", rec->ru.telemetry.hours, rec->ru.telemetry.minutes, rec->ru.telemetry.seconds);
    strcat(out, buf);
    
    // Latitude
    memset(buf, '\0', sizeof(buf));
    i = 0;
    if (!rec->ru.telemetry.status2.north_hemi) { // southern hemisphere
        buf[i++] = '-';
    }
    buf[i++] = rec->ru.telemetry.latitude[0]; // D
    buf[i++] = rec->ru.telemetry.latitude[1]; // D
    buf[i++] = rec->ru.telemetry.latitude[2]; // m
    buf[i++] = rec->ru.telemetry.latitude[3]; // m
    buf[i++] = '.';                           // .
    buf[i++] = rec->ru.telemetry.latitude[4]; // m
    buf[i++] = rec->ru.telemetry.latitude[5]; // m
    buf[i++] = rec->ru.telemetry.latitude[6]; // m
    buf[i++] = rec->ru.telemetry.latitude[7]; // m
    strcat(out, buf);
    
    // Longitude
    memset(buf, '\0', sizeof(buf));
    i = 0;
    if (!rec->ru.telemetry.status2.east_hemi) { // western hemisphere
        buf[i++] = '-';
    }
    buf[i++] = rec->ru.telemetry.longitude[0]; // D
    buf[i++] = rec->ru.telemetry.longitude[1]; // D
    buf[i++] = rec->ru.telemetry.longitude[2]; // D
    buf[i++] = rec->ru.telemetry.longitude[3]; // m
    buf[i++] = rec->ru.telemetry.longitude[4]; // m
    buf[i++] = '.';                           // .
    buf[i++] = rec->ru.telemetry.longitude[5]; // m
    buf[i++] = rec->ru.telemetry.longitude[6]; // m
    buf[i++] = rec->ru.telemetry.longitude[7]; // m
    buf[i++] = rec->ru.telemetry.longitude[8]; // m
    strcat(out, buf);
    
    // Altitude
    pf24bfix = (sint32)rec->ru.telemetry.alt_gps;
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "%d,", pf24bfix);
    strcat(out, buf);
    
    // Temperatures
    temp = rec->ru.telemetry.temperature; // Printf routine does not handle 24-bit types well.
    temp_in = ((sint16)(temp & 0x000fff)) / 10;
    temp_ex = ((sint16)(temp >> 12)) / 16;
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "%d,%d,", temp_in, temp_ex);
    strcat(out, buf);
    
    // Pressure
    pf24bfix = (sint32)rec->ru.telemetry.pressure;
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "%d,", pf24bfix);
    strcat(out, buf);
    
    // Mode and status:
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "%u,%02X", global_config.ru.config.mode, rec->status.status_byte);
    strcat(out, buf);  
    
    // Checksum:
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "*%04X\n", crc16_checksum(out));
    strcat(out, buf);
    printf(out);
    
    // Send message and disable radio:
    enable_radio(global_config.ru.config.radio_invert);
    rtty_send(out, global_config.ru.config.radio_invert);
    disable_radio();
}


static uint16 crc16_checksum(ubyte *string)
{
	uint16 i, crc;
	ubyte c;
 
	crc = 0xffff;
 
	// Calculate checksum ignoring the first two $s
	for (i = 2; i < strlen(string); i++) {
		c = string[i];
		crc = _crc_xmodem_update(crc, c);
	}
 
	return crc;
}


static uint16 _crc_xmodem_update (uint16 crc, ubyte data)
{
    ubyte i;
    
    crc = crc ^ ((uint16)data << 8);
    
    for (i = 0; i < 8; i++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc <<= 1;
        }
    }
    
    return crc;
}
