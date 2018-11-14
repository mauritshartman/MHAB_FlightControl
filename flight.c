/*
 * File:   flight.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:16
 *
 * Flight controller.
 *
 */

#include "flight.h"
#include "record.h"
#include "gps.h"
#include "gsm.h"
#include "parachute.h"
#include "serial.h"
#include "storage.h"
#include "radio.h"
#include "temperature.h"
#include "analog_pressure.h"
#include "digital_pressure.h"
#include "util.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>


// Prototypes for handling states:
static void handle_state_prelaunch(record *);
static void handle_state_prelaunch_gps(record *);
static void handle_state_asc_main(record *);
static void handle_state_asc_main2(record *);
static void handle_state_asc_timeout(record *);
static void handle_state_desc_burst(record *);
static void handle_state_desc_pyro(record *);
static void handle_state_desc_gsm(record *);
static void handle_state_landed(record *);

// General prototypes
static void orientate(record *, record *);
static void sensor_measurements(record *);
static void position_measurements(record *, record *);
static ubyte mov_compare_pos(record *, record *);
static void time_measurements(record *, gps_pos *, record *);
static void prep_prev_record(record *);
static void prep_curr_record(record *, record *);
static void save_curr_record_config(record *);
static void set_mode(ubyte);


/**
 * Flight control logic routines that controls the probe during flight.
 */
void flight_control(void)
{
    record curr_rec, prev_rec;
    memset(&curr_rec, '\0', sizeof(record));
    memset(&prev_rec, '\0', sizeof(record));

    // Prepare the previous and current records:
    prep_prev_record(&prev_rec);
    prep_curr_record(&curr_rec, &prev_rec);

    // Determine state and action: based on previous state:
    switch (global_config.ru.config.mode) {
        default:
        case MODE_PRELAUNCH:
            handle_state_prelaunch(&curr_rec); break;
        case MODE_PRELAUNCH_GPS:
            handle_state_prelaunch_gps(&curr_rec); break;
        case MODE_ASC_MAIN:
            handle_state_asc_main(&curr_rec); break;
        case MODE_ASC_MAIN2:
            handle_state_asc_main2(&curr_rec); break;
        case MODE_ASC_TIMEOUT:
            handle_state_asc_timeout(&curr_rec); break;
        case MODE_DESC_BURST:
            handle_state_desc_burst(&curr_rec); break;
        case MODE_DESC_PYRO:
            handle_state_desc_pyro(&curr_rec); break;
        case MODE_DESC_GSM:
            handle_state_desc_gsm(&curr_rec); break;
        case MODE_LANDED:
            handle_state_landed(&curr_rec); break;
    }
    
    // Save and send current record as the last record and update the global config
    save_curr_record_config(&curr_rec);
    send_record(&curr_rec);
}


static void set_mode(ubyte m) {
#ifdef DEBUG_ON
    printf("Switch to mode %u\r\n", m);
#endif
    global_config.ru.config.mode = m;
}


static void handle_state_prelaunch(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_PRELAUNCH\r\n");
#endif
            
    if (!curr_rec->status.moving) { // Not moving, determine GPS lock
        if (curr_rec->status.gps_lock) {    // GPS lock acquired, move to MODE_PRELAUNCH_GPS
            set_mode(MODE_PRELAUNCH_GPS);
        }
        // not moving, but also no GPS lock yet so stay in MODE_PRELAUNCH
        
    }
    else {  // Moving! Move to MODE_ASC_MAIN if also ascending (GPS lock is implied by moving)
        if (curr_rec->status.ascending) {
            set_mode(MODE_ASC_MAIN);
        }
    }
}


static void handle_state_prelaunch_gps(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_PRELAUNCH_GPS\r\n");
#endif
    
    if (!curr_rec->status.moving) { // Not moving, determine GPS lock
        if (!curr_rec->status.gps_lock) {    // Lost GPS lock, move back to MODE_PRELAUNCH
            set_mode(MODE_PRELAUNCH);
        }
    }
    else {  // Moving! Move to MODE_ASC_MAIN if also ascending (GPS lock is implied by moving)
        if (curr_rec->status.ascending) {
            set_mode(MODE_ASC_MAIN);
        }
    }    
}


static void handle_state_asc_main(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_ASC_MAIN\r\n");
#endif
    if (curr_rec->ru.telemetry.alt_gps > 20000) {
        set_mode(MODE_ASC_MAIN2);
    }
}


#define RECS_HIST 10
static void handle_state_asc_main2(record *curr_rec)
{
    // WARN: 32 + 11 * 3 + 2 + 1 + 2 = 70 bytes of stack memory (or even 80 in debug)
    record rec;
    sint24 alts[RECS_HIST + 1];
    sint16 h_diff;
    ubyte i;
    uint16 lr = global_config.ru.config.last_record;
#ifdef DEBUG_ON
    sint32 pf24bfix;
    
    printf("MODE_ASC_MAIN2 ");
#endif
    
    // If less than 10 records are created, remain in ASC_MAIN2.
    if (lr < RECS_HIST) {
#ifdef DEBUG_ON
        printf("Remain - not enough records \r\n");
#endif
        return;
    }
    
    // Read the previous RECS_HIST records, oldest first:
    for (i = 0; i < RECS_HIST; i++) {
        retr_record(lr - (RECS_HIST - 1) + i, &rec);
        alts[i] = rec.ru.telemetry.alt_gps;
#ifdef DEBUG_ON
        pf24bfix = (sint32)alts[i];
        printf("%ld ", pf24bfix);
#endif
    }
    alts[RECS_HIST] = curr_rec->ru.telemetry.alt_gps;
#ifdef DEBUG_ON
    pf24bfix = (sint32)alts[RECS_HIST];
    printf("%ld\r\n", pf24bfix);
#endif
    
    // Read the last 5 records and see if they are monotonically descending, if so: DESC_BURST:
    for (i = RECS_HIST - 5; i < RECS_HIST; i++) {
        if (alts[i] <= alts[i + 1]) { break; }  // Only continue is next record is lower than the one before it
    }
    if (i == RECS_HIST) {   // Each record is lower than the one before it
        set_mode(MODE_DESC_BURST);
        return;
    }
    
    // Flight time is more than 6 hours:
    if (curr_rec->ru.telemetry.days > 0) {
#ifdef DEBUG_ON
        printf("Not the same day. Switch to MODE_ASC_TIMEOUT\r\n");
#endif
        set_mode(MODE_ASC_TIMEOUT);
        return;
    }
    h_diff = curr_rec->ru.telemetry.hours - global_config.ru.config.l_hours;
    if (h_diff > 6 || (h_diff == 5 && curr_rec->ru.telemetry.minutes >= global_config.ru.config.l_minutes)) {
 #ifdef DEBUG_ON
        printf("More than 6 hours. Switch to MODE_ASC_TIMEOUT\r\n");
#endif
        set_mode(MODE_ASC_TIMEOUT);
        return;       
    }
        
    // Default: we stay in ASC_MAIN2
}


static void handle_state_asc_timeout(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_ASC_TIMEOUT\r\n");
#endif
    // Fire pyros and deploy chute:
    deploy_parachute();
    set_mode(MODE_DESC_PYRO);
}


static void handle_state_desc_burst(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_DESC_BURST\r\n");
#endif
    // Fire pyros and deploy chute:
    deploy_parachute();
    set_mode(MODE_DESC_PYRO);  
}


static void handle_state_desc_pyro(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_DESC_PYRO\r\n");
#endif
    if (curr_rec->ru.telemetry.alt_gps < 2000) {
        set_mode(MODE_DESC_GSM);
    }
}


static void handle_state_desc_gsm(record *curr_rec)
{
#ifdef DEBUG_ON
    printf("MODE_DESC_GSM\r\n");
#endif
    
    if (!curr_rec->status.moving) {
        set_mode(MODE_LANDED);
        save_record(0, &global_config); // Save the new mode in case SMS takes too long
    }
    
    // Transmit position over radio first (in GSM boot-up takes too long)
    send_record(curr_rec);
    
    // Transmit position over sms
    if (!global_config.status.gsm_on || !sms_ready()) {    // Turn on GSM (should not be necessary)
        enable_gsm();
    }
    send_sms_record(curr_rec);
}


static void handle_state_landed(record *curr_rec) {
#ifdef DEBUG_ON
    printf("MODE_LANDED\r\n");
#endif
    
    // Transmit position over radio first (in GSM boot-up takes too long)
    send_record(curr_rec);
    
    // Transmit position over sms
    if (!global_config.status.gsm_on || !sms_ready()) {    // Turn on GSM (should not be necessary)
        enable_gsm();
    }
    send_sms_record(curr_rec);
}


/**
 * Retrieve the previous record. If there is none, create a dummy record:
 * Altitude is set to the lowest possible value in that record, and movement is set to ascending.
 */
static void prep_prev_record(record *prev_rec)
{
    // 0. Retrieve previous flight record:
    if (global_config.ru.config.last_record == 0) {     // This is the first record: create dummy previous record
        prev_rec->status.ascending = 0;                 // Assume probe has just been launched and is on the ground in prelaunch
        prev_rec->status.moving = 0;                    // Assume not yet moving
        prev_rec->status.config = 0;                    // Telemetry record
        prev_rec->ru.telemetry.alt_gps = SHRTLONG_MIN;  // Lowest possible height: next record will always be higher
#ifdef DEBUG_ON
        printf("No previous record found, created dummy one\r\n");
#endif
    }
    else {
        retr_record(global_config.ru.config.last_record, prev_rec);
#ifdef DEBUG_ON
        printf("Previous record retrieved: ID %u\r\n", global_config.ru.config.last_record);
#endif
    }    
}


static void prep_curr_record(record *curr_rec, record *prev_rec)
{
    // 1. Take measurements:
    curr_rec->status.config = 0;
    orientate(curr_rec, prev_rec);
    print_record(curr_rec);
}


static void save_curr_record_config(record *curr_rec)
{
    // Save current record as the last record and update the global config
    global_config.ru.config.last_record++;
    if (global_config.ru.config.last_record >= RECORDS_PER_BLOCK * 2) {
        // When we exceed the 4095 telemetry records, we keep rewriting the final (4095) record:
        global_config.ru.config.last_record = RECORDS_PER_BLOCK * 2 - 1;
    }
#ifdef DEBUG_ON
    printf("Saving record: ID %u\r\n", global_config.ru.config.last_record);
#endif
    save_record(global_config.ru.config.last_record, curr_rec);
    save_record(0, &global_config);     // Save global config to EEPROM
}


/**
 * Take measurements, get the GPS position and set several flags in the location
 * record.
 * @param curr_rec The current record to be populated
 * @param prev_rec Previous record
 */
static void orientate(record *curr_rec, record *prev_rec)
{
    // 1. Get sensor data, and GPS time and position:
    sensor_measurements(curr_rec);
    position_measurements(curr_rec, prev_rec);
    
    curr_rec->status.ascending = (curr_rec->ru.telemetry.alt_gps > prev_rec->ru.telemetry.alt_gps) ? 1: 0;
    curr_rec->status.moving = mov_compare_pos(curr_rec, prev_rec);
    
    // TODO 2. Set the necessary status flags:
    if (sms_ready()) { curr_rec->status.gsm_on = 1; }
    else { curr_rec->status.gsm_on = 0; }
    curr_rec->status.radio_on = 1;       // APRS is always on
    curr_rec->status.chute_deployed = global_config.status.chute_deployed;
}


/**
 * Compare longitude, latitude and altitude: if all equal, return 0, otherwise 1 for moving.
 * @param curr_rec
 * @param prev_rec
 * @return 0 iff long, lat and alt are all equal (and there is a GPS lock).
 */
static ubyte mov_compare_pos(record *curr_rec, record *prev_rec)
{
    ubyte i;
    
    // If there was or is no GPS lock, return 0 (no move) as nothing real movement can be determined:
    if (!curr_rec->status.gps_lock || !prev_rec->status.gps_lock) {
        return 0;
    }
    
    // Determine if longitude is equal:
    for (i = 0; i < 9; i++) {   // 9 chars for longitude
        if (curr_rec->ru.telemetry.longitude[i] != prev_rec->ru.telemetry.longitude[i]) {
            return 1;   // Longitude not equal: immediately return
        }
    }
    
    // Determine if latitude is equal:
    for (i = 0; i < 8; i++) {   // 8 chars for latitude
        if (curr_rec->ru.telemetry.latitude[i] != prev_rec->ru.telemetry.latitude[i]) {
            return 1;   // Latitude not equal: immediately return
        }
    }
    
    // Determine if altitude is equal:
    if (curr_rec->ru.telemetry.alt_gps != prev_rec->ru.telemetry.alt_gps) {
        return 1;       // Not equal: immediately return
    }
    
    return 0;   // Everything equal, so return 0 (no move)
}


/**
 * Perform pressure and temperature measurements and parse into record
 * @param curr_rec
 */
static void sensor_measurements(record *curr_rec)
{
    sint16 temp_in, temp_ex;
    uint24 temp = 0;

    // 1. Get temperature:
    temp_in = get_internal_temp();
    temp_ex = get_external_temp();
    temp = ((uint24)temp_ex << 12) | ((uint24)temp_in & 0x000fff);  // Shift together into 24 bits
    curr_rec->ru.telemetry.temperature = temp;

    // 2. Get BMP180 pressure:
    curr_rec->ru.telemetry.status2.baro_digi = 1;
    curr_rec->ru.telemetry.pressure = read_bmp180_pressure();
    /*
    // 2. Get pressure (alternating between digital (uneven) and analog pressure (even records)):
    if (global_config.ru.config.last_record % 2 == 0) {
        curr_rec->ru.telemetry.status2.baro_digi = 1;
        curr_rec->ru.telemetry.pressure = read_bmp180_pressure();
    }
    else {
        curr_rec->ru.telemetry.status2.baro_digi = 0;
        curr_rec->ru.telemetry.pressure = read_analog_pressure();
    }
    */
}


/**
 * Add position (GPS) information to the given curr_rec.
 * @param curr_rec Current telemetry record which is being updated
 */
static void position_measurements(record *curr_rec, record *prev_rec)
{
    gps_pos pos;
    ubyte i;
    sint32 tmp_alt;

    // 1. Retrieve GPS information:
    get_position(&pos);

    // 2. Set hemisphere (N vs S and E vs W) information:
    curr_rec->ru.telemetry.status2.north_hemi = (pos.lat_hemi[0] == 'N') ? 1: 0;
    curr_rec->ru.telemetry.status2.east_hemi = (pos.lon_hemi[0] == 'E') ? 1: 0;

    // 3. Check for GPS fix:
    if (pos.pos_fix[0] == '1') {
        curr_rec->status.gps_lock = 1;
        curr_rec->status.error = 0;
    }
    else {  // No lock: set error flag:
        curr_rec->status.gps_lock = 0;
        curr_rec->status.error = 1;
    }

    // 3. Set time:
    time_measurements(curr_rec, &pos, prev_rec);
    
    // 4. Set altitude (complex logic to avoid 32 to 24 bit truncation and changes in sign):
    tmp_alt = atol(pos.altitude);
    if (tmp_alt > SHRTLONG_MAX) {
        curr_rec->ru.telemetry.alt_gps = SHRTLONG_MAX;  // 8389 km which is unlikely
    }
    else if (tmp_alt < SHRTLONG_MIN) {
        curr_rec->ru.telemetry.alt_gps = SHRTLONG_MAX;  // -8389 km which is equally unlikely
    }
    else {
        curr_rec->ru.telemetry.alt_gps = (sint24)tmp_alt;   // Should be safe
    }

    // 5. Set latitude and longitude:
    curr_rec->ru.telemetry.longitude[0] = pos.longitude[0];             // The 100 degree byte of longitude
    for (i = 0; i < 4; i++) {                                           // Lat / lon information up to the dot (ddmm.):
        curr_rec->ru.telemetry.latitude[i] = pos.latitude[i];
        curr_rec->ru.telemetry.longitude[i + 1] = pos.longitude[i + 1]; // The first decimal is already handled
    }
    for (i = 4; i < 8; i++) {                                           // Lat / lon information after the dot (.mmmm):
        curr_rec->ru.telemetry.latitude[i] = pos.latitude[i + 1];
        curr_rec->ru.telemetry.longitude[i + 1] = pos.longitude[i + 2]; // The first decimal is already handled
    }
}


/**
 * Parse NMEA GPGGA time information (hhmmss) into record.
 * @param curr_rec
 * @param pos
 */
static void time_measurements(record *curr_rec, gps_pos *pos, record *prev_rec)
{
    ubyte buf[3];
    
    buf[0] = pos->time[0];
    buf[1] = pos->time[1];
    buf[2] = '\0';
    curr_rec->ru.telemetry.hours = (ubyte)atoi(buf);
            
    buf[0] = pos->time[2];
    buf[1] = pos->time[3];
    buf[2] = '\0';
    curr_rec->ru.telemetry.minutes = (ubyte)atoi(buf);
 
    buf[0] = pos->time[4];
    buf[1] = pos->time[5];
    buf[2] = '\0';
    curr_rec->ru.telemetry.seconds = (ubyte)atoi(buf);
    
    // In order to determine the days, have a look at the previous record:
    if (curr_rec->ru.telemetry.hours < prev_rec->ru.telemetry.hours) {
        // Increment the number of days:
        curr_rec->ru.telemetry.days = prev_rec->ru.telemetry.days + 1;
    }
    else {  // Keep the same number of days as the previous record:
        curr_rec->ru.telemetry.days = prev_rec->ru.telemetry.days;
    }
}
