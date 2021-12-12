#ifndef UART_H
#define	UART_H

/* Makes all required inital configurations for UART usage. */
void uart_init(void);

/* Sends a report strings via UART every second.
 * The report strings contain readings from LDR, NTC and potentiometer */
void uart_send_reports(void* parameter);

#endif	/* UART_H */

