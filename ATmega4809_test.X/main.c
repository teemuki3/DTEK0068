/*
 * File:   main.c
 * Author: jasata@utu.fi
 *
 * Created on July 29, 2020, 11:16 AM
 * Simple LED blinking application for testing that the ATmega4809 PCB
 * can be programmed with the course specific virtual machine.
 */

#define F_CPU   3333333

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // Set PF5 (LED) as out
    PORTF.DIRSET = PIN5_bm;

    // The superloop
    while (1)
    {
        // Toggle LED ON/OFF every 1/2 seconds
        _delay_ms(500);
        PORTF.OUTTGL = PIN5_bm;
    }
}

