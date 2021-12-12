/*
 * File:    lcd.c
 *          Minimal 1602 LCD driver (8-bit bus), FreeRTOS version
 * Author:  Jani Tammi <jasata@utu.fi>
 * Created: 13 November 2021, 12:31
 * Device:  ATmega4809 Curiosity Nano
 *
 ******************************************************************************
 *
 * NOTE: Older ST7066 (without the suffix 'U') has different delays
 *       and different initialisation (FOUR function set commands).
 *       ...however, WinTek WD-C1602MB data sheet distinctly states
 *       that it is built with the newer ST7066U chip.
 *
 *  Pin     Fnc     MCU     Description
 * ----------------------------------------------------------------------------
 *   1      GND
 *   2      Vcc             Logic power supply
 *   3      V0              LCD drive / contrast
 *   4      RS      PB4     Register Select (0 = instruction, 1 = data)
 *   5      RW      GND     Read/Write      (0 = write,       1 = read)
 *   6      ENABLE  PB3     Enable          (0 = disabled,    1 = process input)
 *   7-14   DATA    PD[0:7] Data lines
 *
 * Apparently, ENABLE line is supposed to be pulsed when the device is
 * expected to process input. Curiously, it will function when constantly
 * set as HIGH.
 * 
 * This implementation uses <util/delay.h> for microsecond delays. If this code
 * is to be used in an application that requires accurate timing of few
 * microseconds, this code should be rewritten.
 */

/******************************************************************************
 * PIN CONFIGURATION
 *****************************************************************************/
#define F_CPU                   3333333
// Control lines in PORTB
#define LCD_E_PIN               PIN3_bm
#define LCD_RS_PIN              PIN4_bm
// CPS = Characters per second
#define SCROLL_SPEED_CPS        5
#define MANUFACTURER_TEXT       " DTEK0068 Embedded Microprocessor Systems "

#include <avr/io.h>
#include <util/delay.h>
// FreeRTOS
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "string.h"
#include "stdio.h"
#include "lcd.h"
#include "adc.h"

/*
 * LCD_CMD_DELAY - Delay between LCD commands
 *      Documentation claims 37 us (verified to be too short) some state
 *      up to 43 us. Value of 40 microseconds appears to work reliably.
 * LCD_ENABLE_PULSE_DELAY - Length of Enable duty
 *      According to ST7066U datasheet, E pulse (Tpw) is at minimum 460 ns.
 *      Enable cycle time (Tc, time between E rising edges) must be >= 1200 ns.
 *          1/3,33 MHz is 0,3 us (-> 2x NOP) = 600 ns
 *          1/20 MHz is 50 ns    (-> 9x NOP) = 450 ns
 *      If timing has issues, the asm directive could be replaced with:
 *          asm volatile("nop\n\tnop\n\t"::)  ==>  _delay_us(1)
 *      It will also adapt to different F_CPU values
 */
#define LCD_CMD_DELAY()                 _delay_us(40)
#define LCD_ENABLE_PULSE_DELAY()        _delay_us(1)
#define LCD_ENABLE_PULSE()      \
{                               \
    taskENTER_CRITICAL();       \
    VPORTB.OUT |= LCD_E_PIN;    \
    LCD_ENABLE_PULSE_DELAY();   \
    VPORTB.OUT &= ~LCD_E_PIN;   \
    taskEXIT_CRITICAL();        \
}
#define LCD_CMD_SEND(byte)      \
{                               \
    VPORTB.OUT &= ~LCD_RS_PIN;  \
    VPORTD.OUT = (byte);        \
    LCD_ENABLE_PULSE();         \
    LCD_CMD_DELAY();            \
}
#define LCD_DATA_SEND(byte)     \
{                               \
    VPORTB.OUT |= LCD_RS_PIN;   \
    VPORTD.OUT = (byte);        \
    LCD_ENABLE_PULSE();         \
    LCD_CMD_DELAY();            \
}

// Queue used for sending data to display on the LCD
QueueHandle_t lcd_msg_queue;

// Message format used in lcd_msg_queue: line number and text content
struct LCD_message
{
    uint8_t line_num;
    char text[17];
};

/******************************************************************************
 * Public functions
 *****************************************************************************/
void lcd_write(char *str)
{
    while (*str)
    {
        LCD_DATA_SEND(*str++);
    }
}

// Zero indexed positions
// x        Line (even = line 0, odd = line 1)
// y        Character position (0x00 ... 0x0F)
void lcd_cursor_set(uint8_t x, uint8_t y)
{
    // Set DDRAM address command is 0b1aaaaaaa (0x80)
    // Where 'a' is an address bit (7-bit address value).
    // For 16x2, this is:
    //      0x00 - 0x15 (line 1)
    //      0x40 - 0x55 (line 2)
    //
    // What this REALLY means is:
    // Bit 7        Set DDRAM address command bit (= 1)
    // Bit 6        Line bit (0 = line 1, 1 = line 2)
    // Bit [5:4]    Undefined (keep as zero)
    // Bits [3:0]   Char position (0x00 - 0x15)

    // Cap at 0x0F
    y = y > 0x0F ? 0x0F : y;
    // Shift odd/even (line #) to line bit position (bit 6)
    LCD_CMD_SEND(0x80 | ((x & 0x01) << 6) | y);
}


void lcd_clear(void)
{
    // Send "clear screen" command
    LCD_CMD_SEND(0b00000001);
    // Wait until clear is completed (>1,52 ms)
    vTaskDelay(pdMS_TO_TICKS(2));
}

/*
 * LCD initialisation
 * 
 *      ST7066U initialization (datasheet v2.0, 2001/3/01)
 *      HD44780, KS0066, and SED1278 pin-compatible.
 *
 *      According to page 19 of the datasheet:
 *      Control + Data as [RS,RW] [D7...D0]
 * 
 */
void lcd_init(void)
{
    // Control      lines
    //  LCD_E_PIN     E       
    //  -             RW      Read/Write (0 = Write, 1 = Read)
    //  LCD_RS_PIN    RS      Register select
    // Note: RW is permanently grounded - this implementation does not read.
    PORTB.DIRSET = (LCD_E_PIN | LCD_RS_PIN);
    
    // Set PORTD as out
    PORTD.DIRSET = 0xFF;
    /*
     * Display will be busy for 40 ms after Vcc has stabilized > 4.5 V
     */
    vTaskDelay(pdMS_TO_TICKS(100));

    /*
     * 1) Function set
     *      [00] [0011NFxx]
     *      bit 4   Number of datalines.        0 = 4-bit, 1 = 8-bit
     *      N       Number of display lines.    0 = 1 line, 1 = 2 lines
     *      F       Font type.                  0 = 5x8 dots, 1 = 5x11 dots
     *
     *      Send [00] [00111100] (2 display lines, 5x11 dots)
     *      Wait > 37 us
     */
    LCD_CMD_SEND(0b00111100);   // 8-bit data, 2 display lines, 5x11 dots

    /*
     *  2) Repeat step 1
     *      Send [00] [00111100]
     *      Wait > 37 us
     */
    LCD_CMD_SEND(0b00111100);   // 8-bit data, 2 display lines, 5x11 dots

    /*
     *  3) Display ON/OFF
     *      [00] [00001DCB]
     *      D       Display ON/OFF              0 = OFF, 1 = ON
     *      C       Cursor ON/OFF               0 = OFF, 1 = ON
     *      B       Blink ON/OFF                0 = OFF, 1 = ON
     * 
     *      Send [00] [00001100] (Display ON, cursor and blink OFF)
     *      Wait > 37 us
     */
    LCD_CMD_SEND(0b00001100);   // Display ON, cursor and blink OFF

    /*
     *  4) Display clear
     *      [00] [000000001]
     * 
     *      Send [00] [00000001] (Display clear)
     *      Wait > 1,52 ms
     *
     * DDRAM address set to 0x00 and cursor returned to "original position"
     */
    lcd_clear();

    /*
     *  5) Entry mode set
     *      [00] [000001IS]
     *      I       Inc/dec DDRAM address.      0 = Cursor/Blink left, 1 = right
     *      S       Shift entire display.       0 = Cursor/Blink, 1 = Entire dis
     * 
     *      Send [00] [000000110] (increment DDRAM position on writes)
     *      Wait > 37 us
     *
     * I = 1 means that the DDRAM address is automatically INCREMENTED by one
     * on each write. I = 0 is likely reserved for right-to-left writing
     * systems...
     */
    LCD_CMD_SEND(0b00000110);
}

void lcd_msg_queue_init(void)
{
    // Create new msg_queue
    lcd_msg_queue = xQueueCreate(2, sizeof(struct LCD_message));
}

void lcd_control(void* parameter)
{
    lcd_init(); // First initialize LCD
     
    struct LCD_message msg;
    char space[1] = " ";
    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS);
    while (1)
    {
        // Wait until new message is available and receive it
        xQueueReceive(lcd_msg_queue, (void *)&msg, portMAX_DELAY); 
        // Set cursor at the beginning of the correct line
        lcd_cursor_set(msg.line_num, 0);
        
        // Fill too short message string with end spaces to clear old writing
        while (strlen(msg.text) < 16)
        {
            strncat(msg.text, space, 1); 
        }
        
        // Write the text on LCD
        lcd_write(msg.text);
    }
    vTaskDelete(NULL);
}

void lcd_scrolling_text(void* parameter)
{
    struct LCD_message msg;
    msg.line_num = 1; // Use lower line
    
    char man_text[] = MANUFACTURER_TEXT; 
    size_t text_length = strlen(man_text);
    
    // If the manufacturer text length is shorter than or 16 characters, 
    // there is no need to scroll the text and this task can be deleted
    // Or also if scroll speed has been defined as 0.
    if ((text_length <= 16) || (SCROLL_SPEED_CPS <= 0))
    {
        // Send the text to displaying task before of course
        memcpy(msg.text, man_text, 16);
        xQueueSend(lcd_msg_queue, (void *)&msg, 10);
        vTaskDelete(NULL);
    }
        
    // Scroll direction. 1: left to right, -1: right to left
    int8_t scroll_dir = 1; 
    size_t i = 0;
    size_t max_index = text_length - 16;
    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS); 
    
    // Each iteration either decreases or increases the loop counter by one 
    // depending on the current scroll direction
    for (;; i += scroll_dir)
    {    
        // Change scroll direction if necessary
        if (i == max_index)
        {
            scroll_dir = -1; // Leftwards
        }     
        else if (i == 0)
        {
            scroll_dir = 1; // Rightwards
        }
         
        // Update the text to send: next 16 characters starting from the
        // current index
        memcpy(msg.text, &man_text[i], 16);
        // Send the message via lcd_msg_queue
        xQueueSend(lcd_msg_queue, (void *)&msg, portMAX_DELAY);
        // Wait before scrolling again 
        vTaskDelay((1000 / SCROLL_SPEED_CPS) / portTICK_PERIOD_MS);
    } 
    vTaskDelete(NULL);
}

void lcd_adc_report(void* parameter)
{
    struct LCD_message msg;
    msg.line_num = 0; // Use upper line

    uint16_t ldr_reading;
    uint16_t ntc_reading;
    uint16_t pot_reading;
    
    // 200 ms delay before entering superloop
    vTaskDelay(200 / portTICK_PERIOD_MS); 
    while (1)
    {  
        /* LDR */
        ldr_reading = adc_read(LDR); // Take the reading
        sprintf(msg.text, "LDR value: %u ", ldr_reading); // Format to string
        xQueueSend(lcd_msg_queue, (void *)&msg, portMAX_DELAY); // Send to queue    
        vTaskDelay(660 / portTICK_PERIOD_MS); // Wait 660 ms
        // ...Repeat these steps with NTC and POT...
        
        /* NTC */
        ntc_reading = adc_read(NTC);
        sprintf(msg.text, "NTC value: %u ", ntc_reading); 
        xQueueSend(lcd_msg_queue, (void *)&msg, portMAX_DELAY);
        vTaskDelay(660 / portTICK_PERIOD_MS);
        
        /* POTENTIOMETER */
        pot_reading = adc_read(POT);
        sprintf(msg.text, "POT value: %u", pot_reading); 
        xQueueSend(lcd_msg_queue, (void *)&msg, portMAX_DELAY);
        vTaskDelay(660 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/* EOF */
