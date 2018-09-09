/*
 * File:   gsm.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:25
 *
 * Routines for handling the GSM modem.
 */

#ifndef GSM_H
#define	GSM_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "defs.h"
#include "record.h"

#define GSM_MODEM_BUFSIZE   48
#define CTRLZ               0x1a

ubyte   init_gsm(void);
void    enable_gsm(void);
void    disable_gsm(void);
void    send_sms(ubyte *);
ubyte   sms_ready(void);
void    send_sms_record(record *);


#ifdef	__cplusplus
}
#endif

#endif	/* GSM_H */
