/* 
 * File:    lcd.h
 *          Minimal 1602 LCD driver (8-bit bus), FreeRTOS version
 * Author:  Jani Tammi <jasata@utu.fi>
 * Created: 13 November 2021, 14:19
 * Device:  ATmega4809 Curiosity Nano
 *
 */

#ifndef LCD_H
#define	LCD_H

#define LCD_LINE0       0
#define LCD_LINE1       1


#ifdef	__cplusplus
extern "C" {
#endif
    
/*
 * lcd_init()
 *
 *      EXTREMELY SLOW!!! Takes minimum of 103 milliseconds.
 */
void lcd_init(void);

/*
 * lcd_write()
 *
 *      Writes the given string to current cursor position.
 */
void lcd_write(char *str);

/*
 * lcd_cursor_set()
 *
 *      Moves internal cursor to specified position.
 *      x       even = line 0, odd = line 1
 *      y       0x00 ... 0x15, the character position in a line
 */
void lcd_cursor_set(uint8_t x, uint8_t y);

/*
 * lcd_clear()
 *
 *      SLOW! Takes 2 milliseconds. Resets cursor position to (0, 0).
 */
void lcd_clear(void);

/* Initialize lcd_msg_queue. */
void lcd_msg_queue_init(void);

/* Controls the cursor position and shows whatever text it 
 * receives via lcd_msg_queue on the LCD */
void lcd_control(void* parameter);

/* Provides the lower line "scrolling text" to lcd_control via the queue */
void lcd_scrolling_text(void* parameter);

/* Provides the upper line LDR, NTC and potentiometer 
 * value reports to lcd_control via the queue */
void lcd_adc_report(void* parameter);

#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */

