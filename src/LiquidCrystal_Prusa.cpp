#include "LiquidCrystal_Prusa.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "i18n.h"

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//		DL = 1; 8-bit interface data
//		N = 0; 1-line display
//		F = 0; 5x8 dot character font
// 3. Display on/off control:
//		D = 0; Display off
//		C = 0; Cursor off
//		B = 0; Blinking off
// 4. Entry mode set:
//		I/D = 1; Increment by 1
//		S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that it's in that state when a sketch starts (and the
// LiquidCrystal_Prusa constructor is called).

LiquidCrystal_Prusa::LiquidCrystal_Prusa(uint8_t rs, uint8_t rw, uint8_t enable,
					 uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
					 uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
	init(0, rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7);
}

LiquidCrystal_Prusa::LiquidCrystal_Prusa(uint8_t rs, uint8_t enable,
					 uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
					 uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
	init(0, rs, 255, enable, d0, d1, d2, d3, d4, d5, d6, d7);
}

LiquidCrystal_Prusa::LiquidCrystal_Prusa(uint8_t rs, uint8_t rw, uint8_t enable,
					 uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
	init(1, rs, rw, enable, d0, d1, d2, d3, 0, 0, 0, 0);
}

LiquidCrystal_Prusa::LiquidCrystal_Prusa(uint8_t rs, uint8_t enable,
					 uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
	init(1, rs, 255, enable, d0, d1, d2, d3, 0, 0, 0, 0);
}

void LiquidCrystal_Prusa::init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
			 uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
			 uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
	_rs_pin = rs;
	_rw_pin = rw;
	_enable_pin = enable;

	_data_pins[0] = d0;
	_data_pins[1] = d1;
	_data_pins[2] = d2;
	_data_pins[3] = d3;
	_data_pins[4] = d4;
	_data_pins[5] = d5;
	_data_pins[6] = d6;
	_data_pins[7] = d7;

	pinMode(_rs_pin, OUTPUT);
	// we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
	if (_rw_pin != 255) {
		pinMode(_rw_pin, OUTPUT);
	}
	pinMode(_enable_pin, OUTPUT);

	if (fourbitmode)
		_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE;
	else
		_displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE;

	begin();
}

void LiquidCrystal_Prusa::begin() {
	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delayMicroseconds(50000);
	// Now we pull both RS and R/W low to begin commands
	digitalWrite(_rs_pin, LOW);
	digitalWrite(_enable_pin, LOW);
	if (_rw_pin != 255) {
		digitalWrite(_rw_pin, LOW);
	}

	//put the LCD into 4 bit or 8 bit mode
	if (! (_displayfunction & LCD_8BITMODE)) {
		// this is according to the hitachi HD44780 datasheet
		// figure 24, pg 46

		// we start in 8bit mode, try to set 4 bit mode
		write4bits(0x03);
		delayMicroseconds(4500); // wait min 4.1ms

		// second try
		write4bits(0x03);
		delayMicroseconds(4500); // wait min 4.1ms

		// third go!
		write4bits(0x03);
		delayMicroseconds(150);

		// finally, set to 4-bit interface
		write4bits(0x02);
	} else {
		// this is according to the hitachi HD44780 datasheet
		// page 45 figure 23

		// Send function set command sequence
		command(LCD_FUNCTIONSET | _displayfunction);
		delayMicroseconds(4500);	// wait more than 4.1ms

		// second try
		command(LCD_FUNCTIONSET | _displayfunction);
		delayMicroseconds(150);

		// third go
		command(LCD_FUNCTIONSET | _displayfunction);
	}

	// finally, set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);
	delayMicroseconds(60);
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	delayMicroseconds(60);
	// clear it off
	clear();
	delayMicroseconds(3000);
	// Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	delayMicroseconds(60);
}


void LiquidCrystal_Prusa::reinit() {
	// we start in 8bit mode, try to set 4 bit mode
	write4bits(0x03);
	//delayMicroseconds(50); // wait min 4.1ms

	// second try
	write4bits(0x03);
	//delayMicroseconds(50); // wait min 4.1ms

	// third go!
	write4bits(0x03);
	//delayMicroseconds(50);

	// finally, set to 4-bit interface
	write4bits(0x02);

	// finally, set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);
	//delayMicroseconds(60);

	command(LCD_ENTRYMODESET | _displaymode);
	//delayMicroseconds(60);
	display();

	command(LCD_CURSORSHIFT | LCD_ENTRYSHIFTDECREMENT);
	command(LCD_CURSORSHIFT | LCD_ENTRYSHIFTDECREMENT);
	write(' ');
	write(' ');
}


/********** high level commands, for the user! */
void LiquidCrystal_Prusa::clear() {
	command(LCD_CLEARDISPLAY);	// clear display, set cursor position to zero
	delayMicroseconds(1600);	// this command takes a long time
}

void LiquidCrystal_Prusa::home() {
	command(LCD_RETURNHOME);	// set cursor position to zero
	delayMicroseconds(1600);	// this command takes a long time!
}

void LiquidCrystal_Prusa::setCursor(uint8_t col, uint8_t row) {
	if (col != 255 && row != 255) {
		command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
	}
}

// Turn the display on/off (quickly)
void LiquidCrystal_Prusa::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_Prusa::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystal_Prusa::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_Prusa::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystal_Prusa::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_Prusa::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystal_Prusa::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal_Prusa::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystal_Prusa::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystal_Prusa::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystal_Prusa::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystal_Prusa::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal_Prusa::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (uint8_t i = 0; i < 8; i++) {
		write(charmap[i]);
	}
}

void LiquidCrystal_Prusa::print(uint8_t number, uint8_t col, uint8_t row, uint8_t denom, unsigned char filler) {
	setCursor(col, row);
	div_t division;
	while (denom) {
		division = div(number, denom);
		if (division.quot || denom == 1) {
			write(division.quot + '0');
			filler = '0';
		} else {
			write(filler);
		}
		number = division.rem;
		denom /= 10;
	}
}

void LiquidCrystal_Prusa::print(float number, uint8_t col, uint8_t row) {
	number += 0.05;
	uint8_t integer = (uint8_t)number;
	print(integer, col, row, 100, ' ');
	write('.');
	integer = (number - integer) * 10;
	print(integer, 255, 255, 1);
}

void LiquidCrystal_Prusa::printTime(uint8_t min, uint8_t sec, uint8_t col, uint8_t row) {
	print(min, col, row, 10, '0');
	write(':');
	print(sec, 255, 255, 10, '0');
}

void LiquidCrystal_Prusa::print(const char *str, uint8_t col, uint8_t row) {
	setCursor(col, row);
	uint8_t c;
	while ((c = *(str++))) {
		write(c);
	}
}

void LiquidCrystal_Prusa::print_P(const char *str, uint8_t col, uint8_t row) {
	setCursor(col, row);
	uint8_t c;
	while ((c = pgm_read_byte(str++))) {
		write(c);
	}
}

//! Print n characters from null terminated string c
//! if there are not enough characters, prints ' ' for remaining n.
//!
//! @param str null terminated string to print
//! @param n number of characters to print or clear
//! ignored for terminator Terminator::serialNumber - prints always 18 characters
//! @param terminator additional symbol to be printed
//!  * Terminator::none none
//!  * Terminator::back back arrow
//!  * Terminator::right right arrow
//!  * Terminator::serialNumber none
void LiquidCrystal_Prusa::printClear_P(const char *str, uint_least8_t n, Terminator terminator) {
	if (terminator == Terminator::serialNumber) {
		print_P(pgmstr_sn);
		n = 15;
	} else if (terminator != Terminator::none) {
		--n;
	}

	while (n--) {
		uint8_t c = pgm_read_byte(str);
		if (c) {
			write(c);
			++str;
		} else {
			write(' ');
		}
	}
	if (terminator == Terminator::back) write(char(0));
	if (terminator == Terminator::right) write(char(1));
}

/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystal_Prusa::command(uint8_t value) {
	send(value, LOW);
}

inline void LiquidCrystal_Prusa::write(uint8_t value) {
	send(value, HIGH);
}


/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LiquidCrystal_Prusa::send(uint8_t value, uint8_t mode) {
	digitalWrite(_rs_pin, mode);

	// if there is a RW pin indicated, set it low to Write
	if (_rw_pin != 255) {
		digitalWrite(_rw_pin, LOW);
	}

	if (_displayfunction & LCD_8BITMODE) {
		write8bits(value);
	} else {
		write4bits(value>>4);
		write4bits(value);
	}
}

void LiquidCrystal_Prusa::pulseEnable(void) {
	digitalWrite(_enable_pin, LOW);
	delayMicroseconds(1);
	digitalWrite(_enable_pin, HIGH);
	delayMicroseconds(1);		// enable pulse must be >450ns
	digitalWrite(_enable_pin, LOW);
	delayMicroseconds(50);		// commands need > 37us to settle
}

void LiquidCrystal_Prusa::write4bits(uint8_t value) {
	for (uint8_t i = 0; i < 4; i++) {
		pinMode(_data_pins[i], OUTPUT);
		digitalWrite(_data_pins[i], (value >> i) & 0x01);
	}
	pulseEnable();
}

void LiquidCrystal_Prusa::write8bits(uint8_t value) {
	for (uint8_t i = 0; i < 8; i++) {
		pinMode(_data_pins[i], OUTPUT);
		digitalWrite(_data_pins[i], (value >> i) & 0x01);
	}
	pulseEnable();
}