/*
 * File:   adc.c
 * Provides functions for ADC usage.
 */

#include <avr/io.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "adc.h"

SemaphoreHandle_t mutex; 

void adc_init(void)
{
    mutex = xSemaphoreCreateMutex(); // Create mutex
    
    PORTE.DIRCLR = PIN0_bm; // Set PE0 (LDR) as in 
    PORTE.DIRCLR = PIN1_bm; // Set PE0 (NTC-Thermistor) as in 
    PORTF.DIRCLR = PIN4_bm; // Set PF4 (Potentiometer) as in
    PORTE.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc; // Disable PE0 input buffer
    PORTE.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc; // Disable PE1 input buffer
    PORTF.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc; // Disable PF4 input buffer
    VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc; // Use 2,5V internal reference
    ADC0.CTRLC |= ADC_PRESC_DIV16_gc; // Set ADC0 prescaler to 16
    ADC0.CTRLA |= ADC_ENABLE_bm; // Enable ADC using default 10-bit resolution
}

uint16_t adc_converse(void)
{
    ADC0.COMMAND = ADC_STCONV_bm; // Start ADC conversion
    while(!(ADC0.INTFLAGS & ADC_RESRDY_bm))
    {
        ; // Wait for conversion to finish
    }
    return ADC0.RES;
}

uint16_t adc_read(uint8_t input)
{   
    // Wait until mutex is available and take it
    xSemaphoreTake(mutex, portMAX_DELAY);
    
    // Select correct ADC input and reference voltage
    switch (input)
    {
        case LDR:
            ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc; // Set AIN8 (PE0) as ADC input
            ADC0.CTRLC |= ADC_REFSEL_INTREF_gc; // Internal reference voltage
            break;  
            
        case NTC:       
            ADC0.MUXPOS = ADC_MUXPOS_AIN9_gc; // Set AIN14 (PE1) as ADC input
            ADC0.CTRLC |= ADC_REFSEL_INTREF_gc; // VDD as reference voltage
            break;     
            
        case POT:
            ADC0.MUXPOS = ADC_MUXPOS_AIN14_gc; // Set AIN14 (PF4) as ADC input
            ADC0.CTRLC |= ADC_REFSEL_VDDREF_gc; // VDD as reference voltage      
    }   
    
    // Do ADC conversion two times and save the latter result
    adc_converse();
    uint16_t final_result = adc_converse();
    
    // Make mutex available again before returning
    xSemaphoreGive(mutex); 
    return final_result; 
}
