/**
 * File:   record.c
 * Author: Maurits
 *
 * Created on 24 november 2012
 */

#include "record.h"

#include "serial.h"

#include <stdio.h>


/**
 * Create a human readable printout of a record.
 * @param rec The record to be printed
 */
void print_record(record *rec)
{
    ubyte i;
    sint16 temp_in, temp_ex;
    uint24 temp;
    sint32 pf24bfix;

    printf("{\r\n");
    
    printf("\"ascending\": %u, ", rec->status.ascending);
    printf("\"moving\": %u, ", rec->status.moving);
    printf("\"parachute\": %u, ", rec->status.chute_deployed);
    printf("\"config\": %u, ", rec->status.config);
    printf("\"GPS\": %u, ", rec->status.gps_lock);
    printf("\"radio\": %u, ", rec->status.radio_on); // TODO reimplement
    printf("\"GSM\": %u, ", rec->status.gsm_on);
    printf("\"error\": %u,\r\n", rec->status.error);

    printf("\"time\": {\"days\": %u, \"hours\": %2u, \"minutes\": %2u, \"seconds\": %2u },\r\n", \
            rec->ru.telemetry.days, \
            rec->ru.telemetry.hours, \
            rec->ru.telemetry.minutes, \
            rec->ru.telemetry.seconds);

    temp = rec->ru.telemetry.temperature;
    temp_in = ((sint16)(temp & 0x000fff)) / 10;
    temp_ex = ((sint16)(temp >> 12)) / 16;
    printf("\"temp_in\": %d, \"temp_ex\": %d,\r\n", temp_in, temp_ex);
    printf("\"pressure\": \"%lu Pa\", ", rec->ru.telemetry.pressure);
    printf("\"baro digital\": %u,\r\n", rec->ru.telemetry.status2.baro_digi);

    printf("\"position\": { \"lat\": \"");
    for (i = 0; i < 8; i++) {
        if (i == 2) { putch(' '); }     // Put a space after the degrees (dd mm.mmmm output format)
        if (i == 4) { putch('.'); }     // Put a dot after the first two minute decimals
        putch(rec->ru.telemetry.latitude[i]);
    }
    if (rec->ru.telemetry.status2.lat_hemi == 1) { printf(" N"); }
    else { printf(" S"); }
    printf("\", \"lon\": \"");

    for (i = 0; i < 9; i++) {
        if (i == 3) { putch(' '); }     // Put a space after the degrees (ddd mm.mmmm output format)
        if (i == 5) { putch('.'); }     // Put a dot after the first two minute decimals
        putch(rec->ru.telemetry.longitude[i]);
    }
    if (rec->ru.telemetry.status2.lon_hemi == 1) { printf(" W"); }
    else { printf(" E"); }
    printf("\" },\r\n");

    pf24bfix = (sint32)rec->ru.telemetry.alt_gps;   // Printf routine does not handle 24 bit types well
    printf("\"altitude\": \"%ld m\"\r\n", pf24bfix);
    
    printf("}\r\n");
}
