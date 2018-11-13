/* 
 * File:   radio.h
 * Author: Maurits
 *
 * Created on September 27, 2015, 9:30 PM
 */

#ifndef RADIO_H
#define	RADIO_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "defs.h"
#include "record.h"


ubyte   init_radio(void);
void    enable_radio(ubyte invert);
void    disable_radio(void);
void    rtty_send(ubyte *msg, ubyte invert);
void    rtty_tone(ubyte high);
void    send_record(record *);
//void    send_record_ukhas(record *, uint16 );


#ifdef	__cplusplus
}
#endif

#endif	/* RADIO_H */
