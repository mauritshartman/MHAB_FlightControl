/*
 * File:   digital_pressure.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:43
 *
 * Routines for handling the BMP180 pressure sensor.
 */

#include "digital_pressure.h"

#include "i2c.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>


// Function prototypes
static uint16 read_coeff(ubyte reg_addr_h, ubyte reg_addr_l);
static sint32 read_sensor(ubyte pressure_reading, ubyte oss);


ubyte init_bmp180_pressure(void)
{
    sint32 dummy;

    // Per datasheet, check data communication by reading address 0
    dummy = read_sensor(TRUE, 0);
    if (dummy == LONG_MAX) {
        printf("\r\nError initializing digital barometer\r\n");
        return FALSE;
    }
    printf("DIGI_BARO ");
    return TRUE;
}


static uint16 read_coeff(ubyte reg_addr_h, ubyte reg_addr_l)
{
    uint16 result = 0;
    ubyte msb, lsb;

    // 1. Read out the MSB of the coefficient:
    i2c_start();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_WRITE))    { return USHRT_MAX; }
    if (!i2c_write(reg_addr_h))                     { return USHRT_MAX; }
    i2c_restart();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_READ))     { return USHRT_MAX; }
    if (!i2c_read(&msb))                            { return USHRT_MAX; }
    i2c_nack();
    i2c_stop();

    // 2. Read out the LSB of the coefficient:
    i2c_start();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_WRITE))    { return USHRT_MAX; }
    if (!i2c_write(reg_addr_l))                     { return USHRT_MAX; }
    i2c_restart();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_READ))     { return USHRT_MAX; }
    if (!i2c_read(&lsb))                            { return USHRT_MAX; }
    i2c_nack();
    i2c_stop();

    // Process and return the results:
    result = ((uint16)msb << 8) | (uint16)lsb;
    return result;
}


static sint32 read_sensor(ubyte pressure_reading, ubyte oss)
{
    sint32 result = 0;
    ubyte msb = 0, lsb = 0, xlsb = 0, val;

    // 0. Determine the correct value for entering at the F4 address:
    if (pressure_reading) { val = 0x34 + (oss << 6); }
    else { val = 0x2e; }

    // 1. Start temperature or pressure measurement:
    i2c_start();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_WRITE))    { return LONG_MAX; }
    if (!i2c_write(0xF4))                           { return LONG_MAX; }
    if (!i2c_write(val))                            { return LONG_MAX; }
    i2c_stop();
    __delay_ms(20); __delay_ms(20);  // Delay for 40ms to wait for conversion (we use maximum oversampling)

    // 2. Read out the results (always registers 0xF6 and 0xF7, optionally 0xF8 for pressure):
    i2c_start();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_WRITE))    { return LONG_MAX; }
    if (!i2c_write(0xF6))                           { return LONG_MAX; }
    i2c_restart();
    if (!i2c_write(I2C_BMP180_ADDR | I2C_READ))     { return LONG_MAX; }
    // Read and ack the MSB and LSB:
    if (!i2c_read(&msb))                            { return LONG_MAX; }
    i2c_ack();
    if (!i2c_read(&lsb))                            { return LONG_MAX; }
    // Optionally read and ack the XLSB:
    if (pressure_reading) {
        i2c_ack();
        if (!i2c_read(&xlsb))                       { return LONG_MAX; }
    }
    i2c_nack();
    i2c_stop();

    // Process and return the results:
    if (pressure_reading) {
        result = (((sint32)msb << 16) | ((sint32)lsb << 8) | (sint32)xlsb) >> (8 - oss);
    }
    else { // Temperature reading
        result = ((sint32)msb << 8) | (sint32)lsb;
    }
    return result;
}


ubyte read_bmp180_coefficients(bmp180_coeff *coeff)
{
    memset(coeff, '\0', sizeof(coeff));

    // Read out calibration coefficients:
    coeff->ac1 = (sint16)read_coeff(0xaa, 0xab);
    coeff->ac2 = (sint16)read_coeff(0xac, 0xad);
    coeff->ac3 = (sint16)read_coeff(0xae, 0xaf);
    coeff->ac4 =         read_coeff(0xb0, 0xb1);
    coeff->ac5 =         read_coeff(0xb2, 0xb3);
    coeff->ac6 =         read_coeff(0xb4, 0xb5);
    coeff->b1 =  (sint16)read_coeff(0xb6, 0xb7);
    coeff->b2 =  (sint16)read_coeff(0xb8, 0xb9);
    coeff->mb =  (sint16)read_coeff(0xba, 0xbb);
    coeff->mc =  (sint16)read_coeff(0xbc, 0xbd);
    coeff->md =  (sint16)read_coeff(0xbe, 0xbf);

    return TRUE;
}


sint16 read_bmp180_temperature(void)
{
    bmp180_coeff coeff;
    sint32 ut, tt, x1, x2, b5;
    sint16 ret;

    // 1. Retrieve coefficients and uncompensated temperature and pressure:
    read_bmp180_coefficients(&coeff);   // TODO: error checking on coefficient readout
    ut = read_sensor(BMP180_TEMPERATURE, BMP180_ULTRA_HIGH);
    if (ut == LONG_MAX) { return SHRT_MAX; }

    // 2. Calculate true temperature:
    x1 = (ut - coeff.ac6) * coeff.ac5 / 32768;
    x2 = (sint32)coeff.mc * 2048 / (x1 + (sint32)coeff.md);
    b5 = x1 + x2;
    tt = (b5 + 8) / 16; // Temperature in 0.1C

    // 3. TT, even though 32 bit, should not be outside range of signed 16 bit:
    if (tt >= 0) {
        ret = (sint16)(0x0000ffff & tt);
    }
    else {  // Negative temp
        tt *= -1;
        ret = (sint16)(0x0000ffff & tt);
        ret *= -1;
    }
    return ret;
}


uint24 read_bmp180_pressure(void)
{
    bmp180_coeff coeff;
    sint32 ut, tt, tp, up, x1, x2, x3, b3, b5, b6, temp;
    uint32 b4, b7;

    // 1. Retrieve coefficients and uncompensated temperature and pressure:
    read_bmp180_coefficients(&coeff);
    ut = read_sensor(BMP180_TEMPERATURE, BMP180_ULTRA_HIGH);
    up = read_sensor(BMP180_PRESSURE,    BMP180_ULTRA_HIGH);

    // 2. Calculate true temperature:
    x1 = (ut - coeff.ac6) * coeff.ac5 / 32768;
    x2 = (sint32)coeff.mc * 2048 / (x1 + (sint32)coeff.md);
    b5 = x1 + x2;
    tt = (b5 + 8) / 16; // Temperature in 0.1C

    // 5. Calculate true pressure (page 13, BMP180 datasheet):
    b6 = b5 - 4000;

    x1 = ((sint32)coeff.b2 * ((b6 * b6) / 4096)) / 2048;
    x2 = ((sint32)coeff.ac2 * b6) / 2048;
    x3 = x1 + x2;
    temp = (sint32)coeff.ac1 * 4;
    temp += x3;
    temp = temp << BMP180_ULTRA_HIGH;
    temp += 2;
    b3 = temp / 4;

    x1 = ((sint32)coeff.ac3 * b6) / 8192;
    x2 = ((sint32)coeff.b1 * ((b6 * b6) / 4096)) / 65536;
    x3 = ((x1 + x2) + 2) / 4;
    b4 = (coeff.ac4 * (uint32)(x3 + 32768)) / 32768;
    b7 = (uint32)(up - b3) * (50000 >> BMP180_ULTRA_HIGH);

    tp = (b7 < 0x80000000) ? tp = (b7 * 2) / b4 : (b7 / b4) * 2;
    x1 = (tp / 256) * (tp / 256);
    temp = x1 * 3038;
    x1 = temp / 65536;
    temp = -7357 * tp;
    x2 = temp / 65536;
    temp = x1 + x2 + 3791;
    tp += temp / 16;

    return (uint24)tp;
}

