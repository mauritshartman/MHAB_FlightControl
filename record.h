/*
 * File:   record.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 21:55
 */

#ifndef RECORD_H
#define	RECORD_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"

#define SIZE_CELL_NUMBER    16
#define SIZE_PHONE_PIN      4


typedef struct {
                                    // Bit#      Description
                                    // =======================================================
    union {
    struct {
    ubyte       ascending: 1;       // 00       1 when probe is ascending, 0 if probe is not
    ubyte       moving: 1;          // 01       1 when position is different with respect to previous position
    ubyte       chute_deployed: 1;  // 02       1 if parachute is deployed, 0 if not
    ubyte       gps_lock: 1;        // 03       1 if probe has GPS lock, 0 if not
    ubyte       radio_on: 1;        // 04       1 if Radio NTX2 system is turned on, 0 if not
    ubyte       gsm_on: 1;          // 05       1 if GSM system is turned on, 0 if not
    ubyte       error: 1;           // 06       1 if an error occurred, 0 if not
    ubyte       config: 1;          // 07       1 if the record is a config record
    };
    ubyte       status_byte;
    } status;

union {     // Union: either telemetry or configuration

/**
 * Telemetry-specific data.
 */
struct {
    ubyte       seconds;            // 08 - 15  UTC seconds
    ubyte       minutes;            // 16 - 23  UTC minutes
    ubyte       hours;              // 24 - 31  UTC hours
    ubyte       days;               // 32 - 39  Number of days since launch
    uint24      temperature;        // 40 - 51  External temperature (12 bit)
                                    // 52 - 63  Internal temperature (12 bit)
    uint24      pressure;           // 64 - 87  Analog or digital pressure (24 bit)

    union {
    struct {
    ubyte       baro_digi: 1;       // 88       1 if digital pressure, 0 if analog pressure
    ubyte       lat_hemi: 1;        // 89       1 if North, 0 if South
    ubyte       lon_hemi: 1;        // 90       1 if West, 0 if East
    ubyte       padding: 5;         // 91 - 95  Padding bytes (zero default)
    };
    ubyte       status2_byte;
    } status2;

    ubyte       latitude[8];        // 96 -159  ddmm.mmmm format
    ubyte       longitude[9];       // 160-231  dddmm.mmmm format
    sint24      alt_gps;            // 232-255  GPS altitude (in meters)
} telemetry;

/**
 * Configuration-specific data (address will be 0x00 in the EEPROM for this record)
 */
struct {
    ubyte       mode;               // The mode the device was in.
    uint16      last_altitude;      // The altitude from the last record
    uint16      last_record;        // The number of the last written record

    ubyte       cell_number[SIZE_CELL_NUMBER];  // Space for a 16 digit cell-phone number used for the GSM (16 bytes)
    ubyte       phone_pin[SIZE_PHONE_PIN];      // four digit PIN for the GSM (4 bytes)
    sint16      apc;                // Compensation factor for the ASDX015A24R

    ubyte       l_seconds;          // UTC seconds at launch time
    ubyte       l_minutes;          // UTC minutes at launch time
    ubyte       l_hours;            // UTC minutes at launch time
    
    ubyte       radio_invert;       // Whether the radio RTTY should be inverted
    //ubyte       pad;                // Padding bytes to make config 32 bytes long (strangely not necessary: without it already 32 bytes)
} config;

} ru;     // End of union

} record;   // End of record struct


// The global config is defined in the main.c file:
extern record global_config;

void print_record(record *rec);


#ifdef	__cplusplus
}
#endif

#endif	/* RECORD_H */
