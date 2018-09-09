/*
 * File:   i2c.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:45
 *
 * Routines for handling I2C peripherals.
 */

#include "i2c.h"

void init_i2c(void)
{
    ubyte dummy;

    // Set both SDA and SCL pins as inputs:
    I2C_SDA_DIR = INPUT;
    I2C_SCL_DIR = INPUT;

    // Configure hardware I2C:
    SSPSTAT =   0x80;           // Disable SLEW RATE for 100 kHz
    SSPADD =    49;             // 100Khz operation @20 MHz (Fosc / baudrate) / 4 - 1
    SSPCON1 =   0x28;           // Set SSPEN bit: SCL and SDA are open-drain now, MASTER mode, clock = (Fosc/4)*(SSPADD+1)
    SSPCON2 =   0x00;           // Clear all
    dummy =     SSPBUF;         // Dummy read to completely clear the SSPBUF buffer

    PIR1bits.SSPIF = 0;
    PIR2bits.BCLIF = 0;
}

/**
 * Close the SSP module and with it I2C.
 */
void close_i2c(void)
{
    SSPCON1bits.SSPEN = 0;      // Disable the SSP module to conserve power.
}


/**
 * This function will be in a wait state until Start Condition Enable bit,
 * Stop Condition Enable bit, Receive Enable bit, Acknowledge Sequence
 * Enable bit of I2C Control register and Transmit Status bit I2C Status
 * register are clear. The IdleI2C function is required since the hardware
 * I2C peripheral does not allow for spooling of bus sequence. The I2C
 * peripheral must be in Idle state before an I2C operation can be initiated
 * or write collision will be generated.
 */
void i2c_idle( void )
{
    while ((SSPSTATbits.R_W) | (SSPCON2 & 0x1F));
}

/**
 * Send ACK bit
 */
void i2c_ack(void)
{
    i2c_idle();
    SSPCON2bits.ACKDT = I2C_ACK;
    SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN);
    PIR1bits.SSPIF = 0;
}

/**
 * Send non-ack to end the transaction
 */
void i2c_nack(void)
{
    i2c_idle();
    SSPCON2bits.ACKDT = I2C_NACK;
    SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN);
    PIR1bits.SSPIF = 0;
}

/**
 * Generate the start condition for I2C:
 */
void i2c_start(void)
{
    i2c_idle();                 // Wait for bus to be idle before doing anything
    SSPCON2bits.SEN = 1;        // Raise start condition
    while(SSPCON2bits.SEN);     // Wait for start condition to clear
    PIR1bits.SSPIF = 0;
}

/**
 * Generate the restart condition for I2C:
 */
void i2c_restart(void)
{
    i2c_idle();                 // Wait for bus to be idle before doing anything
    SSPCON2bits.RSEN = 1;       // Raise restart condition
    while(SSPCON2bits.RSEN);    // Wait for restart condition to clear
    PIR1bits.SSPIF = 0;
}

/**
 * Generate the stop condition for I2C:
 */
void i2c_stop(void)
{
    i2c_idle();
    SSPCON2bits.PEN = 1;
    while(SSPCON2bits.PEN);
    PIR1bits.SSPIF = 0;
}

/**
 * Write a byte of data to the I2C bus.
 * @param data The byte to be written
 * @return True iff the write went OK
 */
ubyte i2c_write(const ubyte data)
{
    i2c_idle();
    SSPBUF = data;
    if (SSPCON1bits.WCOL) {             // Check for write collision
        SSPCON1bits.WCOL = 0;
        return FALSE;
    }
    while (SSPSTATbits.BF);             // BF cleared after falling edge of the eighth clock
    i2c_idle();                         // Wait for the bus to be idle
    PIR1bits.SSPIF = 0;                 // Not sure
    return (SSPCON2bits.ACKSTAT == 0);  // True if there was an ack from the slave
}

/**
 * Read a byte from the I2C bus
 * @param buf On success, buf will be filled with the received byte.
 * @return True iff the read went OK
 */
ubyte i2c_read(ubyte *buf)
{
    i2c_idle();
    SSPCON2bits.RCEN = 1;
    if (SSPCON1bits.SSPOV || SSPCON1bits.WCOL) {
        SSPCON1bits.SSPOV = 0;
        SSPCON1bits.WCOL = 0;
        return FALSE;
    }

    while (SSPCON2bits.RCEN);   // Wait until (BF is set) and RCEN is cleared
    PIR1bits.SSPIF = 0;
    *buf = SSPBUF;
    return TRUE;
}

/**
 * Write a byte of data into the 24LC1026 EEPROM at the given address.
 * @param addr Lower 16 bits of the address
 * @param high High bit to select between the high or low block (65K bytes each)
 * @param data Data to write into the EEPROM
 * @return True iff the write operation went OK
 */
ubyte i2c_eeprom_byte_write(const uint16 addr, const ubyte high, const ubyte data)
{
    ubyte ctl, ah, al;

    // 1. Set control byte (high or low block):
    ctl = I2C_EEPROM_ADDR | (high ? I2C_24LC1026_HIGH_BLK : I2C_24LC1026_LOW_BLK);
    al = addr & 0xff;
    ah = addr >> 8;

    // 2. Write the byte:
    i2c_start();
    if (!i2c_write(ctl | I2C_WRITE)) { return FALSE; }
    if (!i2c_write(ah)) { return FALSE; }
    if (!i2c_write(al)) { return FALSE; }
    if (!i2c_write(data)) { return FALSE; }
    i2c_stop();
    return TRUE;
}

/* Write a full page of 128 bytes to the EEPROM based on the data in the given buffer.
 * If more than 128 bytes are given, then the write will wrap around.
 * At the end of the page write, ack polling will be used to wait for the write to complete.
 */
ubyte i2c_eeprom_page_write(const uint16 addr, const ubyte high, const ubyte *buf, uint16 len, ubyte ack_poll)
{
    uint16 i, mod_ps;
    ubyte ctl, ah, al;

    // 1. Set control byte (high or low block):
    ctl = I2C_EEPROM_ADDR | (high ? I2C_24LC1026_HIGH_BLK : I2C_24LC1026_LOW_BLK);
    al = addr & 0xff;
    ah = addr >> 8;

    // 2. Check the address and length for rollovers:
    len = (len < I2C_24LC1026_PAGE_SIZE) ? len : I2C_24LC1026_PAGE_SIZE;    // Cannot write more than page size (128 bytes)
    mod_ps = addr % I2C_24LC1026_PAGE_SIZE;                                 // Calculate address offset within page
    if (mod_ps + len > I2C_24LC1026_PAGE_SIZE) {    // Reduce len to fall to prevent wraparound within the page
        len = I2C_24LC1026_PAGE_SIZE - mod_ps;
    }

    // 3. Write the page:
    i2c_start();
    if (!i2c_write(ctl | I2C_WRITE)) { return FALSE; }
    if (!i2c_write(ah)) { return FALSE; }
    if (!i2c_write(al)) { return FALSE; }
    for (i = 0; i < len; i++) {
        if (!i2c_write(buf[i])) { return FALSE; }
    }
    i2c_stop();

    // 4. Use ack polling to wait for the EEPROM to finish writing the page:
    if (ack_poll) { i2c_eeprom_ack_polling(ctl | I2C_WRITE); }
    return TRUE;
}

/**
 * Read a byte from any address (0000h - 1FFFFh) in the EEPROM.
 * @param addr Lower 16 bits of the address
 * @param high High bit that selects the block
 * @param buf Buffer to put the read byte into
 * @return True iff the random read went OK
 */
ubyte i2c_eeprom_random_read(const uint16 addr, const ubyte high, ubyte *buf)
{
    ubyte ctl, ah, al;

    // 1. Set control byte (high or low block):
    ctl = I2C_EEPROM_ADDR | (high ? I2C_24LC1026_HIGH_BLK : I2C_24LC1026_LOW_BLK);
    al = addr & 0xff;
    ah = addr >> 8;

    // 2. Read the byte from the specified block and address:
    i2c_start();
    if (!i2c_write(ctl | I2C_WRITE)) { return FALSE; }
    if (!i2c_write(ah)) { return FALSE; }
    if (!i2c_write(al)) { return FALSE; }
    i2c_restart();
    if (!i2c_write(ctl | I2C_READ)) { return FALSE; }
    if (!i2c_read(buf)) { return FALSE; }
    i2c_nack();
    i2c_stop();
    return TRUE;
}

/**
 * Perform a sequential read across one of the blocks. This function does not perform bounds
 * checking, so the read may wrap around.
 * @param addr Lower 16 bits to start the sequential read
 * @param high Block select
 * @param buf Buffer to put the read result into
 * @param len Number of bytes to read
 * @return True iff the sequential read went OK
 */
ubyte i2c_eeprom_sequence_read(const uint16 addr, const ubyte high, ubyte *buf, uint16 len)
{
    uint16 i;
    ubyte ctl, ah, al, temp;

    // 1. Set control byte (high or low block):
    ctl = I2C_EEPROM_ADDR | (high ? I2C_24LC1026_HIGH_BLK : I2C_24LC1026_LOW_BLK);
    al = addr & 0xff;
    ah = addr >> 8;

    // 2. Generate the start signal and signal a read:
    i2c_start();
    if (!i2c_write(ctl | I2C_WRITE)) { return FALSE; }
    if (!i2c_write(ah)) { return FALSE; }
    if (!i2c_write(al)) { return FALSE; }
    i2c_restart();
    if (!i2c_write(ctl | I2C_READ)) { return FALSE; }

    // 3. Keep ACKing the read bytes until the length - 1 byte:
    for (i = 0; i < (len - 1); i++) {
        if (!i2c_read(&temp)) { return FALSE; }
        buf[i] = temp;
        i2c_ack();
    }

    // 4. Read the final byte and end with a NACK:
    if (!i2c_read(&temp)) { return FALSE; }
    buf[len - 1] = temp;
    i2c_nack();
    i2c_stop();
    return TRUE;
}

/**
 * Ack polling: poll the EEPROM during its write cycle, return when it's done.
 * @param control The control byte used during the previous write (must match).
 */
void i2c_eeprom_ack_polling(ubyte ctl)
{
    ubyte write_done = FALSE;

    while (write_done == FALSE) {
        i2c_start();
        write_done = i2c_write(ctl);
    }
}

