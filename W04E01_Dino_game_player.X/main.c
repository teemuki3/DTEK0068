/*
 * File:   main.c
 * Author: teantm@utu.fi
 *
 * Created on 17 November 2021, 11:41
 * Exercise W04E01 for DTEK 0068.
 * This application plays Chrome's Dino game. An LDR is used to detect cacti 
 * and a servo is used to press the spacebar. The threshold value of the LDR 
 * can be modified with a potentiometer and is displayed in a 7-segment-display.
 */

#define RTC_PERIOD              51 // Period of ~100 ms with prescaler of 64
#define SERVO_PWM_DUTY_NEUTRAL  312 // Position of 0°
#define SERVO_PWM_DUTY_DOWN     364 // Position of 22,5°
#define SERVO_PWM_PERIOD        0x1046 // Period of ~20 ms with prescaler of 16

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

// Used to signal the superloop when the servo is ready to activate
volatile uint8_t g_servo_ready = 1; 

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
    
    /* Set period */
    RTC.PER = RTC_PERIOD;
    
    /* 32.768kHz External Crystal Oscillator (XOSC32K) */
    RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;
    
    RTC.CTRLA = RTC_PRESCALER_DIV64_gc /* Set prescaler to 64 */
                | RTC_RTCEN_bm; /* Enable: enabled */
                
    /* Enable Overflow Interrupt */
    RTC.INTCTRL |= RTC_OVF_bm;
    
}

/* Reads and returns value from ADC  */
uint16_t adc0_read(void)
{
    ADC0.COMMAND = ADC_STCONV_bm; // Start ADC conversion
    while(!(ADC0.INTFLAGS & ADC_RESRDY_bm))
    {
        ; // Wait for conversion to finish
    }
    return ADC0.RES;
}

/* Selects trimmer potentiometer as input for ADC, 
 * then reads and returns it's value  */
uint16_t trimpot_read()
{      
    ADC0.MUXPOS = ADC_MUXPOS_AIN14_gc; // Set AIN14 (PF4) as ADC input
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc; // Use VDD as reference voltage
    adc0_read(); // First value is trash and it's ignored
    return adc0_read();
}

/* Selects LDR as input for ADC, 
 * then reads and returns it's value */
uint16_t ldr_read()
{      
    ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc; // Set AIN8 (PE0) as ADC input
    ADC0.CTRLC = ADC_REFSEL_INTREF_gc; // Use internal reference voltage
    adc0_read(); // First value is trash and it's ignored
    return adc0_read();
}


int main(void) 
{     
    PORTC.DIRSET = 0xFF;  // Set all pins in port C (7-segment-display) as out 
    PORTF.DIRSET = PIN5_bm; // Set PF5 (Transistor control and LED) as out
    PORTF.OUTSET = PIN5_bm; // Set PF5 high
    
    // Define different led configurations for displaying numbers 0-9 and A
    // 8 bits representing the states of 8 pins
    const uint8_t led_configurations[] =     
    {
        0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, // 0-4
        0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111, // 5-9
        0b01110111 // A
    }; 
    
    /* ADC initialization */
    PORTE.DIRCLR = PIN0_bm; // Set PE0 (LDR) as in 
    PORTF.DIRCLR = PIN4_bm; // Set PF4 (Potentiometer) as in 
    PORTE.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc; // Disable PE0 input buffer
    PORTF.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc; // Disable PF4 input buffer    
    VREF.CTRLA = VREF_ADC0REFSEL_2V5_gc; // Use 2,5V internal reference
    ADC0.CTRLC = ADC_PRESC_DIV16_gc; // Set ADC0 prescaler to 16
    ADC0.CTRLA = ADC_ENABLE_bm; // Enable ADC using default 10-bit resolution
    
    /* Servo initialization */
    PORTB.DIRSET = PIN2_bm; // Set PB2 (Servo) as out
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTB_gc; // Waveform output on port B
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc; // Set TCA0 prescaler to 16
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc; // Single-slope mode
    TCA0.SINGLE.CMP2BUF = SERVO_PWM_DUTY_NEUTRAL; // Set start position of 0°
    TCA0.SINGLE.PERBUF = SERVO_PWM_PERIOD; // Set PWM period of 20ms
    TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm; // Enable compare channel 2
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm; // Enable TCA0   
    
    /* RTC initialization */
    rtc_init();
    
    sei(); // Enable interrupts
            
    uint16_t threshold_value;
    uint8_t servo_pos = 0; // Value 0 means servo is in initial position
    
    while (1) 
    {
        // Read threshold value from the potentiometer 
        // and display its hundreds in the 7-segment-display
        threshold_value = trimpot_read();
        PORTC.OUT = led_configurations[threshold_value / 100];
        
        // Only give commands to the servo if it is in ready state
        if (g_servo_ready)
        {
            // Return the servo to its initial position if it's not there...
            if (servo_pos != 0)
            {
                TCA0.SINGLE.CMP2BUF = SERVO_PWM_DUTY_NEUTRAL;
                servo_pos = 0;
                RTC.CNT = 0; 
                g_servo_ready = 0;
            }
            
            // ... otherwise make it press spacebar if the LDR reading is 
            // higher than the threshold value
            else if (ldr_read() > threshold_value)
            {
                TCA0.SINGLE.CMP2BUF = SERVO_PWM_DUTY_DOWN;
                servo_pos = 1;
                RTC.CNT = 0;
                g_servo_ready = 0;
            }
            /* In both cases the RTC count is started from zero
             * and servo is put in non-ready state until the count finishes 
             * 100 ms after and triggers RTC interrupt. 
             * Servo_pos variable is also updated accordingly. */
        }        
    }
}

// This interrupt occurs every time RTC count finishes (taking 100 ms from 0)
ISR(RTC_CNT_vect)
{
    RTC.INTFLAGS = RTC_OVF_bm; // Clear interrupt flags
    g_servo_ready = 1; // Give signal that the servo is ready again
}