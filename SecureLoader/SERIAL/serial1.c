#include "serial.h"

void uart_init(void) {
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;

#if USE_2X
    UCSR1A |= _BV(U2X1);
#else
    UCSR1A &= ~(_BV(U2X1));
#endif

    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10); /* 8-bit data */
    UCSR1B = _BV(RXEN1) | _BV(TXEN1);   /* Enable RX and TX */
}

void uart_putchar(char c) {
    loop_until_bit_is_set(UCSR1A, UDRE1); /* Wait until data register empty. */
    UDR1 = c;
}

void uart_putchars(char data[]) {
    while(true){
      char c = *data++;
      if(!c){
        break;
      }
      uart_putchar(c);
    }
}

char uart_getchar(void) {
    loop_until_bit_is_set(UCSR1A, RXC1); /* Wait until data exists. */
    return UDR1;
}

int uart_putchar_stream(char c, FILE *stream) {
    // Force proper carriage returns
    if (c == '\n') {
        uart_putchar('\r');
    }
    if (c != '\r') {
        uart_putchar(c);
    }
    return 0;
}

int uart_getchar_stream(FILE *stream) {
    return uart_getchar();
}

void hexdump(void * data, size_t len)
{
	uart_putchars("HEX:\r\n");
	size_t i;
	for (i = 0; i < len; i++) {
		uint8_t b = ((uint8_t*)data)[i];
		uint8_t l = b & 0x0F;
		uint8_t h = b >> 4;

		uart_putchar('0');
		uart_putchar('x');

		if(h<10){
			uart_putchar('0'+h);
		}
		else{
			uart_putchar('A'+h-10);
		}
		if(l<10){
			uart_putchar('0'+l);
		}
		else{
			uart_putchar('A'+l-10);
		}
		uart_putchar('\t');
	}
	uart_putchar('\r');
	uart_putchar('\n');
}

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar_stream, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar_stream, _FDEV_SETUP_READ);
FILE uart_io = FDEV_SETUP_STREAM(uart_putchar_stream, uart_getchar_stream, _FDEV_SETUP_RW);
