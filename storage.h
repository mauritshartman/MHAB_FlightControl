/*
 * File:   storage.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:48
 *
 * Routines for handling the external EEPROM.
 */

#ifndef STORAGE_H
#define	STORAGE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "defs.h"
#include "record.h"
#include "i2c.h"

// Defines
#define WIPE_BUFFER_SIZE (I2C_24LC1026_PAGE_SIZE / 2)
#define PAGES_PER_BLOCK (I2C_24LC1026_BLOCK_SIZE / I2C_24LC1026_PAGE_SIZE)
#define RECORDS_PER_BLOCK (I2C_24LC1026_BLOCK_SIZE / sizeof(record))
#define CHUNK_SIZE 32
#define CHUNKS_PER_BLOCK (I2C_24LC1026_BLOCK_SIZE / CHUNK_SIZE)

// Protypes
ubyte init_storage(void);
ubyte wipe_storage(ubyte c);
/* void  dump_storage(void); */
ubyte save_record(uint16 num, record *rec);
ubyte retr_record(uint16 num, record *rec);

#ifdef	__cplusplus
}
#endif

#endif	/* STORAGE_H */
