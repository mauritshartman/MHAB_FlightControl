/*
 * File:   analog_pressure.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:41
 *
 * Routines for handling the analog pressure sensor.
 */

#ifndef ANALOG_PRESSURE_H
#define	ANALOG_PRESSURE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"

ubyte init_analog_pressure(void);
uint24 read_analog_pressure(void);
void close_analog_pressure(void);



#ifdef	__cplusplus
}
#endif

#endif	/* ANALOG_PRESSURE_H */

