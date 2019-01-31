#include "Emulator.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

//takes in the pc and increments it, also intakes the num of cycles for each PC instruction
void Emulator::op(int pc, int cycle)
{
	reg_PC += (Byte_Signed)pc; //needs to be Signed byte b/c PC can be negative which is not seen w/ unsigned

	num_cycles += (cycle * 4); //1 Machine Cycle  = 4 clock cycles


}

//process the cb prefix instructions
//void parse_bit_op(Opcode code)

void Emulator::set_flag(int flag, bool value)
{
	if (value)
		reg_AF.lo |= flag; //or means to keep the others the same value (or w/ 0) but turn on the bit we want (or w/ 1)
	else
		reg_AF.lo &= ~(flag); //and means to keep the others the same (and w/ 1) but turn off the bit we want (and w/ 0) [Notting means we're 'and'ing with the correct values]
}

// 8-bit loads
void Emulator::LD(Byte& destination, Byte value)
{
	destination = value;
}

//used to read the contents of an address in memory [LDH A,(n)]
void Emulator::LD(Byte& destination, Address addr)
{
	destination = readMemory(addr);
}

void Emulator::LD(Address addr, Byte value)
{
	writeMemory(addr, value);
}

// 16-bit loads
void Emulator::LD(Register & pair, Byte upper, Byte lower)
{
	pair.reg = (Word) (upper << 8 | lower);
}


/*void Emulator::LD(Word& reg_pair, Byte upper, Byte lower)
{
	reg_pair = combine(upper, lower);
}*/

//ld hl, sp+n ????????????????
void Emulator::LDHL(Byte value)
{
	//value is a signed value

	Word_Signed val = (Word_Signed)(Byte_Signed)value; //need to convert from unsigned byte to signed byte to signed word in order to place in SP
	Word result = Word((Word_Signed)reg_SP + val); //need to convert back to Unsigned word b/c using SP

	//??
	set_flag(FLAG_HALF_CARRY, (result & 0xF) < (reg_SP & 0xF)); //! --> if there was a carry, then result would have a 0 at bit3 [0-3 indice] making it appear to be smaller b/c of the carry
	set_flag(FLAG_CARRY, (result & 0xFF) < (reg_SP & 0xFF)); //! --> if there was a carry, then result would have a 0 at bit 15 [12-15 indice] making it appear to be smaller b/c of the carry
	set_flag(FLAG_ZERO, false); //reset
	set_flag(FLAG_SUB, false); //reset

	reg_HL.reg = result;
	
}

//load (nn), SP
void Emulator::LDNN(Byte low, Byte high)
{
	Byte lowSP = (Byte)reg_SP;
	Byte highSP = (Byte)(reg_SP >> 8);

	Address location = (high << 8) | low;
	writeMemory(location, lowSP);
	writeMemory(location + 1, highSP);

}

void Emulator::PUSH(Byte high, Byte low)
{
	reg_SP--;
	writeMemory(reg_SP, high);
	reg_SP--;
	writeMemory(reg_SP, low);

}

void Emulator::POP(Byte& high, Byte& low)
{
	low = readMemory(reg_SP);
	reg_SP++;
	high = readMemory(reg_SP);
	reg_SP++;
}

void Emulator::ADD(Byte& target, Byte value)
{
	Word result = target + value;

	set_flag(FLAG_ZERO, (result & 0xFF) == 0);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, ((value & 0xF) + (target & 0xF)) > 0x0F);
	set_flag(FLAG_CARRY, result > 0xFF);

	target = (Byte)(result & 0xFF);
	//LD(target, (Byte)(result & 0xFF));
}

//add from an address [HL?]
void Emulator::ADD(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	ADD(target, value);
}

void Emulator::ADC(Byte& target, Byte value)
{
	Word carry = (reg_AF.lo & 0x10) >> 4;
	Word result = target + value + carry;

	set_flag(FLAG_ZERO, (result & 0xFF) == 0);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, ((value & 0xF) + (target & 0xF) + (Byte)carry) & 0x10);
	set_flag(FLAG_CARRY, result > 0xFF);

	target = (Byte)(result & 0xFF);
	//LD(target, (Byte)(result & 0xFF));


}

//add from an address [HL?]
void Emulator::ADC(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	ADC(target, value);
}

void Emulator::SUB(Byte& target, Byte value)
{
	Word_Signed result = (Word_Signed)target - (Word_Signed)value;
	Byte_Signed sTarget = (Byte_Signed)target;
	Byte_Signed sValue = (Byte_Signed)value;

	set_flag(FLAG_ZERO, result == 0);
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_HALF_CARRY, (sTarget & 0xF) < (sValue & 0xF)); //check if subtracting a bigger value [need it in signed]
	set_flag(FLAG_CARRY, (sTarget & 0xFF) < (sValue & 0xFF));

	target = (Byte)(result & 0xFF);
	//LD(target, (Byte)(result & 0xFF));
}

void Emulator::SUB(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	SUB(target, value);
}

void Emulator::SBC(Byte& target, Byte value)
{
	Byte carry = (reg_AF.lo & 0x10) >> 4;
	Word_Signed result = (Word_Signed)target - (Word_Signed)value - (Word_Signed)carry;
	Byte_Signed sTarget = (Byte_Signed)target;
	Byte_Signed sValue = (Byte_Signed)value;

	set_flag(FLAG_ZERO, (result & 0xFF) == 0);
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_HALF_CARRY, ((sTarget & 0xF) - (sValue & 0xF) - carry) < 0); //If it's less than 0 that means it went negative & needed to borrow 
	set_flag(FLAG_CARRY, (target & 0xFF) < (value + carry)); //subtracting value is greater than the target

	target = (Byte)(result & 0xFF);
	//LD(target, (Byte)(result & 0xFF));
}

void Emulator::SBC(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	SBC(target, value);
}

void Emulator::AND(Byte& target, Byte value)
{
	target &= value;

	set_flag(FLAG_ZERO, target == 0);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_CARRY, false);
	set_flag(FLAG_HALF_CARRY, true);

	/*int x;
	printf("Inside and\n");
	cin >> x;
	printf("opcode: %x,"
		"reg_A: %02x, "
		"reg_B: %02x, "
		"reg_C: %02x, "
		"reg_D: %02x, "
		"reg_E: %02x, "
		"reg_F: %02x, "
		"reg_H: %02x, "
		"reg_L: %02x, "
		"reg_PC: %04x, "
		"reg_SP: %04x, \n",
		reg_PC, reg_A, reg_B, reg_C, reg_D, reg_E, reg_F, reg_H, reg_L, reg_PC, reg_SP);*/

}

void Emulator::AND(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	AND(target, value);
}

void Emulator::OR(Byte& target, Byte value)
{
	target |= value;

	set_flag(FLAG_ZERO, target == 0);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_CARRY, false);
	set_flag(FLAG_HALF_CARRY, false);
}

void Emulator::OR(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	OR(target, value);
}

void Emulator::XOR(Byte& target, Byte value)
{
	target ^= value;

	set_flag(FLAG_ZERO, target == 0);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_CARRY, false);
	set_flag(FLAG_HALF_CARRY, false);
}

void Emulator::XOR(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	XOR(target, value);
}

void Emulator::CP(Byte& target, Byte value)
{
	set_flag(FLAG_ZERO, target == value);
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_CARRY, ((target & 0xFF) < (value & 0xFF)));
	set_flag(FLAG_HALF_CARRY, ((target & 0xF) < (value & 0xF)));
}

void Emulator::CP(Byte& target, Address addr)
{
	Byte value = readMemory(addr);
	CP(target, value);
}

void Emulator::INC(Byte& target)
{
	Byte result = target + 1;

	set_flag(FLAG_ZERO, result == 0);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, ((target & 0xf) + (0x1)) & 0x10);

	target = result;

}

//INC (HL)
void Emulator::INC(Address addr)
{
	Byte value = readMemory(addr);
	INC(value);
	writeMemory(addr, value);
}

void Emulator::DEC(Byte& target)
{
	Byte_Signed sTarget = (Byte_Signed)target;

	target -= 1;

	set_flag(FLAG_ZERO, target == 0);
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_HALF_CARRY, (sTarget & 0xF) - 1 < 0);
}

void Emulator::DEC(Address addr)
{
	Byte value = readMemory(addr);
	DEC(value);
	writeMemory(addr, value);
}

// 16-bit arithmetic
void Emulator::ADD16(Word target, Word value)
{
	uint32_t result = target + value; //can check for overflow easier w/ int (32 bits)

	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, ((target & 0xFFF) + (value & 0xFFF)) & 0x1000);
	set_flag(FLAG_CARRY, result > 0xFFFF); //overflow is into bit 16 or 0x10000
}

// ADD HL, Reg pair
void Emulator::ADDHL(Register & pair)
{
	Word value = pair.reg;
	ADD16(reg_HL.reg, value); //Adding HL to HL, set appropriate flags

	value += reg_HL.reg; //actaully add the values together
	reg_HL.reg = value; //above ADD16 sets the flags according to what the value would be after the flag but doesn't actually set it, need to set here
}

void Emulator::ADDHLSP()
{
	Byte low = lowByte(reg_SP);
	Byte high = highByte(reg_SP);

	Register temp;
	temp.reg = (Word) (high << 8) | low;
	ADDHL(temp);
}

void Emulator::ADDSP(Byte value)
{
	Word_Signed sVal = (Word_Signed)(Byte_Signed)value; //need to convert from unsigned byte to signed byte to signed word in order to place in SP
	Word result = Word((Word_Signed)reg_SP + sVal); //need to convert back to Unsigned word b/c using SP

	//??
	set_flag(FLAG_HALF_CARRY, (result & 0xF) < (reg_SP & 0xF)); //! --> if there was a carry, then result would have a 0 at bit3 [0-3 indice] making it appear to be smaller b/c of the carry
	set_flag(FLAG_CARRY, (result & 0xFF) < (reg_SP & 0xFF)); //! --> if there was a carry, then result would have a 0 at bit 15 [12-15 indice] making it appear to be smaller b/c of the carry
	set_flag(FLAG_ZERO, false); //reset
	set_flag(FLAG_SUB, false); //reset

	reg_SP = result;
}

void Emulator::INC(Register & pair)
{
	Word value = pair.reg;
	value += 1;
	pair.reg = value;
}

void Emulator::INCSP()
{
	reg_SP++;
}

void Emulator::DEC(Register & pair)
{
	Word value = pair.reg;
	value -= 1;
	pair.reg = value;
}

void Emulator::DECSP()
{
	reg_SP--;
}

// Rotate shift instructions

// Shift Left
//????
void Emulator::SL(Byte& target)
{
	Byte result = (target << 1);

	set_flag(FLAG_CARRY, (target & 0x80) != 0);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, (result == 0));

	target <<= 1;
}

void Emulator::SL(Address addr)
{
	Byte value = readMemory(addr);
	SL(value);
	writeMemory(addr, value);
}

// Shift Right
void Emulator::SR(Byte& target, bool include_top_bit)
{
	bool msb = testBit(target, BIT_7); //check if the MSB is 0 or 1
	Byte result;

	if (include_top_bit)
		result = (msb) ? (target >> 1 | 0x80) : target >> 1; //depending on whether msb is 0 or 1, we keep the MSB in both instances, the second one by shifting right the 0 is automatically kept
	else
		result = target >> 1;

	set_flag(FLAG_CARRY, testBit(target, BIT_0));
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, (result == 0));

	target = result;

}

void Emulator::SR(Address addr, bool include_top_bit)
{
	Byte value = readMemory(addr);
	SR(value, include_top_bit);
	writeMemory(addr, value);
}

// Shifts through carry
//??????
void Emulator::RL(Byte& target, bool carry, bool zero_flag)
{
	int bit7 = ((target & 0x80) != 0);
	target = target << 1;

	target |= (carry) ? ((reg_AF.lo & FLAG_CARRY) != 0) : bit7;

	set_flag(FLAG_ZERO, ((zero_flag) ? (target == 0) : false));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, (bit7 != 0));

}

void Emulator::RL(Address addr, bool carry)
{
	Byte value = readMemory(addr);
	RL(value, carry, true);
	writeMemory(addr, value);
}

//????
void Emulator::RR(Byte& target, bool carry, bool zero_flag)
{
	int bit1 = ((target & 0x1) != 0);
	target = target >> 1;

	target |= (carry) ? (((reg_AF.lo & FLAG_CARRY) != 0) << 7) : (bit1 << 7);

	set_flag(FLAG_ZERO, ((zero_flag) ? (target == 0) : false));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, (bit1 != 0));
}

void Emulator::RR(Address addr, bool carry)
{
	Byte value = readMemory(addr);
	RR(value, carry, true);
	writeMemory(addr, value);
}

//?????
void Emulator::SRA(Byte& target)
{
	// content of bit 7 is unchanged
	int bit7 = ((target & 0x80) != 0);
	RR(target, true);
	target |= (bit7 << 7);
	set_flag(FLAG_ZERO, (target == 0));
}

void Emulator::SRA(Address addr)
{
	Byte value = readMemory(addr);
	SRA(value);
	writeMemory(addr, value);
}

//?same as shift right but bit 7 is reset
void Emulator::SRL(Byte& target)
{
	RR(target, true, true);
}

void Emulator::SRL(Address addr)
{
	Byte value = readMemory(addr);
	SRL(value);
	writeMemory(addr, value);
}

void Emulator::SWAP(Byte& target)
{
	Byte low = lowNibble(target);
	Byte high = highNibble(target);

	target = (low << 4) | high;

	set_flag(FLAG_CARRY, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, (target == 0));
}

void Emulator::SWAP(Address addr)
{
	Byte value = readMemory(addr);
	SWAP(value);
	writeMemory(addr, value);
}

// Bit operations
void Emulator::BIT(Byte target, int bit)
{
	Byte bBit = (Byte)bit;
	bool set = !(testBit(target, bBit)); //want if the bit is = 0 

	set_flag(FLAG_HALF_CARRY, true);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, set);
}

void Emulator::BIT(Address addr, int bit)
{
	Byte value = readMemory(addr);
	BIT(value, bit);
}

void Emulator::SET(Byte& target, int bit)
{
	Byte bBit = (Byte)bit;
	target |= (1 << bBit);
}

void Emulator::SET(Address addr, int bit)
{
	Byte value = readMemory(addr);
	SET(value, bit);
	writeMemory(addr, value);
}

void Emulator::RES(Byte& target, int bit)
{
	Byte bBit = (Byte)bit;
	target = bitClear(target, bBit);


}

void Emulator::RES(Address addr, int bit)
{
	Byte value = readMemory(addr);
	RES(value, bit);
	writeMemory(addr, value);
}

void Emulator::SCF()
{
	set_flag(FLAG_CARRY, true);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
}

void Emulator::CCF()
{
	bool set = testBit(reg_AF.lo, BIT_4);

	set_flag(FLAG_CARRY, !set);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
}

// Jump instructions

void Emulator::JP(Register target)
{
	reg_PC = target.reg;
	op(0, 1);  //shouldn't it be 3?
}

void Emulator::JPNZ(Register target)
{
	bool set = (reg_AF.lo >> 7) & 1 ? true : false; //is_bit_set(reg_F, BIT_7);
	if (!set) //jump if 'zero' flag = 0
		JP(target);
}

void Emulator::JPZ(Register target)
{
	bool set = (reg_AF.lo >> 7) & 1 ? true : false; //is_bit_set(reg_F, BIT_7);
	if (set) //jump if 'zero' flag = 1
		JP(target);
}

void Emulator::JPNC(Register target)
{
	bool set = (reg_AF.lo >> 4) & 1 ? true : false; //is_bit_set(reg_F, BIT_4);
	if (!set) //jump if carry flag = 0
		JP(target);
}

void Emulator::JPC(Register target)
{
	bool set = (reg_AF.lo >> 4) & 1 ? true : false; //is_bit_set(reg_F, BIT_4);
	if (set) //jump if carry flag = 1
		JP(target);
}

void Emulator::JR(Byte value)
{
	Byte_Signed sValue = (Byte_Signed)value;
	reg_PC += sValue;
	op(0, 1); //shouldn't it be 2?
}

void Emulator::JRNZ(Byte value)
{
	bool set = (reg_AF.lo >> 7) & 1 ? true : false; //is_bit_set(reg_F, BIT_7);
	if (!set) //jump if 'zero' flag = 0
		JR(value);
}


void Emulator::JRZ(Byte value)
{
	bool set = (reg_AF.lo >> 7) & 1 ? true : false; //is_bit_set(reg_F, BIT_7);
	if (set) //jump if 'zero' flag = 1
		JR(value);
}

void Emulator::JRNC(Byte value)
{
	bool set = (reg_AF.lo >> 4) & 1 ? true : false; //is_bit_set(reg_F, BIT_4);
	if (!set) //jump if carry flag = 0
		JR(value);
}


void Emulator::JRC(Byte value)
{
	bool set = (reg_AF.lo >> 4) & 1 ? true : false; //is_bit_set(reg_F, BIT_4);
	if (set) //jump if carry flag = 1
		JR(value);
}

void Emulator::JPHL()
{
	reg_PC = reg_HL.reg; // Pair(reg_H, reg_L).address();
}

// Function Instructions
void Emulator::CALL(Byte low, Byte high)
{
	Byte lPC = lowByte(reg_PC);
	Byte hPC = highByte(reg_PC);

	reg_SP--;
	writeMemory(reg_SP, hPC);
	reg_SP--;
	writeMemory(reg_SP, lPC);

	Register temp;
	temp.reg = (Word) (high << 8) | low;
	JP(temp);

	op(0, 2); //Because JP is called (which is 1 cycle), need to have op(0,2) so it'll equal to a total of op(0,3)
}

void Emulator::CALLNZ(Byte low, Byte high)
{
	bool set = testBit(reg_AF.lo, BIT_7);
	if (!set) //jump if 'zero' flag = 0
		CALL(low, high);
}

void Emulator::CALLZ(Byte low, Byte high)
{
	bool set = testBit(reg_AF.lo, BIT_7);
	if (set) //jump if 'zero' flag = 1
		CALL(low, high);
}

void Emulator::CALLNC(Byte low, Byte high)
{
	bool set = testBit(reg_AF.lo, BIT_4);
	if (!set) //jump if carry flag = 0
		CALL(low, high);
}

void Emulator::CALLC(Byte low, Byte high)
{
	bool set = testBit(reg_AF.lo, BIT_4);
	if (set) //jump if carry flag = 1
		CALL(low, high);
}

void Emulator::RET()
{
	Byte lAddr = readMemory(reg_SP++);
	Byte hAddr = readMemory(reg_SP++);

	reg_PC = (Word)(hAddr << 8) | lAddr; //Pair(hAddr, lAddr).address();

	op(0, 3); //shouldn't it be 2?
}

///????
void Emulator::RETI()
{
	//?
	interruptMasterEnable = true; //restore interrupts
	RET();
}

void Emulator::RETNZ()
{
	bool set = testBit(reg_AF.lo, BIT_7);
	if (!set) {//jump if 'zero' flag = 0
		RET();
		op(0, 2);
	}
}

void Emulator::RETZ()
{
	bool set = testBit(reg_AF.lo, BIT_7);
	if (set) { //jump if 'zero' flag = 1
		RET();
		op(0, 2);
	}

}

void Emulator::RETNC()
{
	bool set = testBit(reg_AF.lo, BIT_4);
	if (!set) {//jump if carry flag = 0
		RET();
		op(0, 2);
	}
}

void Emulator::RETC()
{
	bool set = testBit(reg_AF.lo, BIT_4);
	if (set) {//jump if carry flag = 1
		RET();
		op(0, 2);
	}
}

// Miscellaneous Instructions
void Emulator::RST(Address addr)
{
	Byte lPC = lowByte(reg_PC);
	Byte hPC = highByte(reg_PC);

	reg_SP--;
	writeMemory(reg_SP, hPC);
	reg_SP--;
	writeMemory(reg_SP, lPC);


	reg_PC = addr;

}

//https://ehaskins.com/2018-01-30%20Z80%20DAA/ --> explains DAA instruction
void Emulator::DAA()
{
	Byte high = highNibble(reg_AF.hi);
	Byte low = lowNibble(reg_AF.hi);

	bool add = !(testBit(reg_AF.lo, BIT_6)); //check if the previous operation was a subtract (0 means it was an add)
	bool carry = testBit(reg_AF.lo, BIT_4);
	bool half_carry = testBit(reg_AF.lo, BIT_5);

	Word result = (Word)reg_AF.hi;
	Word correction = (carry) ? 0x60 : 0x00; //if there was a carry need to add 60 (or 6 to the second digit)

	if (half_carry || (add) && ((result & 0x0F) > 9))
		correction |= 0x06; //there was half carry or the value is greater than 9

	if (carry || (add) && (result > 0x99))
		correction |= 0x60;

	if (add)
		result += correction; //for adding need to add correction
	else
		result -= correction; //o/w subtract

	if (((correction << 2) & 0x100) != 0) //check if there was a carry and is greather than 0x99
		set_flag(FLAG_CARRY, true);

	set_flag(FLAG_HALF_CARRY, false);
	reg_AF.hi = (Byte)(result & 0xFF);
	set_flag(FLAG_ZERO, reg_AF.hi == 0);

}

void Emulator::CPL()
{
	reg_AF.hi = ~reg_AF.hi;
	set_flag(FLAG_HALF_CARRY, true);
	set_flag(FLAG_SUB, true);
}

void Emulator::NOP()
{
	//nothing
}

void Emulator::HALT()
{
	halted = true;
	op(-1, 0);  //repeat the halt instruction until interrupted
}

void Emulator::STOP()
{
	//nothing?
}

// GBEmulatorMan
void Emulator::DI()
{
	interruptMasterEnable = false; //disable interrupts
}

void Emulator::EI()
{
	interruptMasterEnable = true;
}