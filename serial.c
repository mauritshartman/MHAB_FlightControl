/*
 * File:   serial.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:21
 *
 * Routines for selecting the right serial (RX/TX) channel.
 */

#include "serial.h"


// Global variables
volatile ubyte global_uart_buffer;  // Volatile since it will be accessed by both normal code and  ISR
volatile ubyte global_uart_new_data;    // True iff unread data is in the uart buffer.

// Function prototypes
static void open_uart(uint16 sbrg, ubyte inversion);
static void close_uart(void);
void interrupt uart_isr(void);


/**
 * Initialize the UART and select the PC channel.
 * @return True iff initialization went OK.
 */
ubyte init_serial(void)
{
    // Set pins RB2 (COM SEL 0) and RB3 (COM SEL 1) as outputs:
    disable_serial();           // Should already be HIGH because of pullups (disables serial mux)
    COM_SEL0_DIR = OUTPUT;  // COM select 0
    COM_SEL1_DIR = OUTPUT;  // COM select 1
    COM_ENABLE_DIR = OUTPUT;  // COM enable

    serial_channel(SELECT_PC);
    return TRUE;
}


/**
 * Select the communications channel by setting COM SEL 0 and 1 (74H4052 serial mux).
 * @param channel Channel to select (GPS, APRS, GSM or PC).
 */
void serial_channel(ubyte channel)
{
    // 1. Close UART and disable serial MUX:
    close_uart();   // Close UART before turning of the serial mux (to avoid spurious characters)
    disable_serial();

    // 2. Select serial mux channel and open UART at appropriate speed:
    switch (channel) {
        case SELECT_GPS:
            COM_SEL0_PIN = LOW;
            COM_SEL1_PIN = LOW;
            open_uart(BAUD4800, FALSE);
            break;
        case SELECT_GSM:
            COM_SEL0_PIN = HIGH;
            COM_SEL1_PIN = LOW;
            open_uart(BAUD9600, FALSE);
            break;
        /* Not used anymore - NTX2 radio is on dedicated pin
        case SELECT_APRS:
            COM_SEL0_PIN = LOW;
            COM_SEL1_PIN = HIGH;
            open_uart(BAUD4800, FALSE);
            break;
            */
        case SELECT_PC:
        default:
            COM_SEL0_PIN = HIGH;
            COM_SEL1_PIN = HIGH;
            open_uart(BAUD115200, FALSE);
            break;
    }

    enable_serial();
}


/**
 * Blocking wait for a character to arrive from the UART.
 * @return The received character
 */
ubyte getc_uart(void)
{
    while (!data_rdy_uart()) { Nop(); } // Blocking wait for character to arrive

    // Clear new data flag and return charactor from buffer:
    global_uart_new_data = CLEAR;
    return global_uart_buffer;
}


/**
 * Send a single character to the UART.
 * @param c The character to send
 */
void putch(ubyte c)
{
    while (!TXSTAbits.TRMT) { Nop(); }  // Wait until the TSR buffer is empty
    TXREG = c;      // Write the data byte to the USART
    while (!TXSTAbits.TRMT) { Nop(); }  // Wait until the TSR buffer is empty
}


/**
 * Open the EUSART properly.
 * @param sbrg Baudrate generator value
 * @param inversion Whether to invert the RX signal.
 */
static void open_uart(uint16 sbrg, ubyte inversion)
{
    ubyte c;

    // Invalidate any data in the buffer:
    global_uart_new_data = CLEAR;

    RCSTA = CLEAR;              // Reset USART registers to POR state:
    TXSTA = CLEAR;

    UART_TX_DIR = INPUT;        // TX pin also as input - a bit strange but this is done in other examples as well
    UART_RX_DIR = INPUT;        // RX pin

    TXSTAbits.TX9 =  CLEAR;     // 8-bit transmission
    TXSTAbits.SYNC = CLEAR;     // asynchronous mode
    TXSTAbits.SENDB = CLEAR;    // don't use the break character
    TXSTAbits.BRGH = SET;       // high speed mode
    RCSTAbits.RX9 =  CLEAR;     // 8-bit reception

    if (inversion) { BAUDCONbits.RXDTP = SET; } // Inversion of the RX data
    else { BAUDCONbits.RXDTP = CLEAR; } // No inversion of the RX data
    BAUDCONbits.TXCKP = CLEAR;  // No inversion of the TX data
    BAUDCONbits.BRG16 = SET;    // Enable 16 bit BRG
    BAUDCONbits.WUE = CLEAR;    // Auto wake-up not used
    BAUDCONbits.ABDEN = CLEAR;  // Auto-baud detect feature disabled

    PIE1bits.RCIE = SET;        // Enable interrupt on receive
    PIR1bits.RCIF = CLEAR;      // Clear EUSART interrupt receive flag
    PIE1bits.TXIE = CLEAR;      // Disable interrupt on transmission

    SPBRG = sbrg;               // Write lower 8 bits of baudrate into SPBRG
    SPBRGH = sbrg >> 8;         // Write upper 8 bits of baudrate into SPBRGH

    TXSTAbits.TXEN = SET;       // Enable transmitter
    RCSTAbits.CREN = SET;       // Enable receiver
    RCSTAbits.SPEN = SET;       // Enable receiver - configures RX/DT and TX/CK pins as serial port pins

    // Clear receive FIFO queue explicitly:
    c = RCREG; c = RCREG; c = RCREG;
}


/**
 * Close the EUSART properly.
 */
static void close_uart(void)
{
    TXSTAbits.TXEN = 0;  // Disable transmitter
    RCSTAbits.CREN = 0;  // Disable receiver
    RCSTAbits.SPEN = 0;  // Disable receiver

    PIE1bits.RCIE = 0;  // Disable interrupt on receive
    PIE1bits.TXIE = 0;  // Disable interrupt on transmission

    // Invalidate any data in the buffer:
    global_uart_new_data = CLEAR;
}

/*
 * For PIC18cxxx devices the high interrupt vector is found at 00000008h. The
 * following code will branch to the high_interrupt_service_routine function to
 * handle interrupts that occur at the high vector.
 * 
 * Interrupt service routine for the UART, including buffering of one char.
 */
void interrupt uart_isr(void)
{
    ubyte dummy;

    if (PIR1bits.RCIF == SET) {      // Service an EUSART receive interrupt
        if (RCSTAbits.OERR || RCSTAbits.FERR) { // Overrun or frame error:
            RCSTAbits.CREN = CLEAR; // Reset the EUSART
            dummy = RCREG;          // Dummy read to clear the RCIF flag
            RCSTAbits.CREN = SET;   // Enable receiver again.
        }
        else {
            global_uart_buffer = RCREG;  // This will also clear the PIR1.RCIF flag
            global_uart_new_data = (ubyte)SET;
        }
    }
}
