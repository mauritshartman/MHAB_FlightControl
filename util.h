/*
 * File:   util.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:03
 */

#ifndef UTIL_H
#define	UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"

void alt_gets(ubyte *buf, ubyte buf_size);
void alt_gets_no_echo(ubyte *buf, ubyte buf_size);
void delay_1sec(void);


#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */


