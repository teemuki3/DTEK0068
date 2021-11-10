/*
 * File:   main.c
 * Author: teantm@utu.fi
 *
 * Created on 10 November 2021, 13:33
 * Exercise W03E01 for DTEK 0068.
 * This application makes the 7-segment display to show a countdown timer
 * which can be stopped by "cutting the red wire".
 * This version takes use of RTC timer.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <avr/sleep.h>

volatile uint8_t g_running = 1;
volatile uint8_t g_clockticks = 0;

// This interrupt occurs every 125 ms
ISR(RTC_PIT_vect)
{
    // Clear interrupt flags
    RTC.PITINTFLAGS = RTC_PI_bm;
    // Advance clockticks variable and reset it every time it reaches 8
    // (meaning that a full second has passed)
    g_clockticks = (++g_clockticks) % 8;
}

// This interrupt occurs when the red wire is cut
ISR(PORTA_PORT_vect)
{
    // Clear interrupt flags
    PORTA.INTFLAGS = PIN4_bm;
    // Halt the countdown
    g_running = 0;
}


// Modified example code from TB3213
void rtc_init(void)
{
    uint8_t temp;
    
    /* Initialize 32.768kHz Oscillator: */
    /* Disable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_ENABLE_bm;
    /* Writing to protected register */
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    
    while(CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm)
    {
        ; /* Wait until XOSC32KS becomes 0 */
    }
    
    /* SEL = 0 (Use External Crystal): */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_SEL_bm;
    /* Writing to protected register */
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    
     /* Enable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp |= CLKCTRL_ENABLE_bm;
    /* Writing to protected register */
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    
    /* Initialize RTC: */
    while (RTC.STATUS > 0)
    {
        ; /* Wait for all register to be synchronized */
    }
    
    /* 32.768kHz External Crystal Oscillator (XOSC32K) */
    RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;
    
    /* Run in debug: enabled */
    RTC.DBGCTRL = RTC_DBGRUN_bm;
    
    RTC.PITINTCTRL = RTC_PI_bm; /* Periodic Interrupt: enabled */
    
    RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc /* RTC Clock Cycles 4096 */
                 | RTC_PITEN_bm; /* Enable: enabled */
}

int main(void) 
{    
    rtc_init(); // Initialize RTC
    PORTC.DIRSET = 0xFF;  // Set all pins in port C as out  
    PORTA.DIRCLR = PIN4_bm; // Set PA4 (Red wire) as in 
    PORTF.DIRSET = PIN5_bm; // Set PF5 (Transistor control and LED) as out
    PORTF.OUTSET = PIN5_bm; // Set PF5 high
    
    // Define different led configurations for displaying numbers 0-9
    // 8 bits representing the states of 8 pins
    const uint8_t led_configurations[] =     
    {
        0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
        0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111,
    };   
    
    // Set sleep mode to idle
    SLPCTRL.CTRLA |= SLPCTRL_SMODE_IDLE_gc;   
    // For PA4: enable pull-up and trigger interrupt on rising edge
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;  
    // Enable interrupts
    sei();
                
    // Start the countdown from 10 (so that first number to be displayed is 9)
    uint8_t number = 10;
    
    while(1) 
    {   
        // Check if the countdown is still running 
        // and it's also time to advance the timer
        if (g_running && (g_clockticks == 0))
        {
            // Display the next number
            --number;
            PORTC.OUT = led_configurations[number];   
            
            // Stop the countdown when reaching zero
            if (number == 0)
            {
                g_running = 0;
            }
        }
        
        // If the countdown got to zero, blink the display
        if (number == 0)
        {
            PORTF.OUTTGL = PIN5_bm;
        }
        
        // Enter sleep until next interrupt
        sleep_mode();
    }
}