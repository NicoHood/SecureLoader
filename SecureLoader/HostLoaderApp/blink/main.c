/*
 blinklight.c

 Blinks a LED- basic example to show you can upload to your avr
 */

#include <avr/io.h>
#include <avr/delay.h>

#define LED 7

int main(void) {

    DDRC |= _BV(LED);

    for(;;) {
        PORTC |= _BV(LED);
        _delay_ms(1000);
        PORTC &= ~_BV(LED);
        _delay_ms(1000);
    }

    return 0;
}
