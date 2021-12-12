/*
 * File:   backlight.c
 * Functions for controlling the LCD backlight using TCB3 8-bit PWM.
 */

#define TCB_CMP_INITIAL_VALUE               0x00FF // off

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"
#include "main.h"

void backlight_init(void)
{
    PORTB.DIRSET = PIN5_bm; // PB5 out    
    
    /* TCB3 initialization */
    TCB3.CCMP = TCB_CMP_INITIAL_VALUE; // Write default value to CCMP register
    TCB3.CTRLB |= TCB_CNTMODE_PWM8_gc; // Configure TCB in 8-bit PWM mode  
    TCB3.CTRLB |= TCB_CCMPEN_bm; // Enable Pin Output
    TCB3.CTRLA |= TCB_CLKSEL_CLKDIV1_gc; // Use CLK_PER
    TCB3.CTRLA |= TCB_ENABLE_bm;  // Enable TCB3
}

void backlight_auto_adjust(void* parameter)
{        
    uint8_t brightness;
    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS);
    while (1) 
    {
        // Take LDR reading and scale down its value range from 0-1023 to 0-255
        brightness = adc_read(LDR) / 4;
        TCB3.CCMPH = brightness; // Set new brightness level    
        // Wait 75 ms before adjusting brightness again
        vTaskDelay(75 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void backlight_control(void* parameter)
{    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS);
    while (1)
    {
        // Wait up to 10 seconds if this task gets notified from dummy task 
        // because of potentiometer interaction... 
        
        // If 10 seconds pass without interaction, suspend backlight 
        // adjusting task and set the backlight off
        if (ulTaskNotifyTake(pdTRUE, 10000 / portTICK_PERIOD_MS) == pdFALSE)           
        {
            vTaskSuspend(bl_adj_handle);
            TCB3.CCMPH = 0x00; // off
        }
        // Whenever interaction occurs, make backlight available again by 
        // resuming the backlight adjusting task
        else
        {
            vTaskResume(bl_adj_handle);
        }
    }
    vTaskDelete(NULL);
}