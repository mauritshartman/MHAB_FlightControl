/*
 * File:   util.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 21:55
 */

#include "util.h"
#include "serial.h"

#include <string.h>


// Alternative gets() implementation. Reads from the USART and outputs the characters into
// the specified array until one of two conditions arises:
// 1) The given buffer is full
// 2) A LF is encountered (LF and CR are never copied into the buffer)
void alt_gets(ubyte *buf, ubyte buf_size)
{
    ubyte i = 0, c = 0;

    // Clear the given buffer:
    memset(buf, '\0', buf_size);

    // Read characters until the buffer is full or LF is encountered:
    while (i < (buf_size - 1) && c != '\n') {   // Make sure the buffer is always null-terminated
        // Blocking wait for a character to arrive and echo it to the screen:
        c = getc_uart();
        putch(c);

        // Copy the character to the buffer if it's not a CR or LF:
        if (c != '\r' && c != '\n') {
            buf[i] = c;
            i++;
        }
        else { break; }
    }
}


void alt_gets_no_echo(ubyte *buf, ubyte buf_size)
{
    ubyte i = 0, c = 0;

    // Clear the given buffer:
    memset(buf, '\0', buf_size);

    // Read characters until the buffer is full or LF is encountered:
    while (i < (buf_size - 1) && c != '\n') {   // Make sure the buffer is always null-terminated
        // Blocking wait for a character to arrive:
        c = getc_uart();

        // Copy the character to the buffer if it's not a CR or LF:
        if (c != '\r' && c != '\n') {
            buf[i] = c;
            i++;
        }
        else { break; }
    }
}


/**
 * Delay for one second.
 * TODO: verify this calculation.
 */
void delay_1sec(void)
{
    ubyte i;

    // Delay for 10M cycles = 1 sec @20MHz:
    for (i = 0; i < 100; i++) {
        __delay_ms(10);
    }
}
