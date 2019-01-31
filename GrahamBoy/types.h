#pragma once

#include <cstdint> //uint variables

typedef uint16_t Address, Word;
typedef uint8_t Byte, Opcode;
typedef int8_t Byte_Signed;
typedef int16_t Word_Signed;

extern Byte cartridgeMemory[0x200000];


union Register
{
	Word reg;
	struct
	{
		Byte lo;
		Byte hi;
	};
};

const int 
	FLAG_Z = 7,
	FLAG_N = 6,
	FLAG_H = 5,
	FLAG_C = 4;

const int 
	BIT_7 = 7,
	BIT_6 = 6,
	BIT_5 = 5,
	BIT_4 = 4,
	BIT_3 = 3,
	BIT_2 = 2,
	BIT_1 = 1,
	BIT_0 = 0;

const Byte
	INTERRUPT_VBLANK = 0,
	INTERRUPT_LCD = 1,
	INTERRUPT_TIMER = 2,
	INTERRUPT_SERIAL = 3,
	INTERRUPT_JOYPAD = 4;

const int
	WHITE = 0xFFFFFEFF,
	LIGHT_GREY = 0xc6c6c6FF,
	DARK_GREY = 0x7f7f7fFF,
	BLACK = 0x000000FF;

const int
	FLAG_ZERO = 0b10000000,
	FLAG_SUB = 0b01000000,
	FLAG_HALF_CARRY = 0b00100000,
	FLAG_CARRY = 0b00010000;

const int 
	width = 160,
	height = 144;


bool testBit(Word value, Byte shift);
Byte bitSet(Byte value, int bitNum);
Byte bitClear(Byte value, int bitNum);
Byte highByte(Word value);
Byte lowByte(Word value);
Byte highNibble(Byte value);
Byte lowNibble(Byte value);
Byte getBitVal(Word value, Byte shift);