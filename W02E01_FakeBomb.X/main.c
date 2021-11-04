/*
 * File:   main.c
 * Author: teantm@utu.fi
 *
 * Created on November 3, 2021, 2:34 PM
 * Exercise W02E01 for DTEK 0068.
 * This application makes the 7-segment display to show a countdown timer
 * which can be stopped by "cutting the red wire"
 */

#define F_CPU   3333333

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

int g_running = 1;

ISR(PORTA_PORT_vect)
{
    // Clear interrupt flags
    PORTA.INTFLAGS = PIN4_bm;
    // Halt the countdown
    g_running = 0;
}

int main(void) 
{    
    PORTC.DIRSET = 0xFF;  // Set all pins in port C as out  
    PORTA.DIRCLR = PIN4_bm; // Set PA4 (Red wire) as in 
    
    // Define different led configurations for displaying numbers 0-9
    // 8 bits representing the states of 8 pins
    uint8_t led_configurations[] =     
    {
        0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
        0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111,
    };
    
    // For PA4: enable pull-up and trigger interrupt on rising edge
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;  
    // Enable interrupts
    sei();
                
    // Start the countdown from 10 (so that first number to be displayed is 9)
    int number = 10;
    
    while(1) 
    {   
        // Check if the countdown is still running
        if (g_running)
        {
            // Display the next number
            --number;
            PORTC.OUT = led_configurations[number];     
            
            // Wait one second before displaying the next number...
            if (number > 0)
            {
                _delay_ms(1000);
            }        
            // ...or stop the countdown when reaching zero
            else 
            {
                g_running = 0;
            }      
        }    
        
        // If the countdown got to zero, blink the display
        if (number == 0)
        {
            _delay_ms(333);
            PORTC.OUTTGL = led_configurations[0];
        } 
    }
}
