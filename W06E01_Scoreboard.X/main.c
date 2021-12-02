/*
 * File:   main.c
 * Author: teantm@utu.fi
 *
 * Created on November 30, 2021, 2:47 PM
 * Exercise W06E01 for DTEK 0068.
 * This application receives input from serial terminal via USART, 
 * checks if inputted character was a valid number, 
 * then displays either that number or the letter E in a 7-segment-display
 * and sends a message string via USART back to the serial terminal informing
 * the user whether the input was valid or not.
 * Implemented with FreeRTOS.
 */

#define F_CPU 3333333
#define USART0_BAUD_RATE(BAUD_RATE)\
    ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include "FreeRTOS.h"
#include "clock_config.h"
#include "queue.h"
#include "task.h"
#include "string.h"

// Define different led configurations for displaying numbers 0-9 and letter E
// 8 bits representing the states of 8 pins
const uint8_t led_configurations[] =     
{
    0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, // 0-4
    0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111, // 5-9
    0b01111001 // Error symbol E
}; 

QueueHandle_t queue_A; // Carries data to task usart_send
QueueHandle_t queue_B; // Carries data to task display_score

// Task used to read incoming characters via USART RX
void usart_receive(void* parameter)
{
    uint8_t digit;
    while (1) 
    {
        // Wait until data is available
        while (!(USART0.STATUS & USART_RXCIF_bm))
        {
            ;
        }   
        // Receive the next character and convert it to matching integer value 
        // (or some other value over 9 if it was not a number)
        digit = USART0.RXDATAL - '0';
        // Use value 10 to represent any invalid character 
        if (digit > 9)
        {
            digit = 10;
        }
        // Finally send the digit to other tasks via queues
        xQueueSend(queue_A, (void *)&digit, 10);
        xQueueSend(queue_B, (void *)&digit, 10);
    }
}

// Task used to write back messages to user via USART TX
void usart_send(void* parameter)
{
    uint8_t digit;
    while (1) 
    {
        // Receive the next number from queue A if it's not empty
        if (xQueueReceive(queue_A, (void *)&digit, 0) == pdPASS)
        {
            // Select which message is sent back to the user's terminal: 
            // Was the entered character valid or not?
            char* msg_string = (digit == 10) ? 
                "Error! Not a valid digit.\r\n" : 
                "Number received!\r\n";

            // Iterate through the message string and send each character 
            // one at a time
            for (size_t i = 0; i < strlen(msg_string); i++)
            {
                // Wait until data can be sent
                while (!(USART0.STATUS & USART_DREIF_bm))
                {
                    ;
                }
                USART0.TXDATAL = msg_string[i];
            } 
        }
    }
}

// Task used to display score in 7-segment-display
void display_score(void* parameter)
{
    /* Display initialization */
    PORTC.DIRSET = 0xFF; // All pins in port C as out (7-segment-display)
    PORTF.DIRSET = PIN5_bm; // PF5 out (transistor controlling the display)
    PORTF.OUTSET = PIN5_bm; // PF5 high
    
    uint8_t digit;  
    while (1)
    {
        // Receive the next number from queue B if it is not empty
        if (xQueueReceive(queue_B, (void *)&digit, 0) == pdPASS)
        {
           // Display the number or E if it's value was 10 (invalid)
            PORTC.OUT = led_configurations[digit]; 
        }
    }
}

int main(void)
{
    /* USART initialization */
    PORTA.DIRSET = PIN0_bm; // PA0 out
    PORTA.DIRCLR = PIN1_bm; // PA1 in
    USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);
    USART0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
    
    /* Queue creation */
    queue_A = xQueueCreate(10, sizeof(uint8_t));
    queue_B = xQueueCreate(10, sizeof(uint8_t));
            
    /* Task creation */
    xTaskCreate(
            usart_receive,
            "usart_receive",
            configMINIMAL_STACK_SIZE,
            NULL,
            tskIDLE_PRIORITY,
            NULL
    );
    
    xTaskCreate(
            usart_send,
            "usart_send",
            configMINIMAL_STACK_SIZE,
            NULL,
            tskIDLE_PRIORITY,
            NULL
    );
    
    xTaskCreate(
            display_score,
            "display_score",
            configMINIMAL_STACK_SIZE,
            NULL,
            tskIDLE_PRIORITY,
            NULL
    );
    
    // Start...
    vTaskStartScheduler();
    return 0;
}
