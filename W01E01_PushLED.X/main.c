/*
 * File:   main.c
 * Author: teantm@utu.fi
 *
 * Created on October 29, 2021, 3:20 PM
 * Exercise W01E01 for DTEK 0068.
 * This application turns on the LED of ATmega4809 when its button is pressed
 * and turns off the LED when the button is not pressed. 
 */

#include <avr/io.h>

int main(void)
{   
    PORTF.DIRSET = PIN5_bm;  // Set PF5 (LED) as out
    PORTF.DIRCLR = PIN6_bm;  // Set PF6 (Button) as in
    
    while (1) 
    {
        // If the button is not pressed, turn the LED off
        if (PORTF.IN & PIN6_bm) 
        {
            PORTF.OUTSET = PIN5_bm;
        }
        
        // And if the button is pressed, turn the LED on
        else 
        {
            PORTF.OUTCLR = PIN5_bm;
        }
    }
}
