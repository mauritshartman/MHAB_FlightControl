/*
 * File:   parachute.h
 * Author: Maurits
 *
 * Created on 26 september 2012, 22:18
 *
 * Code for releasing the parachute.
 */

#include "parachute.h"
#include "record.h"
#include "storage.h"
#include "util.h"

void deploy_parachute(void)
{
	// Set the output pin high to blow the charges:
	PARACHUTE_DIR = OUTPUT;
	PARACHUTE_PIN = HIGH;

	// Wait four seconds:
    delay_1sec(); delay_1sec(); delay_1sec(); delay_1sec();

	// Set the output pin low:
	PARACHUTE_PIN = LOW;
    PARACHUTE_DIR = INPUT;

	global_config.status.chute_deployed = 1;
    
    // Make sure it gets saved in the EEPROM:
    save_record(0, &global_config);
}

