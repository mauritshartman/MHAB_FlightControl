/*
 * File:   command.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:13
 *
 * Routines and definitions for the command interpreter for when the device is in COMMAND mode.
 */

#ifndef COMMAND_H
#define	COMMAND_H

#ifdef	__cplusplus
extern "C" {
#endif

// The main command loop that repeatedly prompts the user for input:
void command_loop(void);


#ifdef	__cplusplus
}
#endif

#endif	/* COMMAND_H */
