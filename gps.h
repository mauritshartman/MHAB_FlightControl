/*
 * File:   gps.h
 * Author: Maurits
 *
 * Created on 30 september 2012, 14:59
 *
 * GPS handling routines
 */

#ifndef GPS_H
#define	GPS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"

// Buffer to hold complete NMEA sentences:
#define NMEA_BUF_SIZE 100
extern ubyte global_gps_buf[NMEA_BUF_SIZE];


typedef struct {
    ubyte time[12];         // hhmmss.ssss format
    ubyte latitude[10];     // ddmm.mmmm format (including dot and leading zeroes)
    ubyte lat_hemi[2];      // N or S
    ubyte longitude[11];    // dddmm.mmmm format (including dot and leading zeroes)
    ubyte lon_hemi[2];      // E or W
    ubyte pos_fix[2];       // 0: no fix, 1: valid, SPS; 2: valid, differential GPS
    ubyte satellites[3];    // Number of satellites in view
    ubyte dilution[6];      // Dilution precision 00.0 - 99.9
    ubyte altitude[10];     // -9999.9 - 17999.9
} gps_pos;


ubyte get_position(gps_pos *pos);
void print_position(gps_pos *pos);

#ifdef	__cplusplus
}
#endif

#endif	/* GPS_H */
