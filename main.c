/*
 * File:   main.c
 * Author: Maurits
 *
 * Created on 26 september 2012, 21:49
 */

#include "config.h"
#include "defs.h"
#include "flight.h"
#include "init.h"
#include "record.h"
#include "command.h"
#include "util.h"
#include "storage.h"
#include "serial.h"
#include "digital_pressure.h"
#include "temperature.h"

#include <stdio.h>


// Global variable:
record global_config;

/*
 * Entry point of the balloon controller. Init code and start control loop.
 */
void main(void) {
    // Initialize all peripherals and storage. Retrieve last config as well.
    init();

    // Depending on the last saved mode, determine what to do:
    if (global_config.ru.config.mode == MODE_COMMAND) {
        printf("In command mode\r\n");
        command_loop();     // Continue with command mode:
    }
    else {
        // Before continuing with flight mode, give the user two seconds to enter command mode:
        printf("Mode %u. Press 'c' to enter command mode.\r\n", global_config.ru.config.mode);
        delay_1sec(); delay_1sec();

        if (data_rdy_uart()) {
            if (getc_uart() == 'c') {
                global_config.ru.config.mode = MODE_COMMAND;
                save_record(0, &global_config);
                Reset();
            }
        }
    }

    // Otherwise run the flight routine:
    flight_control();

    // Perform NOP's for the rest of the routine (watchdog will wake up CPU and start main() again).
    // Somehow a Sleep instruction reset the WDT and doubles the time (30 sec for flight routine, 33 sec WDT again)
    while (TRUE) { Nop(); }
}

