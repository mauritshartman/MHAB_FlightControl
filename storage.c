/*
 * File:   storage.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:48
 *
 * Routines for handling the external EEPROM.
 */

#include "storage.h"
#include "i2c.h"

#include "serial.h"
#include "util.h"

#include <stdio.h>
#include <string.h>


// Initialize by retrieving the most recent configuration block:
ubyte init_storage(void)
{
    if (!retr_record(0, &global_config)) {
        printf("\r\nError initializing storage\r\n");
        return FALSE;
    }
    printf("EEPROM ");
    return TRUE;
}


/**
 * Wipe the entire storage error and set all bytes to the value specified
 * @param c Character to fill the EEPROM with.
 * @return
 */
ubyte wipe_storage(ubyte c)
{
     // There is only 120 bytes of stack frame avaiable:
    ubyte buf[WIPE_BUFFER_SIZE];
    uint16 i;

    memset(buf, c, WIPE_BUFFER_SIZE);

    // Wipe the first page of the first block (except the config record):
    printf("Wiping EEPROM (any key to interrupt):\r\n");
    printf("Low block, page 0\r\n");
    if (!i2c_eeprom_page_write(sizeof(record), I2C_24LC1026_LOW_BLK, buf, WIPE_BUFFER_SIZE - sizeof(record), TRUE)) { return FALSE; }
    if (!i2c_eeprom_page_write(WIPE_BUFFER_SIZE,      I2C_24LC1026_LOW_BLK, buf, WIPE_BUFFER_SIZE          , TRUE)) { return FALSE; }

    // Wipe all of pages 1 - 512 of the low block (B0 = 0):
    for (i = 1; i < PAGES_PER_BLOCK; i++) {
        printf("Low block, page %d\r\n", i);
        if (!i2c_eeprom_page_write(i * I2C_24LC1026_PAGE_SIZE,                    I2C_24LC1026_LOW_BLK, buf, WIPE_BUFFER_SIZE, TRUE)) { return FALSE; }
        if (!i2c_eeprom_page_write(i * I2C_24LC1026_PAGE_SIZE + WIPE_BUFFER_SIZE, I2C_24LC1026_LOW_BLK, buf, WIPE_BUFFER_SIZE, TRUE)) { return FALSE; }

        // Clear watchdog timer and check if user entered anything
        ClrWdt();
        if (data_rdy_uart()) { return TRUE; }
    }

    // Wipe all of the 512 pages of the high block (B0 = 1):
    for (i = 0; i < PAGES_PER_BLOCK; i++) {
        printf("High block, page %d\r\n", i);
        if (!i2c_eeprom_page_write(i * I2C_24LC1026_PAGE_SIZE,                    I2C_24LC1026_HIGH_BLK, buf, WIPE_BUFFER_SIZE, TRUE)) { return FALSE; }
        if (!i2c_eeprom_page_write(i * I2C_24LC1026_PAGE_SIZE + WIPE_BUFFER_SIZE, I2C_24LC1026_HIGH_BLK, buf, WIPE_BUFFER_SIZE, TRUE)) { return FALSE; }

        // Clear watchdog timer and check if user entered anything
        ClrWdt();
        if (data_rdy_uart()) { return TRUE; }
    }
    return TRUE;
}
 

/**
 * Dump the contents of the EEPROM to the screen (32 bytes wide)
 */
/*
void dump_storage(void)
{
    ubyte j = 0;
    uint16 i;
    ubyte buf[CHUNK_SIZE];

    // 1. Display the contents of the lower block, 32 bytes at a time
    printf("Dumping EEPROM (any key to interrupt):\r\n");
    for (i = 0; i < CHUNKS_PER_BLOCK; i++) {
        // Retrieve a chunk of 32 bytes:
        if (!i2c_eeprom_sequence_read(i * CHUNK_SIZE, I2C_24LC1026_LOW_BLK, buf, CHUNK_SIZE)) {
            printf("Error reading bytes at address 0%04Xh.\r\n", i * CHUNK_SIZE);
            return;
        }

        // Display it:
        printf("0%04Xh: ", i * CHUNK_SIZE);
        for (j = 0; j < CHUNK_SIZE; j++) {
            if (j % 4 == 0) { putch(' '); }
            if (j == CHUNK_SIZE / 2) { printf("- "); }
            printf("%02X", buf[j]);
        }
        printf("\r\n");

        // Clear watchdog timer and check if user entered anything
        ClrWdt();
        if (data_rdy_uart()) { return; }
    }

    // 2. Display the contents of the higher block, 32 bytes at a time
    for (i = 0; i < CHUNKS_PER_BLOCK; i++) {
        if (!i2c_eeprom_sequence_read(i * CHUNK_SIZE, I2C_24LC1026_HIGH_BLK, buf, CHUNK_SIZE)) {
            printf("Error reading bytes at address 1%04Xh.\r\n", i * CHUNK_SIZE);
            return;
        }

        // Display it:
        printf("1%04Xh: ", i * CHUNK_SIZE);
        for (j = 0; j < CHUNK_SIZE; j++) {
            if (j % 4 == 0) { putch(' '); }
            if (j == CHUNK_SIZE / 2) { printf("- "); }
            printf("%02X", buf[j]);
        }
        printf("\r\n");

        // Clear watchdog timer and check if user entered anything
        ClrWdt();
        if (data_rdy_uart()) { return; }
    }
}
*/


// Save the given record at the record slot (counted from 0) in the memory array:
// NB: record slot 0 is reserved for configuration
ubyte save_record(uint16 num, record *rec)
{
    // Determine whether to put the record in the high or low block:
    if (num < RECORDS_PER_BLOCK) {  // Lower block (num cannot be negative)
        if (!i2c_eeprom_page_write(num * sizeof(record), I2C_24LC1026_LOW_BLK,  (ubyte *)rec, sizeof(record), FALSE)) { return FALSE; }
    }
    else if (num >= RECORDS_PER_BLOCK && num < RECORDS_PER_BLOCK * 2) { // High block
        if (!i2c_eeprom_page_write(num * sizeof(record), I2C_24LC1026_HIGH_BLK, (ubyte *)rec, sizeof(record), FALSE)) { return FALSE; }
    }
    else { return FALSE; } // Wrong number - unable to fit in memory

    delay_1sec();   // Just to make sure the record is properly written

    return TRUE;
}


ubyte retr_record(uint16 num, record *rec)
{
    // Determine whether to retrieve the record from the high or low block:
    if (num < RECORDS_PER_BLOCK) {  // Lower block (num cannot be negative)
        if (!i2c_eeprom_sequence_read(num * sizeof(record), I2C_24LC1026_LOW_BLK,  (ubyte *)rec, sizeof(record))) { return FALSE; }
    }
    else if (num >= RECORDS_PER_BLOCK && num < RECORDS_PER_BLOCK * 2) { // High block
        if (!i2c_eeprom_sequence_read(num * sizeof(record), I2C_24LC1026_HIGH_BLK, (ubyte *)rec, sizeof(record))) { return FALSE; }
    }
    else { return FALSE; } // Wrong number - unable to fit in memory or tried to save in config slot

    return TRUE;
}
