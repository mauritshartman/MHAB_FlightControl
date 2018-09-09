 /* 
 * File:   defs.h
 * Author: Maurits
 *
 * Created on December 24, 2014, 4:24 PM
 */

#ifndef DEFS_H
#define	DEFS_H

#ifdef	__cplusplus
extern "C" {
#endif

// We use a clock frequency of 20 MHz:
#define _XTAL_FREQ 20000000

#include <xc.h>
#include <p18f4550.h>      // IDE will instruct compiler what to use

// More verbose debugging information:
#define DEBUG_ON 1
    
// General definitions:
#ifndef TRUE
#define TRUE    0x01
#endif
#ifndef FALSE
#define FALSE   0x00
#endif

#define	HIGH    0x01
#define	LOW     0x00
#define	OUTPUT  0x00
#define	INPUT   0x01
#define	SET     0x01
#define	CLEAR   0x00

// Commonly used types:
typedef unsigned char       ubyte;  // [0 - 255]
typedef signed char         sbyte;  // [-128 - 127]
typedef short               sint16; // [-32768 - 32767]
typedef unsigned short      uint16; // [0 - 65536]
typedef short long          sint24; // [-8388608 - 8388607]
typedef unsigned short long uint24; // [0 - 16777215]
typedef long                sint32; // [-2147483648 - 2147483647]
typedef unsigned long       uint32; // [0 - 4294967295]


// Flight status definitions:
#define MODE_COMMAND        (ubyte)0   /* Command interpreting mode */
#define MODE_PRELAUNCH      (ubyte)1   /* Launch mode enabled, but probe still on the ground without GPS */
#define MODE_PRELAUNCH_GPS  (ubyte)2   /* Probe still on ground but GPS lock acquired */
#define MODE_ASC_MAIN       (ubyte)3   /* Probe is ascending */
#define MODE_ASC_MAIN2      (ubyte)4   /* Probe is ascending */
#define MODE_ASC_TIMEOUT    (ubyte)5   /* Probe is still ascending but timeout (6 hrs) reached */
/*#define MODE_ASC_DRIFT      (ubyte)6   /* Probe barely ascending for some time, but main timeout not yet reached */
#define MODE_DESC_BURST     (ubyte)7   /* Probe descending because of burst balloon, pyro not activated */
#define MODE_DESC_PYRO      (ubyte)8   /* Probe descending, pyro activated to jettison balloon */
#define MODE_DESC_GSM       (ubyte)9   /* Probe descending below 2km so GSM will be enabled */
#define MODE_LANDED         (ubyte)10  /* Probe has landed (because of lack of movement */
#define MODE_ERROR          (ubyte)0xff


// Pin definitions:
#define COM_SEL0_PIN    LATBbits.LATB2      // There are 10k pull-ups on this line
#define COM_SEL0_DIR    TRISBbits.TRISB2
#define COM_SEL1_PIN    LATBbits.LATB3      // There are 10k pull-ups on this line
#define COM_SEL1_DIR    TRISBbits.TRISB3
#define COM_ENABLE_PIN  LATBbits.LATB4      // Active low: enable device if this pin is low
#define COM_ENABLE_DIR  TRISBbits.TRISB4

#define RADIO_ENABLE_PIN    LATCbits.LATC0      // Active high: enable NTX2
#define RADIO_ENABLE_DIR    TRISCbits.TRISC0    // Should be output
#define RADIO_TX_PIN    LATCbits.LATC1          // PWM
#define RADIO_TX_DIR    TRISCbits.TRISC1

#define GSM_ENABLE_PIN  LATEbits.LATE0
#define GSM_ENABLE_DIR  TRISEbits.TRISE0
#define GSM_ENABLE_PORT PORTEbits.RE0       // Used for testing the state
#define GSM_PWR_PIN     LATEbits.LATE1
#define GSM_PWR_DIR     TRISEbits.TRISE1
#define GSM_PWR_PORT    PORTEbits.RE1       // Used for testing state

#define PARACHUTE_DIR   TRISAbits.TRISA5    // Parachute deployment (charge)
#define PARACHUTE_PIN   LATAbits.LATA5

#define UART_TX_DIR     TRISCbits.TRISC6    // UART port configuration
#define UART_RX_DIR     TRISCbits.TRISC7

#define OW_PIN_DIR      TRISAbits.TRISA4    // 1-Wire port configuration
#define OW_WRITE_PIN    LATAbits.LATA4
#define OW_READ_PIN     PORTAbits.RA4

#define I2C_SDA_DIR     TRISBbits.TRISB0    // I2C port configuration
#define I2C_SCL_DIR     TRISBbits.TRISB1


#ifdef	__cplusplus
}
#endif

#endif	/* DEFS_H */

