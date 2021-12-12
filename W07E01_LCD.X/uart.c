/*
 * File:   uart.c
 * Functions for UART usage.
 */

#define F_CPU 3333333
#define USART0_BAUD_RATE(BAUD_RATE)\
    ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include "string.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"


void uart_init(void)
{
    PORTA.DIRSET = PIN0_bm; // PA0 out
    USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600); // Use baud rate of 9600
    USART0.CTRLB |= USART_TXEN_bm;  // Enable USART write mode
}

void uart_send_reports(void* parameter)
{
    char msg_string[60];    
    uint16_t ldr_reading;
    uint16_t ntc_reading;
    uint16_t pot_reading;
    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS);
    while (1) 
    { 
        ldr_reading = adc_read(LDR);
        ntc_reading = adc_read(NTC);
        pot_reading = adc_read(POT);
        
        // Put these readings into one message string
        sprintf(msg_string, 
            "LDR Value: %u\r\nNTC Value: %u\r\nPOT Value: %u\r\n\n", 
            ldr_reading, ntc_reading, pot_reading);

        // Iterate through the message string and send each character 
        // one at a time
        for (size_t i = 0; i < strlen(msg_string); i++)
            {
            while (!(USART0.STATUS & USART_DREIF_bm))
            {
                ; // Wait until data can be sent
            }
            USART0.TXDATAL = msg_string[i];
        }
        
        // Wait 1 second before sending the next report
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
