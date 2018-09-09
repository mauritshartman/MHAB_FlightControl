/* 
 * File:   config.h
 * Author: Maurits
 *
 * Created on December 24, 2014, 4:25 PM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif
    
// Configuration bits:
// Address 0x300000
#pragma config USBDIV = 0           // Use system clock for USB (not used)
#pragma config CPUDIV = 0           // No division of the system clock (for USB)
#pragma config PLLDIV = 0           // No PLL division of the system clock (for USB)

// Address 0x300001
#pragma config FOSC = HS            // External resonator (20.00MHz used, High Speed)
#pragma config IESO = OFF           // Internal/external oscillator switchover disabled
#pragma config FCMEN = OFF          // Fail-safe clock monitor disabled

// Address 0x300002
#pragma config BOR = OFF            // Disable brown-out reset
#pragma config VREGEN = OFF         // Disable internal USB voltage regulator
#pragma config PWRT = ON            // Enable power-up timer (holds CPU in reset for 65ms during startup for clock to stabilize)

// Address 0x300003
#pragma config WDT = ON             // Watchdog timer enabled
#pragma config WDTPS = 8192         // Watchdog timer period: 8192 * 4ms = 33 seconds

// Address 0x300005
#pragma config MCLRE = OFF          // MCLR pin disabled (MCLR must be kept high for operation
#pragma config PBADEN = OFF         // Disable AD on PortB pins for eg. I2C, important for I2C functioning!!
#pragma config LPT1OSC = OFF        // Timer1 configured for high power operation
#pragma config CCP2MX = OFF         // CCP2 input/output multiplexed with RB3: TODO watchout!

// Address 0x300006
#pragma config DEBUG = OFF          // Background debugger enabled, RB6 and RB7 are dedicated to In-Circuit Debug
#pragma config XINST = OFF          // Extended instruction set not used (not supported by XC8)
#pragma config LVP = OFF            // Low voltage programming disabled
#pragma config STVREN = ON          // Stack overflow will cause reset
#pragma config ICPRT = OFF          // Disable dedicated programming port (only on 44-pin devices)

// Address 0x300008
#pragma config CP0 = OFF            // Disable code protection for block 0
#pragma config CP1 = OFF            // Disable code protection for block 1
#pragma config CP2 = OFF            // Disable code protection for block 2
#pragma config CP3 = OFF            // Disable code protection for block 3

// Address 0x300009
#pragma config CPD = OFF            // Disable code protection
#pragma config CPB = OFF            // Disable data EEPROM code protection

// Address 0x30000A
#pragma config WRT0 = OFF           // Disable code protection for block 0
#pragma config WRT1 = OFF           // Disable code protection for block 1
#pragma config WRT2 = OFF           // Disable code protection for block 2
#pragma config WRT3 = OFF           // Disable code protection for block 3

// Address 0x30000B
#pragma config WRTD = OFF            // Disable code protection
#pragma config WRTB = OFF            // Disable data EEPROM code protection
#pragma config WRTC = OFF            // Disable boot block code protection

// Address 0x30000C
#pragma config EBTR0 = OFF           // Disable code protection for block 0
#pragma config EBTR1 = OFF           // Disable code protection for block 1
#pragma config EBTR2 = OFF           // Disable code protection for block 2
#pragma config EBTR3 = OFF           // Disable code protection for block 3

// Address 0x30000D
#pragma config EBTRB = OFF            // Disable code protection

#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */

