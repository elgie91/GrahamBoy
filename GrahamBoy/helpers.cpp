#include "types.h"

bool testBit(Word value, Byte shift)
{
	return ((value >> shift) & 0x1 == 1) ? true : false;
}

Byte bitSet(Byte value, int bitNum)
{
	value |= (0x01 << bitNum);
	return value;
}

Byte bitClear(Byte value, int bitNum)
{
	value &= ~(0x01 << bitNum); //besides the 1 which becomes 0, &ing everything w/ 1 to remain the same due to ~
	return value;
}

Byte highByte(Word value)
{
	return (Byte) ((value >> 8) & 0xFF);
}

Byte lowByte(Word value)
{
	return (Byte) (value & 0xFF);
}

Byte getBitVal(Word value, Byte shift)
{
	return ((value >> shift) & 0x1 == 1) ? 1 : 0;
}

Byte lowNibble(Byte value)
{
	return (value & 0xF);
}
Byte highNibble(Byte value)
{
	return ( (value>>4) & 0xF);
}