#include <stdlib.h>
#include <avr/pgmspace.h>

#include "simple_print.h"

SimplePrint::SimplePrint() :
	_buffer_position(nullptr),
	_buffer_size(0)
{}

void SimplePrint::print(uint8_t number, uint8_t denom, unsigned char filler) {
	print((uint16_t)number, (uint16_t)denom, filler);
}

void SimplePrint::print(uint16_t number, uint16_t denom, unsigned char filler) {
	div_t division;
	while (denom) {
		division = div(number, denom);
		if (division.quot || denom == 1) {
			write(division.quot + '0');
			filler = '0';
		} else if (filler) {
			write(filler);
		}
		number = division.rem;
		denom /= 10;
	}
}

void SimplePrint::print(int8_t number, int8_t denom, unsigned char filler) {
	print((int16_t)number, (int16_t)denom, filler);
}

void SimplePrint::print(int16_t number, int16_t denom, unsigned char filler) {
	div_t division;
	if(number < 0){
		number = -number;
		write('-');
	}else write(' ');
	while (denom) {
		division = div(number, denom);
		if (division.quot || denom == 1) {
			write(division.quot + '0');
			filler = '0';
		} else if (filler) {
			write(filler);
		}
		number = division.rem;
		denom /= 10;
	}
}

// void SimplePrint::print(float number) {
// 	number += 0.05;
// 	uint8_t integer = (uint8_t)number;
// 	print(integer, 100, ' ');
// 	write('.');
// 	integer = (number - integer) * 10;
// 	print(integer, 1);
// }

void SimplePrint::print(const char *str) {
	uint8_t c;
	while ((c = *(str++))) {
		write(c);
	}
}

void SimplePrint::print_P(const char *str) {
	uint8_t c;
	while ((c = pgm_read_byte(str++))) {
		write(c);
	}
}

void SimplePrint::buffer_init(char* buffer_start, uint8_t buffer_size) {
	_buffer_position = buffer_start;
	_buffer_size = buffer_size;
}

void SimplePrint::write(uint8_t c) {
	if (_buffer_size) {
		*_buffer_position = c;
		_buffer_position++;
		_buffer_size--;
	}
}

char* SimplePrint::get_position() {
	return _buffer_position;
}
