/*
 * File:   main.c
 * Author: teantm@utu.fi
 *
 * Created on December 8, 2021, 1:19 PM
 * Exercise W07E01 for DTEK 0068.
 * This application shows readings from NTC-thermistor, LDR and a potentiometer
 * in textual format on the first line of an LCD display. The second line shows
 * manufacturer text string scrolling from side to side. The LCD features a 
 * backlight which automatically adjusts its brightness depending on changes in
 * light level and also turns itself off after user inactivity.
 * Implemented with FreeRTOS.
 */

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "adc.h"
#include "backlight.h"
#include "dummy.h"
#include "lcd.h"
#include "uart.h"

TaskHandle_t bl_ctrl_handle;
TaskHandle_t bl_adj_handle;

int main(void)
{    
    PORTF.OUTSET = PIN5_bm; // Set PF5 high (onboard LED off)
    PORTF.DIRSET = PIN5_bm; // Set PF5 as out
    
    // Initialization code of UART, ADC, LCD backlight and message queue
    uart_init();
    adc_init(); 
    backlight_init();
    lcd_msg_queue_init();
    
    /* Task creation */       
    xTaskCreate(
        backlight_auto_adjust, "bl_adj", configMINIMAL_STACK_SIZE, NULL, 
        tskIDLE_PRIORITY, &bl_adj_handle);    
    
    xTaskCreate(
        dummy, "dummy", configMINIMAL_STACK_SIZE, NULL, 
        configMAX_PRIORITIES - 1, NULL); // Highest priority    
    
    xTaskCreate(
        backlight_control, "bl_ctrl", configMINIMAL_STACK_SIZE, NULL, 
        tskIDLE_PRIORITY, &bl_ctrl_handle);
    
    xTaskCreate(
        lcd_control, "lcd_ctrl", configMINIMAL_STACK_SIZE, NULL, 
        tskIDLE_PRIORITY, NULL);
    
    xTaskCreate(
        lcd_scrolling_text, "lcd_scrl", configMINIMAL_STACK_SIZE, NULL, 
        tskIDLE_PRIORITY, NULL);
    
    xTaskCreate( 
        lcd_adc_report, "lcd_adc", configMINIMAL_STACK_SIZE + 50, NULL, 
        tskIDLE_PRIORITY, NULL); // This task needs a bigger stack
    
    xTaskCreate(
        uart_send_reports, "uart", configMINIMAL_STACK_SIZE + 50, NULL, 
        tskIDLE_PRIORITY, NULL); // This too
    
    // Start...
    vTaskStartScheduler();
    while (1)
    {
        ; // "Never should have come here."
    }
}
