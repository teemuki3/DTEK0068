/*
 * File:   dummy.c
 * Function for toggling the on-board LED based on if NTC reading is higher than 
 * potentiometer reading or off if opposite. 
 * Also notifies the backlight when potentiometer is used.
 */

#define DIFFERENCE(a,b)     ((a > b) ? (a - b) : (b - a))

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"
#include "main.h"

void dummy(void* parameter)
{
    uint16_t ntc_reading;
    uint16_t pot_reading;
    uint16_t prev_pot_reading = 0;
    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS);
    while (1)
    {    
        ntc_reading = adc_read(NTC);
        pot_reading = adc_read(POT);
        
        // If NTC reading was higher than potentiometer reading, turn on the LED 
        if (ntc_reading > pot_reading)
        {
            PORTF.OUTCLR = PIN5_bm; 
        } 
        // ...or turn the LED off if it wasn't
        else
        {
            PORTF.OUTSET = PIN5_bm;
        }
        
        // Additionally, if potentiometer reading was changed by at least 1 %,
        // notify backlight_control task that user interaction has occurred
        // Very minor changes may occur randomly without any interaction 
        // but this comparison  ignores them
        if (DIFFERENCE(pot_reading, prev_pot_reading) >= 10)
        {
            xTaskNotifyGive(bl_ctrl_handle);  
            prev_pot_reading = pot_reading;  // Update prev_pot_reading
        }
         
        // Wait 100 ms before reading the values again
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
