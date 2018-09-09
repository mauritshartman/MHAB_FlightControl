/*
 * File:   command.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:13
 *
 * Routines and definitions for the command interpreter for when the device is in COMMAND mode.
 */


#include "defs.h"
#include "command.h"
#include "record.h"
#include "util.h"

#include "storage.h"
#include "analog_pressure.h"
#include "digital_pressure.h"
#include "temperature.h"
#include "gsm.h"
#include "gps.h"
#include "parachute.h"
#include "serial.h"
#include "radio.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Configuration strings:
const ubyte *global_help_string = \
"Please enter one of the following commands:\r\n" \
"a    Send AT command to GSM modem.\r\n" \
"c    View and/or set the GSM PIN.\r\n" \
"C    Compose a test SMS message and send it.\r\n" \
"d    Dump all logged records from the EEPROM to the serial.\r\n" \
"g    Enable GSM and view status.\r\n" \
"G    Configure the GSM phone number to send SMS to.\r\n" \
"h    Test whether the GSM modem is ready to send SMS messages.\r\n" \
"H    Disable GSM.\r\n" \
"l    Switch to flight mode and start logging.\r\n" \
"L    Display the number of the last logged record.\r\n" \
"n    Display a particular record.\r\n" \
"N    Wipe the complete EEPROM.\r\n" \
"p    Display analog and digital pressures.\r\n" \
"P    Retrieve BMP180 coefficients and set ASDX015 calibration.\r\n" \
"q    Display position and GPS status.\r\n" \
"s    Put the microcontroller to sleep.\r\n" \
"t    Display temperature information.\r\n" \
"x    Radio RTTY invert settings.\r\n" \
"X    Test parachute deployment mechanism.\r\n" \
"y    Continuously send a message on the NTX2 radio.\r\n" \
"Y    Radio testing: set tone high or low.\r\n" \
"z    Turn on the NTX2 radio.\r\n" \
"Z    Turn off the NTX2 radio.\r\n" \
"\r\n";


// Function prototypes:
static void cmd_at_command(void);
static void cmd_phone_number(void);
static void cmd_phone_pin(void);
static void cmd_pressure(void);
static void cmd_position(void);
static void cmd_storage(void);
static void cmd_dump_storage(void);
static void cmd_launch(void);
static void cmd_print_record(void);
static void cmd_cut_parachute(void);
static void cmd_test_sms(void);
static void cmd_sms_ready(void);
static void cmd_sleep(void);
static void cmd_radio_on(void);
static void cmd_radio_off(void);
static void cmd_radio_msg(void);
static void cmd_radio_tone(void);
static void cmd_radio_invert(void);
static void set_print_launch_time(void);


// Continuously loop and ask the user for a command, unless the user initiates the launch mode
// The assumption is made that the mode has already been set to MODE_COMMAND
void command_loop(void)
{
    ubyte c;

    // Continuously loop until the mode is explicitly set to launch:
    while (TRUE) {

        // Wait for a command and echo it back to the user:
        printf("\r\nMain menu (? for help): ");
        while (!data_rdy_uart()) { ClrWdt(); }
        c = getc_uart();
        printf("%c\r\n", c);

        // Interpret command:
        switch (c) {
            case 'a':       // Run an AT command:
                cmd_at_command(); break;
            case 'c':       // Set the GSM PIN number
                cmd_phone_pin(); break;
            case 'C':       // Send test SMS using GSM.
                cmd_test_sms(); break;
            case 'd':       // Dump all logged records
                cmd_dump_storage(); break;
            case 'g':       // Enable GSM and view status
                enable_gsm(); break;
            case 'G':       // Configure the GSM phone number to send position sms to
                cmd_phone_number(); break;
            case 'h':
                cmd_sms_ready(); break;
            case 'H':       // Disable GSM
                disable_gsm(); break;
            case 'l':       // Switch to launch mode and reset
                cmd_launch(); break;
            case 'L':
                printf("Last logged telemetry record: %u\r\n", global_config.ru.config.last_record);
                break;
            case 'n':
                cmd_print_record(); break;
            case 'N':
                cmd_storage(); break;
            case 'p':
                printf("Analog: %lu Pa, digital: %lu Pa\r\n", (uint32)read_analog_pressure(), (uint32)read_bmp180_pressure());
                break;
            case 'P':
                cmd_pressure(); break;
            case 'q':
                cmd_position(); break;
            case 's':
                cmd_sleep(); break;
            case 't':       // Display temperature information
                printf("Internal: %d C, external: %d C\r\n", get_internal_temp() / 10, get_external_temp() / 16);
                break;
            case 'x':
                cmd_radio_invert(); break;
            case 'X':
                cmd_cut_parachute(); break;
            case 'y':
                cmd_radio_msg(); break;
            case 'Y':
                cmd_radio_tone(); break;
            case 'z':
                cmd_radio_on(); break;
            case 'Z':
                cmd_radio_off(); break;
            default:        // Display help
                printf(global_help_string); break;
        }

        // Gobble up any additional characters the user might have entered:
        while (data_rdy_uart()) { c = getc_uart(); }
    }
}


static void cmd_at_command(void)
{
    ubyte i;
    ubyte cmd_buf[32], recv_buf[64];
    
    if (!sms_ready()) {
        printf("GSM modem not turned on. Use 'g' command\r\n");
        return;
    }
    
    // Clear the buffers and read the command:
    /* alt_gets() clears the buffer before using it:
    memset(cmd_buf, '\0', sizeof(cmd_buf));
    memset(recv_buf, '\0', sizeof(recv_buf));
    */
    printf("Enter AT command: ");
    alt_gets(cmd_buf, sizeof(cmd_buf));
    printf("\r\n");
    
    // Send the command to the GSM
    serial_channel(SELECT_GSM);
    printf("%s\r\n", cmd_buf);
    alt_gets_no_echo(recv_buf, sizeof(recv_buf));
    
    // Switch back to PC channel and output GSM modem response:
    serial_channel(SELECT_PC);
    printf("%s\r\n", recv_buf);
}


static void cmd_phone_number(void)
{
    ubyte c, i;
    ubyte buf[SIZE_CELL_NUMBER];

    // Init the buffer and put the cell number from the config in there:
    memset(buf, '\0', sizeof(buf));
    for (i = 0; i < (ubyte)SIZE_CELL_NUMBER; i++) {
        buf[i] = global_config.ru.config.cell_number[i];
    }
    printf("The current GSM phone number is: %s\r\n", buf);

    // Ask the user whether he/she is sure about changing the number:
    printf("Do you want to change the GSM phone number (y or n): ");
    while (!data_rdy_uart()) { Nop(); }
    c = getc_uart();
    printf("%c\r\n", c);

    // If so, retrieve the number and save it:
    if (c == 'y') {
        printf("Type the new number (max 15 chars, valid phone number): ");
        alt_gets(buf, sizeof(buf) - 1);    // We allow the user only to enter 15 chars (always null-terminated)

        printf("\r\nStoring new phone number: %s...", buf);
        for (i = 0; i < (ubyte)SIZE_CELL_NUMBER; i++) {
            global_config.ru.config.cell_number[i] = buf[i];
        }
        save_record(0, &global_config);
        printf("OK\r\n");
    }
}


#define APC_BUF_SIZE    8
static void cmd_pressure(void)
{
    bmp180_coeff coeff;
    ubyte c;
    sint16 tmp;
    ubyte buf[APC_BUF_SIZE];
    memset(buf, '\0', sizeof(buf));

    // 1. Print out the BMP085 coefficients:
    read_bmp180_coefficients(&coeff);
    printf("The BMP085's coefficients are:\r\n");
    printf("AC1: %d\r\n", coeff.ac1);
    printf("AC2: %d\r\n", coeff.ac2);
    printf("AC3: %d\r\n", coeff.ac3);
    printf("AC4: %u\r\n", coeff.ac4);
    printf("AC5: %u\r\n", coeff.ac5);
    printf("AC6: %u\r\n", coeff.ac6);
    printf("B1:  %d\r\n", coeff.b1);
    printf("B2:  %d\r\n", coeff.b2);
    printf("MB:  %d\r\n", coeff.mb);
    printf("MC:  %d\r\n", coeff.mc);
    printf("MD:  %d\r\n", coeff.md);

    // 2. Prompt user to change the calibration factor:
    printf("Current analog calibration factor is: %d\r\nChange it (y or n): ", global_config.ru.config.apc);
    while (!data_rdy_uart()) { Nop(); }
    c = getc_uart();
    printf("%c\r\n", c);

    // 3. If so, set the new ASDX015A24R calibration factor:
    if (c == 'y') {
        printf("Type new factor (max 7 chars): ");
        alt_gets(buf, sizeof(buf) - 1);    // We allow the user only to enter 7 chars (ensure null-termination)
        tmp = atoi(buf);
        printf("\r\nStoring new calibration factor: %d...", tmp);
        global_config.ru.config.apc = tmp;
        save_record(0, &global_config);
        printf("OK\r\n");
    }
}

/**
 * Display the GPS coordinates.
 */
static void cmd_position(void)
{
    gps_pos pos;
    get_position(&pos);
    print_position(&pos);
}


/**
 * Prompt the user for a character and wipe the EEPROM with that character.
 */
static void cmd_storage(void)
{
    ubyte c;

    // 1. Prompt the user:
    printf("Do you want to use a different character than 0x00 (y or n): ");
    while (!data_rdy_uart()) { Nop(); }
    c = getc_uart();
    printf("%c\r\n", c);

    if (c == 'y') {
        printf("Type character to fill the EEPROM with: ");
        while (!data_rdy_uart()) { Nop(); }
        c = getc_uart();
        printf("%c\r\n", c);
    }
    else { c = 0; }

    // 2. Wipe the array:
    printf("Wiping storage array (value %02X)...", c);
    if (wipe_storage(c)) { printf("OK\r\n"); }
    else { printf("Error"); }
}


static void cmd_dump_storage(void)
{
    uint16 r, num_records;
    record rec;
    
    num_records = global_config.ru.config.last_record;
    printf("{\"number of records\": %u,\r\n\"records\": [", num_records);
    
    for (r = 1; r <= num_records && r < (RECORDS_PER_BLOCK * 2); r++) {
        retr_record(r, &rec);
        print_record(&rec);
        printf(",\r\n");
        ClrWdt();
    }
    printf("{\"dummy\": \"dummy\"}]}");
}


/**
 *
 */
static void cmd_launch(void)
{
    // Set mode to MODE_PRELAUNCH:
    printf("\r\nPROBE LAUNCHED! Switching to prelaunch mode\r\n");
    global_config.status.chute_deployed = 0;
    disable_gsm();  // Make sure GSM is disabled (also clears gsm_on bit in global status)

    global_config.ru.config.mode = MODE_PRELAUNCH;
    global_config.ru.config.last_record = 0;  // Start logging
    set_print_launch_time();    // Determine exact launch time and record    
    
    // Save the global record:
    save_record(0, &global_config);

    // Reset mission clock and reset probe computer:
    Reset();
}


static void set_print_launch_time(void)
{
    gps_pos pos;
    ubyte buf[3];
    
    // Retrieve launch time:
    get_position(&pos);
    
    buf[0] = pos.time[0];
    buf[1] = pos.time[1];
    buf[2] = '\0';
    global_config.ru.config.l_hours = (ubyte)atoi(buf);
    buf[0] = pos.time[2];
    buf[1] = pos.time[3];
    buf[2] = '\0';
    global_config.ru.config.l_minutes = (ubyte)atoi(buf);
    buf[0] = pos.time[4];
    buf[1] = pos.time[5];
    buf[2] = '\0';
    global_config.ru.config.l_seconds = (ubyte)atoi(buf);
    printf("Launch time %u:%u:%u UTC\r\n", \
            global_config.ru.config.l_hours, \
            global_config.ru.config.l_minutes, \
            global_config.ru.config.l_seconds);
}


static void cmd_print_record(void)
{
    uint16 tmp;
    ubyte buf[APC_BUF_SIZE];
    record rec;
    memset(buf, '\0', sizeof(buf));

    printf("Type record number to retrieve (1 to %u) (record is %u bytes): ", (uint16)(RECORDS_PER_BLOCK * 2 - 1), sizeof(record));
    alt_gets(buf, sizeof(buf) - 1);    // We allow the user only to enter 7 chars (ensure null-termination)
    tmp = (uint16)atol(buf);

    if (tmp > 0 && tmp < (RECORDS_PER_BLOCK * 2)) {
        printf("\r\nRetrieving record %u\r\n", tmp);
        retr_record(tmp, &rec);
        print_record(&rec);
    }
    else {
        printf("\r\nIllegal record number entered.\r\n");
    }
}


static void cmd_cut_parachute(void)
{
    printf("Testing parachute deployment mechanism...");
    deploy_parachute();
    printf("Done\r\n");
}


static void cmd_phone_pin(void)
{
    ubyte c, i;
    ubyte buf[SIZE_PHONE_PIN + 1];        // One extra byte for null termination

    // Init the buffer and put the cell number from the config in there:
    memset(buf, '\0', sizeof(buf));
    for (i = 0; i < (ubyte)SIZE_PHONE_PIN; i++) {
        buf[i] = global_config.ru.config.phone_pin[i];
    }
    printf("The current GSM phone number is: %s\r\n", buf);

    // Ask the user whether he/she is sure about changing the number:
    printf("Do you want to change the GSM PIN (y or n): ");
    while (!data_rdy_uart()) { Nop(); }
    c = getc_uart();
    printf("%c\r\n", c);

    // If so, retrieve the number and save it:
    if (c == 'y') {
        printf("Type the new PIN (max 4 chars): ");
        alt_gets(buf, sizeof(buf) - 1);    // We allow the user only to enter 16 chars (always null-terminated)

        printf("\r\nStoring new phone PIN: %s...", buf);
        for (i = 0; i < (ubyte)SIZE_PHONE_PIN; i++) {
            global_config.ru.config.phone_pin[i] = buf[i];
        }
        save_record(0, &global_config);
        printf("OK\r\n");
    }
}


static void cmd_test_sms(void)
{
    ubyte buf[GSM_MODEM_BUFSIZE];
    memset(buf, '\0', sizeof(buf));

    printf("Type the SMS message: ");
    alt_gets(buf, sizeof(buf) - 1); // Ensure null-termination
    printf("\r\n");
    send_sms(buf);
}


static void cmd_sms_ready(void)
{
    if (sms_ready()) {
        printf("GSM modem ready to send SMS messages.\r\n");
    }
    else {
        printf("GSM modem not ready.\r\n");
    }
}


static void cmd_sleep(void)
{
    printf("Going to sleep.\r\n");

    OSCCONbits.IDLEN = 0;   // Real sleep mode
    Sleep();
    Nop();
    
    printf("Woke up.\r\n");
}


static void cmd_radio_on(void)
{
    printf("Turning on NTX2 radio...");
    enable_radio(FALSE);
    printf("OK\r\n");
}


static void cmd_radio_off(void)
{
    printf("Turning off NTX2 radio...");
    disable_radio();
    printf("OK\r\n");    
}


static void cmd_radio_msg(void)
{
    ubyte c, inv;
    ubyte buf[GSM_MODEM_BUFSIZE];
    memset(buf, '\0', sizeof(buf));
    
    // Ask the user whether the message should be sent inverted or not:
    printf("Do you want to invert the bits (y or n): ");
    while (!data_rdy_uart()) { Nop(); }
    c = getc_uart();
    printf("%c\r\n", c);
    inv = (c == 'y') ? TRUE: FALSE; // Only invert the bits if the user explicitly types y:
    
    // Prompt the user for the message:
    printf("Type the message (max %d characters): ", GSM_MODEM_BUFSIZE);
    alt_gets(buf, sizeof(buf) - 1); // Ensure null-termination
    printf("\r\n");
    
    // Loop the message until a key is pressed:
    while (TRUE) {
        rtty_send(buf, inv);
        delay_1sec();
        ClrWdt();
        if (data_rdy_uart()) { break; }
    }
}


static void cmd_radio_tone(void)
{
    ubyte c;
    
    printf("Press 'h' for high tone and 'l' for low, other key to stop:\r\n");
    
    while (TRUE) {
        while (!data_rdy_uart()) { ClrWdt(); }
        c = getc_uart();
        
        if (c == 'h') {
            printf("Tone high\r\n");
            rtty_tone(HIGH);
        }
        else if (c == 'l') {
            printf("Tone low\r\n");
            rtty_tone(LOW);
        }
        else { break; }
    }
}


static void cmd_radio_invert(void)
{
    ubyte c, i;
    
    if (global_config.ru.config.radio_invert) { printf("Radio RTTY is inverted. "); }
    else { printf("Radio RTTY is NOT inverted. "); }

    // Ask the user whether he/she is sure about changing the RTTY setting:
    printf("Do you want to change (y or n): ");
    while (!data_rdy_uart()) { Nop(); }
    c = getc_uart();
    printf("%c\r\n", c);

    // If so, flip the setting:
    if (c == 'y') {
        global_config.ru.config.radio_invert = (global_config.ru.config.radio_invert) ? 0: 1;
        
        save_record(0, &global_config);
        if (global_config.ru.config.radio_invert) { printf("Radio RTTY is now inverted.\r\n"); }
        else { printf("Radio RTTY is now NOT inverted.\r\n"); }
    }    
}
