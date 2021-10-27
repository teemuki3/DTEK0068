#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>

int main (void)
{
    // set all PORTB pins for output
    DDRB = 0xFF;

    for (;;) {
        // toggle PORTB pins
        PORTB ^= 0xFF;
        // wait one second
        _delay_ms(1000);
    }
    return 0;
}