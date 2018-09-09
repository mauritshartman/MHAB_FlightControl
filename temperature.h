/*
 * File:   temperature.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:36
 *
 * Routines for handling temperature measurements.
 */

#ifndef TEMPERATURE_H
#define	TEMPERATURE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"

// Function prototypes
ubyte   init_temperature(void);
sint16  get_internal_temp(void);
sint16  get_external_temp(void);


#ifdef	__cplusplus
}
#endif

#endif	/* TEMPERATURE_H */
