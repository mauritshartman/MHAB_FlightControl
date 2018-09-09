/*
 * File:   digital_pressure.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:43
 *
 * Routines for handling the BMP180 pressure sensor.
 */

#ifndef DIGITAL_PRESSURE_H
#define	DIGITAL_PRESSURE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"

#define BMP180_TEMPERATURE      0
#define BMP180_PRESSURE         1
#define BMP180_ULTRA_LOW_PWR    0
#define BMP180_STANDARD         1
#define BMP180_HIGH_RESOLUTION  2
#define BMP180_ULTRA_HIGH       3

// See BMP180 documentation for an explanation on these coefficients:
typedef struct {
    sint16 ac1, ac2, ac3, b1, b2, mb, mc, md;
    uint16 ac4, ac5, ac6;
} bmp180_coeff;

ubyte   init_bmp180_pressure(void);
uint24  read_bmp180_pressure(void);             // Return pressure in Pa
sint16  read_bmp180_temperature(void);
ubyte   read_bmp180_coefficients(bmp180_coeff *coeff);

#ifdef	__cplusplus
}
#endif

#endif	/* DIGITAL_PRESSURE_H */

