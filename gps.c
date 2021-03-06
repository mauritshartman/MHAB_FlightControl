/*
 * File:   gps.c
 * Author: Maurits
 *
 * Created on 30 september 2012, 14:59
 */

#include "gps.h"

#include "serial.h"

#include <string.h>
#include <stdio.h>


static void retr_gpgga_sentence(ubyte *buf, uint16 bufsize);
static ubyte parse_gpgga(gps_pos *pos);
static uint16 next_token(ubyte *, sint16, ubyte *, uint16);


ubyte global_gps_buf[NMEA_BUF_SIZE];


ubyte get_position(gps_pos *pos)
{
    memset(global_gps_buf, '\0', sizeof(global_gps_buf));

    // 1. Switch to GPS and retrieve a single GPGGA sentence:
    serial_channel(SELECT_GPS);
    retr_gpgga_sentence(global_gps_buf, sizeof(global_gps_buf));
    serial_channel(SELECT_PC);

    // 2. Parse it:
    //printf("buffer: %s\n", global_gps_buf);
    return parse_gpgga(pos);
}


/**
 * Output GPS information in a human readable format.
 * @param pos
 */
void print_position(gps_pos *pos)
{
    ubyte i;
    
    printf("\r\nGPS time (UTC): %s\r\nLatitude: ", pos->time);
    for (i = 0; i < 10; i++) {          // Sizeof latitude within gps_pos
        if (i == 2) { putch(' '); }     // Put a space after the degrees (dd mm.mmmm output format, easier for google maps)
        putch(pos->latitude[i]);
    }
    printf(" %s\r\nLongitude: ", pos->lat_hemi);
    for (i = 0; i < 11; i++) {          // Sizeof longitude within gps_pos
        if (i == 3) { putch(' '); }     // Put a space after the degrees (ddd mm.mmmm output format, easier for google maps)
        putch(pos->longitude[i]);
    }
    printf(" %s\r\n", pos->lon_hemi);
    printf("Height (m): %s\r\n", pos->altitude);
    printf("Position fix status: %s\r\n", pos->pos_fix);
    printf("Satellites in view: %s\r\n", pos->satellites);
    printf("Horizontal dilution of precision (m): %s\r\n", pos->dilution);
}


/**
 * Retrieve characters from the GPS until a complete $GPGGA sentence has been retrieved.
 * @param buf Buffer to parse
 * @param bufsize Size of that buffer
 */
static void retr_gpgga_sentence(ubyte *buf, uint16 bufsize)
{
    ubyte c = 0, b1, b2, b3, b4, b5, i;

    // 1. Parse characters until a '$GPGGA' is encountered:
    while (TRUE) {
        // 1a. Discard all characters until a '$' appears:
        while (c != '$') { c = getc_uart(); }   // getc_uart() implementation is blocking

        // 1b. Grab five more characters and check to see if it's a GPGGA sentence:
        b1 = getc_uart(); b2 = getc_uart(); b3 = getc_uart();
        b4 = getc_uart(); b5 = getc_uart();
        if (b1 == 'G' && b2 == 'P' && b3 == 'G' && b4 == 'G' && b5 == 'A') {
            buf[0] = '$'; buf[1] = 'G'; buf[2] = 'P';
            buf[3] = 'G'; buf[4] = 'G'; buf[5] = 'A';
            i = 6;
            break;
        }
    }

    // 2. If so, read and copy the rest into the buffer until the LF is encountered:
    while (i < bufsize && c != '\n') {
        c = getc_uart();    // Wait for a character to arrive
        // Copy the character to the buffer if it's not a CR or LF:
        if (c != '\r' && c != '\n') {
            buf[i] = c;
            i++;
        }
        else { break; }
    }
}

/**
 * Parse the buffer containing a GPGGA sentence into its constituents.
 * @param pos gps_pos structure that will be holding the GPS information
 * @param buf string buffer containing the GPGGA sentence
 * @param bufsize size of the buffer
 */
static ubyte parse_gpgga(gps_pos *pos)
{
    sint16 index = 0;
    ubyte tok[8];

    // Make copy of the buffer since it will be modified by the strtok function:
    memset(tok,         '\0', sizeof(tok));
    memset(pos,         '\0', sizeof(gps_pos));

    // First token must be $GPGGA:
    index = next_token(global_gps_buf, index, tok, sizeof(tok));
    if (strcmp(tok, "$GPGGA")) { return FALSE; }

    // Parse GPS info: $GPGGA,time,lat,N/S,lon,W/E,fix,satellites,dilution,altitude,M,sep,M,age,id*CS
    index = next_token(global_gps_buf, index, pos->time, sizeof(pos->time));
    index = next_token(global_gps_buf, index, pos->latitude, sizeof(pos->latitude));
    index = next_token(global_gps_buf, index, pos->lat_hemi, sizeof(pos->lat_hemi));
    index = next_token(global_gps_buf, index, pos->longitude, sizeof(pos->longitude));
    index = next_token(global_gps_buf, index, pos->lon_hemi, sizeof(pos->lon_hemi));
    index = next_token(global_gps_buf, index, pos->pos_fix, sizeof(pos->pos_fix));
    index = next_token(global_gps_buf, index, pos->satellites, sizeof(pos->satellites));
    index = next_token(global_gps_buf, index, pos->dilution, sizeof(pos->dilution));
    index = next_token(global_gps_buf, index, pos->altitude, sizeof(pos->altitude));

    return TRUE;
}

/* Tokenize the given buffer from index onwards and copy the token into the given
 output buffer, taking care not to exceed its size. Return the index from whence
 the next call can be made. */
static uint16 next_token(ubyte *buf, sint16 index, ubyte *dest, uint16 dest_size)
{
    ubyte c;
    ubyte out_i = 0;
    
    if (index < 0) { return index; }
    
    dest_size--; // ensure null-termination
    
    c = buf[index++];
    while (c != ',') {
        if (c == '\0') { return -1; }
        
        if (out_i < dest_size) { // make the copy if output buf size allows
            dest[out_i++] = c;
        }
        
        c = buf[index++];
    }
    
    dest[out_i] = '\0';
    return index;
}
