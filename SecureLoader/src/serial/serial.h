#ifndef SERIAL_H
#define SERIAL_H

#ifndef BAUD
#warning "Please define BAUD via -D parameter first. Defaulting to baud 9600."
#define BAUD 9600
#endif

#include <avr/io.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

//TODO dynamic serial setup
//TODO add buffers/interrupts
void uart_init(void);
void uart_putchar(char c);
void uart_putchars(char data[]);
char uart_getchar(void);
int uart_putchar_stream(char c, FILE __attribute__((__unused__)) *stream);
int uart_getchar_stream(FILE __attribute__((__unused__)) *stream);
void hexdump(void * data, size_t len);

FILE uart_output;
FILE uart_input;
FILE uart_io;

#endif
