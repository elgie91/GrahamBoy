#include "Emulator.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <stdint.h>
#include <assert.h>

typedef uint8_t u8;

static const char* const s_opcode_mnemonic[256] = {
  "nop", "ld bc,%hu", "ld [bc],a", "inc bc", "inc b", "dec b", "ld b,%hhu",
  "rlca", "ld [$%04x],sp", "add hl,bc", "ld a,[bc]", "dec bc", "inc c", "dec c",
  "ld c,%hhu", "rrca", "stop", "ld de,%hu", "ld [de],a", "inc de", "inc d",
  "dec d", "ld d,%hhu", "rla", "jr %+hhd", "add hl,de", "ld a,[de]", "dec de",
  "inc e", "dec e", "ld e,%hhu", "rra", "jr nz,%+hhd", "ld hl,%hu",
  "ld [hl+],a", "inc hl", "inc h", "dec h", "ld h,%hhu", "daa", "jr z,%+hhd",
  "add hl,hl", "ld a,[hl+]", "dec hl", "inc l", "dec l", "ld l,%hhu", "cpl",
  "jr nc,%+hhd", "ld sp,%hu", "ld [hl-],a", "inc sp", "inc [hl]", "dec [hl]",
  "ld [hl],%hhu", "scf", "jr c,%+hhd", "add hl,sp", "ld a,[hl-]", "dec sp",
  "inc a", "dec a", "ld a,%hhu", "ccf", "ld b,b", "ld b,c", "ld b,d", "ld b,e",
  "ld b,h", "ld b,l", "ld b,[hl]", "ld b,a", "ld c,b", "ld c,c", "ld c,d",
  "ld c,e", "ld c,h", "ld c,l", "ld c,[hl]", "ld c,a", "ld d,b", "ld d,c",
  "ld d,d", "ld d,e", "ld d,h", "ld d,l", "ld d,[hl]", "ld d,a", "ld e,b",
  "ld e,c", "ld e,d", "ld e,e", "ld e,h", "ld e,l", "ld e,[hl]", "ld e,a",
  "ld h,b", "ld h,c", "ld h,d", "ld h,e", "ld h,h", "ld h,l", "ld h,[hl]",
  "ld h,a", "ld l,b", "ld l,c", "ld l,d", "ld l,e", "ld l,h", "ld l,l",
  "ld l,[hl]", "ld l,a", "ld [hl],b", "ld [hl],c", "ld [hl],d", "ld [hl],e",
  "ld [hl],h", "ld [hl],l", "halt", "ld [hl],a", "ld a,b", "ld a,c", "ld a,d",
  "ld a,e", "ld a,h", "ld a,l", "ld a,[hl]", "ld a,a", "add a,b", "add a,c",
  "add a,d", "add a,e", "add a,h", "add a,l", "add a,[hl]", "add a,a",
  "adc a,b", "adc a,c", "adc a,d", "adc a,e", "adc a,h", "adc a,l",
  "adc a,[hl]", "adc a,a", "sub a,b", "sub a,c", "sub a,d", "sub a,e",
  "sub a,h", "sub a,l", "sub a,[hl]", "sub a,a", "sbc a,b", "sbc a,c",
  "sbc a,d", "sbc a,e", "sbc a,h", "sbc a,l", "sbc a,[hl]", "sbc a,a",
  "and a,b", "and a,c", "and a,d", "and a,e", "and a,h", "and a,l",
  "and a,[hl]", "and a,a", "xor a,b", "xor a,c", "xor a,d", "xor a,e",
  "xor a,h", "xor a,l", "xor a,[hl]", "xor a,a", "or a,b", "or a,c", "or a,d",
  "or a,e", "or a,h", "or a,l", "or a,[hl]", "or a,a", "cp a,b", "cp a,c",
  "cp a,d", "cp a,e", "cp a,h", "cp a,l", "cp a,[hl]", "cp a,a", "ret nz",
  "pop bc", "jp nz,$%04hx", "jp $%04hx", "call nz,$%04hx", "push bc",
  "add a,%hhu", "rst $00", "ret z", "ret", "jp z,$%04hx", NULL, "call z,$%04hx",
  "call $%04hx", "adc a,%hhu", "rst $08", "ret nc", "pop de", "jp nc,$%04hx",
  NULL, "call nc,$%04hx", "push de", "sub a,%hhu", "rst $10", "ret c", "reti",
  "jp c,$%04hx", NULL, "call c,$%04hx", NULL, "sbc a,%hhu", "rst $18",
  "ldh [$ff%02hhx],a", "pop hl", "ld [$ff00+c],a", NULL, NULL, "push hl",
  "and a,%hhu", "rst $20", "add sp,%hhd", "jp hl", "ld [$%04hx],a", NULL, NULL,
  NULL, "xor a,%hhu", "rst $28", "ldh a,[$ff%02hhx]", "pop af",
  "ld a,[$ff00+c]", "di", NULL, "push af", "or a,%hhu", "rst $30",
  "ld hl,sp%+hhd", "ld sp,hl", "ld a,[$%04hx]", "ei", NULL, NULL, "cp a,%hhu",
  "rst $38",
};

static const char* const s_cb_opcode_mnemonic[256] = {
    "rlc b",      "rlc c",   "rlc d",      "rlc e",   "rlc h",      "rlc l",
    "rlc [hl]",   "rlc a",   "rrc b",      "rrc c",   "rrc d",      "rrc e",
    "rrc h",      "rrc l",   "rrc [hl]",   "rrc a",   "rl b",       "rl c",
    "rl d",       "rl e",    "rl h",       "rl l",    "rl [hl]",    "rl a",
    "rr b",       "rr c",    "rr d",       "rr e",    "rr h",       "rr l",
    "rr [hl]",    "rr a",    "sla b",      "sla c",   "sla d",      "sla e",
    "sla h",      "sla l",   "sla [hl]",   "sla a",   "sra b",      "sra c",
    "sra d",      "sra e",   "sra h",      "sra l",   "sra [hl]",   "sra a",
    "swap b",     "swap c",  "swap d",     "swap e",  "swap h",     "swap l",
    "swap [hl]",  "swap a",  "srl b",      "srl c",   "srl d",      "srl e",
    "srl h",      "srl l",   "srl [hl]",   "srl a",   "bit 0,b",    "bit 0,c",
    "bit 0,d",    "bit 0,e", "bit 0,h",    "bit 0,l", "bit 0,[hl]", "bit 0,a",
    "bit 1,b",    "bit 1,c", "bit 1,d",    "bit 1,e", "bit 1,h",    "bit 1,l",
    "bit 1,[hl]", "bit 1,a", "bit 2,b",    "bit 2,c", "bit 2,d",    "bit 2,e",
    "bit 2,h",    "bit 2,l", "bit 2,[hl]", "bit 2,a", "bit 3,b",    "bit 3,c",
    "bit 3,d",    "bit 3,e", "bit 3,h",    "bit 3,l", "bit 3,[hl]", "bit 3,a",
    "bit 4,b",    "bit 4,c", "bit 4,d",    "bit 4,e", "bit 4,h",    "bit 4,l",
    "bit 4,[hl]", "bit 4,a", "bit 5,b",    "bit 5,c", "bit 5,d",    "bit 5,e",
    "bit 5,h",    "bit 5,l", "bit 5,[hl]", "bit 5,a", "bit 6,b",    "bit 6,c",
    "bit 6,d",    "bit 6,e", "bit 6,h",    "bit 6,l", "bit 6,[hl]", "bit 6,a",
    "bit 7,b",    "bit 7,c", "bit 7,d",    "bit 7,e", "bit 7,h",    "bit 7,l",
    "bit 7,[hl]", "bit 7,a", "res 0,b",    "res 0,c", "res 0,d",    "res 0,e",
    "res 0,h",    "res 0,l", "res 0,[hl]", "res 0,a", "res 1,b",    "res 1,c",
    "res 1,d",    "res 1,e", "res 1,h",    "res 1,l", "res 1,[hl]", "res 1,a",
    "res 2,b",    "res 2,c", "res 2,d",    "res 2,e", "res 2,h",    "res 2,l",
    "res 2,[hl]", "res 2,a", "res 3,b",    "res 3,c", "res 3,d",    "res 3,e",
    "res 3,h",    "res 3,l", "res 3,[hl]", "res 3,a", "res 4,b",    "res 4,c",
    "res 4,d",    "res 4,e", "res 4,h",    "res 4,l", "res 4,[hl]", "res 4,a",
    "res 5,b",    "res 5,c", "res 5,d",    "res 5,e", "res 5,h",    "res 5,l",
    "res 5,[hl]", "res 5,a", "res 6,b",    "res 6,c", "res 6,d",    "res 6,e",
    "res 6,h",    "res 6,l", "res 6,[hl]", "res 6,a", "res 7,b",    "res 7,c",
    "res 7,d",    "res 7,e", "res 7,h",    "res 7,l", "res 7,[hl]", "res 7,a",
    "set 0,b",    "set 0,c", "set 0,d",    "set 0,e", "set 0,h",    "set 0,l",
    "set 0,[hl]", "set 0,a", "set 1,b",    "set 1,c", "set 1,d",    "set 1,e",
    "set 1,h",    "set 1,l", "set 1,[hl]", "set 1,a", "set 2,b",    "set 2,c",
    "set 2,d",    "set 2,e", "set 2,h",    "set 2,l", "set 2,[hl]", "set 2,a",
    "set 3,b",    "set 3,c", "set 3,d",    "set 3,e", "set 3,h",    "set 3,l",
    "set 3,[hl]", "set 3,a", "set 4,b",    "set 4,c", "set 4,d",    "set 4,e",
    "set 4,h",    "set 4,l", "set 4,[hl]", "set 4,a", "set 5,b",    "set 5,c",
    "set 5,d",    "set 5,e", "set 5,h",    "set 5,l", "set 5,[hl]", "set 5,a",
    "set 6,b",    "set 6,c", "set 6,d",    "set 6,e", "set 6,h",    "set 6,l",
    "set 6,[hl]", "set 6,a", "set 7,b",    "set 7,c", "set 7,d",    "set 7,e",
    "set 7,h",    "set 7,l", "set 7,[hl]", "set 7,a",
};

static void sprint_hex(char* buffer, u8 val) {
  const char hex_digits[] = "0123456789abcdef";
  buffer[0] = hex_digits[(val >> 4) & 0xf];
  buffer[1] = hex_digits[val & 0xf];
}

static const u8 s_opcode_bytes[] = {
    /*       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
    /* 00 */ 1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1,
    /* 10 */ 1, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    /* 20 */ 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    /* 30 */ 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    /* 40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 80 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 90 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* a0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* b0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* c0 */ 1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1,
    /* d0 */ 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1,
    /* e0 */ 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1,
    /* f0 */ 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1,
};

static int disassemble_instr(u8 data[3], char* buffer, size_t size) {
  char temp[100];
  u8 opcode = data[0];
  u8 num_bytes = s_opcode_bytes[opcode];
  switch (num_bytes) {
    case 1: {
      const char* mnemonic = s_opcode_mnemonic[opcode];
      if (!mnemonic) {
        mnemonic = "*INVALID*";
      }
      strncpy(temp, mnemonic, sizeof(temp) - 1);
      break;
    }
    case 2:
      if (opcode == 0xcb) {
        strncpy(temp, s_cb_opcode_mnemonic[data[1]], sizeof(temp) - 1);
      } else {
        snprintf(temp, sizeof(temp), s_opcode_mnemonic[opcode], data[1]);
      }
      break;
    case 3:
      snprintf(temp, sizeof(temp), s_opcode_mnemonic[opcode],
               (data[2] << 8) | data[1]);
      break;
    default: assert(!"invalid opcode byte length.\n"); break;
  }

  char hex[][3] = {"  ", "  ", "  "};
  switch (num_bytes) {
    case 3: sprint_hex(hex[2], data[2]); /* Fallthrough. */
    case 2: sprint_hex(hex[1], data[1]); /* Fallthrough. */
    case 1: sprint_hex(hex[0], data[0]); break;
  }

  snprintf(buffer, size, "%s %s %s  %-15s", hex[0], hex[1], hex[2], temp);
  return num_bytes;
}

int emulator_disassemble(Address addr, u8 code, u8 value, u8 value2,
                         char *buffer, size_t size) {
  char instr[120];
  char hex[][3] = {"  ", "  ", "  "};

  u8 data[3] = {code, value, value2};
  int num_bytes = disassemble_instr(data, instr, sizeof(instr));

  snprintf(buffer, size, "%#06x: %s", addr, instr);
  return num_bytes ? num_bytes : 1;
}

static void print_instruction(Address addr, u8 code, u8 value, u8 value2) {
  char temp[64];
  emulator_disassemble(addr, code, value, value2, temp, sizeof(temp));
  printf("%s", temp);
}

void Emulator::parseBitOp(Byte code)
{
	switch (code)
	{
	case 0x07: RL(reg_AF.hi, false, true); op(2, 2); break;
	case 0x00: RL(reg_BC.hi, false, true); op(2, 2); break;
	case 0x01: RL(reg_BC.lo, false, true); op(2, 2); break;
	case 0x02: RL(reg_DE.hi, false, true); op(2, 2); break;
	case 0x03: RL(reg_DE.lo, false, true); op(2, 2); break;
	case 0x04: RL(reg_HL.hi, false, true); op(2, 2); break;
	case 0x05: RL(reg_HL.lo, false, true); op(2, 2); break;
	case 0x06: RL(reg_HL.reg, false); op(2, 4);  break;
	case 0x17: RL(reg_AF.hi, true, true); op(2, 2); break;
	case 0x10: RL(reg_BC.hi, true, true); op(2, 2); break;
	case 0x11: RL(reg_BC.lo, true, true); op(2, 2); break;
	case 0x12: RL(reg_DE.hi, true, true); op(2, 2); break;
	case 0x13: RL(reg_DE.lo, true, true); op(2, 2); break;
	case 0x14: RL(reg_HL.hi, true, true); op(2, 2); break;
	case 0x15: RL(reg_HL.lo, true, true); op(2, 2); break;
	case 0x16: RL(reg_HL.reg, true); op(2, 4); break;

	case 0x0F: RR(reg_AF.hi, false, true); op(2, 2); break;
	case 0x08: RR(reg_BC.hi, false, true); op(2, 2); break;
	case 0x09: RR(reg_BC.lo, false, true); op(2, 2); break;
	case 0x0A: RR(reg_DE.hi, false, true); op(2, 2); break;
	case 0x0B: RR(reg_DE.lo, false, true); op(2, 2); break;
	case 0x0C: RR(reg_HL.hi, false, true); op(2, 2); break;
	case 0x0D: RR(reg_HL.lo, false, true); op(2, 2); break;
	case 0x0E: RR(reg_HL.reg, false); op(2, 4); break;
	case 0x1F: RR(reg_AF.hi, true, true); op(2, 2); break;
	case 0x18: RR(reg_BC.hi, true, true); op(2, 2); break;
	case 0x19: RR(reg_BC.lo, true, true); op(2, 2); break;
	case 0x1A: RR(reg_DE.hi, true, true); op(2, 2); break;
	case 0x1B: RR(reg_DE.lo, true, true); op(2, 2); break;
	case 0x1C: RR(reg_HL.hi, true, true); op(2, 2); break;
	case 0x1D: RR(reg_HL.lo, true, true); op(2, 2); break;
	case 0x1E: RR(reg_HL.reg, true); op(2, 4); break; // this could have a different beginning opcode, check manual

	case 0x27: SL(reg_AF.hi); op(2, 2); break;
	case 0x20: SL(reg_BC.hi); op(2, 2); break;
	case 0x21: SL(reg_BC.lo); op(2, 2); break;
	case 0x22: SL(reg_DE.hi); op(2, 2); break;
	case 0x23: SL(reg_DE.lo); op(2, 2); break;
	case 0x24: SL(reg_HL.hi); op(2, 2); break;
	case 0x25: SL(reg_HL.lo); op(2, 2); break;
	case 0x26: SL(reg_HL.reg); op(2, 4); break; // this could actually have a different beginning opcode, check manual

	case 0x2F: SR(reg_AF.hi, true); op(2, 2); break;
	case 0x28: SR(reg_BC.hi, true); op(2, 2); break;
	case 0x29: SR(reg_BC.lo, true); op(2, 2); break;
	case 0x2A: SR(reg_DE.hi, true); op(2, 2); break;
	case 0x2B: SR(reg_DE.lo, true); op(2, 2); break;
	case 0x2C: SR(reg_HL.hi, true); op(2, 2); break;
	case 0x2D: SR(reg_HL.lo, true); op(2, 2); break;
	case 0x2E: SR(reg_HL.reg, true); op(2, 4); break;

	case 0x3F: SR(reg_AF.hi, false); op(2, 2); break;
	case 0x38: SR(reg_BC.hi, false); op(2, 2); break;
	case 0x39: SR(reg_BC.lo, false); op(2, 2); break;
	case 0x3A: SR(reg_DE.hi, false); op(2, 2); break;
	case 0x3B: SR(reg_DE.lo, false); op(2, 2); break;
	case 0x3C: SR(reg_HL.hi, false); op(2, 2); break;
	case 0x3D: SR(reg_HL.lo, false); op(2, 2); break;
	case 0x3E: SR(reg_HL.reg, false); op(2, 4); break;

	case 0x37: SWAP(reg_AF.hi); op(2, 2); break;
	case 0x30: SWAP(reg_BC.hi); op(2, 2); break;
	case 0x31: SWAP(reg_BC.lo); op(2, 2); break;
	case 0x32: SWAP(reg_DE.hi); op(2, 2); break;
	case 0x33: SWAP(reg_DE.lo); op(2, 2); break;
	case 0x34: SWAP(reg_HL.hi); op(2, 2); break;
	case 0x35: SWAP(reg_HL.lo); op(2, 2); break;
	case 0x36: SWAP(reg_HL.reg); op(2, 4); break;

	case 0x47: BIT(reg_AF.hi, 0); op(2, 2); break;
	case 0x4F: BIT(reg_AF.hi, 1); op(2, 2); break;
	case 0x57: BIT(reg_AF.hi, 2); op(2, 2); break;
	case 0x5F: BIT(reg_AF.hi, 3); op(2, 2); break;
	case 0x67: BIT(reg_AF.hi, 4); op(2, 2); break;
	case 0x6F: BIT(reg_AF.hi, 5); op(2, 2); break;
	case 0x77: BIT(reg_AF.hi, 6); op(2, 2); break;
	case 0x7F: BIT(reg_AF.hi, 7); op(2, 2); break;
	case 0x40: BIT(reg_BC.hi, 0); op(2, 2); break;
	case 0x48: BIT(reg_BC.hi, 1); op(2, 2); break;
	case 0x50: BIT(reg_BC.hi, 2); op(2, 2); break;
	case 0x58: BIT(reg_BC.hi, 3); op(2, 2); break;
	case 0x60: BIT(reg_BC.hi, 4); op(2, 2); break;
	case 0x68: BIT(reg_BC.hi, 5); op(2, 2); break;
	case 0x70: BIT(reg_BC.hi, 6); op(2, 2); break;
	case 0x78: BIT(reg_BC.hi, 7); op(2, 2); break;
	case 0x41: BIT(reg_BC.lo, 0); op(2, 2); break;
	case 0x49: BIT(reg_BC.lo, 1); op(2, 2); break;
	case 0x51: BIT(reg_BC.lo, 2); op(2, 2); break;
	case 0x59: BIT(reg_BC.lo, 3); op(2, 2); break;
	case 0x61: BIT(reg_BC.lo, 4); op(2, 2); break;
	case 0x69: BIT(reg_BC.lo, 5); op(2, 2); break;
	case 0x71: BIT(reg_BC.lo, 6); op(2, 2); break;
	case 0x79: BIT(reg_BC.lo, 7); op(2, 2); break;
	case 0x42: BIT(reg_DE.hi, 0); op(2, 2); break;
	case 0x4A: BIT(reg_DE.hi, 1); op(2, 2); break;
	case 0x52: BIT(reg_DE.hi, 2); op(2, 2); break;
	case 0x5A: BIT(reg_DE.hi, 3); op(2, 2); break;
	case 0x62: BIT(reg_DE.hi, 4); op(2, 2); break;
	case 0x6A: BIT(reg_DE.hi, 5); op(2, 2); break;
	case 0x72: BIT(reg_DE.hi, 6); op(2, 2); break;
	case 0x7A: BIT(reg_DE.hi, 7); op(2, 2); break;
	case 0x43: BIT(reg_DE.lo, 0); op(2, 2); break;
	case 0x4B: BIT(reg_DE.lo, 1); op(2, 2); break;
	case 0x53: BIT(reg_DE.lo, 2); op(2, 2); break;
	case 0x5B: BIT(reg_DE.lo, 3); op(2, 2); break;
	case 0x63: BIT(reg_DE.lo, 4); op(2, 2); break;
	case 0x6B: BIT(reg_DE.lo, 5); op(2, 2); break;
	case 0x73: BIT(reg_DE.lo, 6); op(2, 2); break;
	case 0x7B: BIT(reg_DE.lo, 7); op(2, 2); break;
	case 0x44: BIT(reg_HL.hi, 0); op(2, 2); break;
	case 0x4C: BIT(reg_HL.hi, 1); op(2, 2); break;
	case 0x54: BIT(reg_HL.hi, 2); op(2, 2); break;
	case 0x5C: BIT(reg_HL.hi, 3); op(2, 2); break;
	case 0x64: BIT(reg_HL.hi, 4); op(2, 2); break;
	case 0x6C: BIT(reg_HL.hi, 5); op(2, 2); break;
	case 0x74: BIT(reg_HL.hi, 6); op(2, 2); break;
	case 0x7C: BIT(reg_HL.hi, 7); op(2, 2); break;
	case 0x45: BIT(reg_HL.lo, 0); op(2, 2); break;
	case 0x4D: BIT(reg_HL.lo, 1); op(2, 2); break;
	case 0x55: BIT(reg_HL.lo, 2); op(2, 2); break;
	case 0x5D: BIT(reg_HL.lo, 3); op(2, 2); break;
	case 0x65: BIT(reg_HL.lo, 4); op(2, 2); break;
	case 0x6D: BIT(reg_HL.lo, 5); op(2, 2); break;
	case 0x75: BIT(reg_HL.lo, 6); op(2, 2); break;
	case 0x7D: BIT(reg_HL.lo, 7); op(2, 2); break;
	case 0x46: BIT(reg_HL.reg, 0); op(2, 3); break;
	case 0x4E: BIT(reg_HL.reg, 1); op(2, 3); break;
	case 0x56: BIT(reg_HL.reg, 2); op(2, 3); break;
	case 0x5E: BIT(reg_HL.reg, 3); op(2, 3); break;
	case 0x66: BIT(reg_HL.reg, 4); op(2, 3); break;
	case 0x6E: BIT(reg_HL.reg, 5); op(2, 3); break;
	case 0x76: BIT(reg_HL.reg, 6); op(2, 3); break;
	case 0x7E: BIT(reg_HL.reg, 7); op(2, 3); break;

	case 0xC7: SET(reg_AF.hi, 0); op(2, 2); break;
	case 0xCF: SET(reg_AF.hi, 1); op(2, 2); break;
	case 0xD7: SET(reg_AF.hi, 2); op(2, 2); break;
	case 0xDF: SET(reg_AF.hi, 3); op(2, 2); break;
	case 0xE7: SET(reg_AF.hi, 4); op(2, 2); break;
	case 0xEF: SET(reg_AF.hi, 5); op(2, 2); break;
	case 0xF7: SET(reg_AF.hi, 6); op(2, 2); break;
	case 0xFF: SET(reg_AF.hi, 7); op(2, 2); break;
	case 0xC0: SET(reg_BC.hi, 0); op(2, 2); break;
	case 0xC8: SET(reg_BC.hi, 1); op(2, 2); break;
	case 0xD0: SET(reg_BC.hi, 2); op(2, 2); break;
	case 0xD8: SET(reg_BC.hi, 3); op(2, 2); break;
	case 0xE0: SET(reg_BC.hi, 4); op(2, 2); break;
	case 0xE8: SET(reg_BC.hi, 5); op(2, 2); break;
	case 0xF0: SET(reg_BC.hi, 6); op(2, 2); break;
	case 0xF8: SET(reg_BC.hi, 7); op(2, 2); break;
	case 0xC1: SET(reg_BC.lo, 0); op(2, 2); break;
	case 0xC9: SET(reg_BC.lo, 1); op(2, 2); break;
	case 0xD1: SET(reg_BC.lo, 2); op(2, 2); break;
	case 0xD9: SET(reg_BC.lo, 3); op(2, 2); break;
	case 0xE1: SET(reg_BC.lo, 4); op(2, 2); break;
	case 0xE9: SET(reg_BC.lo, 5); op(2, 2); break;
	case 0xF1: SET(reg_BC.lo, 6); op(2, 2); break;
	case 0xF9: SET(reg_BC.lo, 7); op(2, 2); break;
	case 0xC2: SET(reg_DE.hi, 0); op(2, 2); break;
	case 0xCA: SET(reg_DE.hi, 1); op(2, 2); break;
	case 0xD2: SET(reg_DE.hi, 2); op(2, 2); break;
	case 0xDA: SET(reg_DE.hi, 3); op(2, 2); break;
	case 0xE2: SET(reg_DE.hi, 4); op(2, 2); break;
	case 0xEA: SET(reg_DE.hi, 5); op(2, 2); break;
	case 0xF2: SET(reg_DE.hi, 6); op(2, 2); break;
	case 0xFA: SET(reg_DE.hi, 7); op(2, 2); break;
	case 0xC3: SET(reg_DE.lo, 0); op(2, 2); break;
	case 0xCB: SET(reg_DE.lo, 1); op(2, 2); break;
	case 0xD3: SET(reg_DE.lo, 2); op(2, 2); break;
	case 0xDB: SET(reg_DE.lo, 3); op(2, 2); break;
	case 0xE3: SET(reg_DE.lo, 4); op(2, 2); break;
	case 0xEB: SET(reg_DE.lo, 5); op(2, 2); break;
	case 0xF3: SET(reg_DE.lo, 6); op(2, 2); break;
	case 0xFB: SET(reg_DE.lo, 7); op(2, 2); break;
	case 0xC4: SET(reg_HL.hi, 0); op(2, 2); break;
	case 0xCC: SET(reg_HL.hi, 1); op(2, 2); break;
	case 0xD4: SET(reg_HL.hi, 2); op(2, 2); break;
	case 0xDC: SET(reg_HL.hi, 3); op(2, 2); break;
	case 0xE4: SET(reg_HL.hi, 4); op(2, 2); break;
	case 0xEC: SET(reg_HL.hi, 5); op(2, 2); break;
	case 0xF4: SET(reg_HL.hi, 6); op(2, 2); break;
	case 0xFC: SET(reg_HL.hi, 7); op(2, 2); break;
	case 0xC5: SET(reg_HL.lo, 0); op(2, 2); break;
	case 0xCD: SET(reg_HL.lo, 1); op(2, 2); break;
	case 0xD5: SET(reg_HL.lo, 2); op(2, 2); break;
	case 0xDD: SET(reg_HL.lo, 3); op(2, 2); break;
	case 0xE5: SET(reg_HL.lo, 4); op(2, 2); break;
	case 0xED: SET(reg_HL.lo, 5); op(2, 2); break;
	case 0xF5: SET(reg_HL.lo, 6); op(2, 2); break;
	case 0xFD: SET(reg_HL.lo, 7); op(2, 2); break;
	case 0xC6: SET(reg_HL.reg, 0); op(2, 4); break;
	case 0xCE: SET(reg_HL.reg, 1); op(2, 4); break;
	case 0xD6: SET(reg_HL.reg, 2); op(2, 4); break;
	case 0xDE: SET(reg_HL.reg, 3); op(2, 4); break;
	case 0xE6: SET(reg_HL.reg, 4); op(2, 4); break;
	case 0xEE: SET(reg_HL.reg, 5); op(2, 4); break;
	case 0xF6: SET(reg_HL.reg, 6); op(2, 4); break;
	case 0xFE: SET(reg_HL.reg, 7); op(2, 4); break;

	case 0x87: RES(reg_AF.hi, 0); op(2, 2); break;
	case 0x8F: RES(reg_AF.hi, 1); op(2, 2); break;
	case 0x97: RES(reg_AF.hi, 2); op(2, 2); break;
	case 0x9F: RES(reg_AF.hi, 3); op(2, 2); break;
	case 0xA7: RES(reg_AF.hi, 4); op(2, 2); break;
	case 0xAF: RES(reg_AF.hi, 5); op(2, 2); break;
	case 0xB7: RES(reg_AF.hi, 6); op(2, 2); break;
	case 0xBF: RES(reg_AF.hi, 7); op(2, 2); break;
	case 0x80: RES(reg_BC.hi, 0); op(2, 2); break;
	case 0x88: RES(reg_BC.hi, 1); op(2, 2); break;
	case 0x90: RES(reg_BC.hi, 2); op(2, 2); break;
	case 0x98: RES(reg_BC.hi, 3); op(2, 2); break;
	case 0xA0: RES(reg_BC.hi, 4); op(2, 2); break;
	case 0xA8: RES(reg_BC.hi, 5); op(2, 2); break;
	case 0xB0: RES(reg_BC.hi, 6); op(2, 2); break;
	case 0xB8: RES(reg_BC.hi, 7); op(2, 2); break;
	case 0x81: RES(reg_BC.lo, 0); op(2, 2); break;
	case 0x89: RES(reg_BC.lo, 1); op(2, 2); break;
	case 0x91: RES(reg_BC.lo, 2); op(2, 2); break;
	case 0x99: RES(reg_BC.lo, 3); op(2, 2); break;
	case 0xA1: RES(reg_BC.lo, 4); op(2, 2); break;
	case 0xA9: RES(reg_BC.lo, 5); op(2, 2); break;
	case 0xB1: RES(reg_BC.lo, 6); op(2, 2); break;
	case 0xB9: RES(reg_BC.lo, 7); op(2, 2); break;
	case 0x82: RES(reg_DE.hi, 0); op(2, 2); break;
	case 0x8A: RES(reg_DE.hi, 1); op(2, 2); break;
	case 0x92: RES(reg_DE.hi, 2); op(2, 2); break;
	case 0x9A: RES(reg_DE.hi, 3); op(2, 2); break;
	case 0xA2: RES(reg_DE.hi, 4); op(2, 2); break;
	case 0xAA: RES(reg_DE.hi, 5); op(2, 2); break;
	case 0xB2: RES(reg_DE.hi, 6); op(2, 2); break;
	case 0xBA: RES(reg_DE.hi, 7); op(2, 2); break;
	case 0x83: RES(reg_DE.lo, 0); op(2, 2); break;
	case 0x8B: RES(reg_DE.lo, 1); op(2, 2); break;
	case 0x93: RES(reg_DE.lo, 2); op(2, 2); break;
	case 0x9B: RES(reg_DE.lo, 3); op(2, 2); break;
	case 0xA3: RES(reg_DE.lo, 4); op(2, 2); break;
	case 0xAB: RES(reg_DE.lo, 5); op(2, 2); break;
	case 0xB3: RES(reg_DE.lo, 6); op(2, 2); break;
	case 0xBB: RES(reg_DE.lo, 7); op(2, 2); break;
	case 0x84: RES(reg_HL.hi, 0); op(2, 2); break;
	case 0x8C: RES(reg_HL.hi, 1); op(2, 2); break;
	case 0x94: RES(reg_HL.hi, 2); op(2, 2); break;
	case 0x9C: RES(reg_HL.hi, 3); op(2, 2); break;
	case 0xA4: RES(reg_HL.hi, 4); op(2, 2); break;
	case 0xAC: RES(reg_HL.hi, 5); op(2, 2); break;
	case 0xB4: RES(reg_HL.hi, 6); op(2, 2); break;
	case 0xBC: RES(reg_HL.hi, 7); op(2, 2); break;
	case 0x85: RES(reg_HL.lo, 0); op(2, 2); break;
	case 0x8D: RES(reg_HL.lo, 1); op(2, 2); break;
	case 0x95: RES(reg_HL.lo, 2); op(2, 2); break;
	case 0x9D: RES(reg_HL.lo, 3); op(2, 2); break;
	case 0xA5: RES(reg_HL.lo, 4); op(2, 2); break;
	case 0xAD: RES(reg_HL.lo, 5); op(2, 2); break;
	case 0xB5: RES(reg_HL.lo, 6); op(2, 2); break;
	case 0xBD: RES(reg_HL.lo, 7); op(2, 2); break;
	case 0x86: RES(reg_HL.reg, 0); op(2, 4); break;
	case 0x8E: RES(reg_HL.reg, 1); op(2, 4); break;
	case 0x96: RES(reg_HL.reg, 2); op(2, 4); break;
	case 0x9E: RES(reg_HL.reg, 3); op(2, 4); break;
	case 0xA6: RES(reg_HL.reg, 4); op(2, 4); break;
	case 0xAE: RES(reg_HL.reg, 5); op(2, 4); break;
	case 0xB6: RES(reg_HL.reg, 6); op(2, 4); break;
	case 0xBE: RES(reg_HL.reg, 7); op(2, 4); break;
	}
}

void Emulator::parseOpcode(Byte code)
{
	Byte value = readMemory(reg_PC + 1);
	Byte value2 = readMemory(reg_PC + 2);

		//Uncomment for disassembler
        /*printf("A:%02X F:%c%c%c%c BC:%04X DE:%04x HL:%04x SP:%04x PC:%04x ",
               reg_AF.hi, reg_AF.lo & FLAG_ZERO ? 'Z' : '-',
               reg_AF.lo & FLAG_SUB ? 'N' : '-',
               reg_AF.lo & FLAG_HALF_CARRY ? 'H' : '-',
               reg_AF.lo & FLAG_CARRY ? 'C' : '-', reg_BC.reg, reg_DE.reg,
               reg_HL.reg, reg_SP, reg_PC, code, value, value2);
        printf(" |");
        print_instruction(reg_PC, code, value, value2);
		//printf(" Current Line: %02X", readMemory(0xFF44));
        printf("\n");*/

	// REG_D could possibly be incorrect, assumed current value from manual to match GBCPUman
	switch (code)
	{
		// 85
	case 0x7F: LD(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0x78: LD(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0x79: LD(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0x7A: LD(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0x7B: LD(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0x7C: LD(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0x7D: LD(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0x47: LD(reg_BC.hi, reg_AF.hi); op(1, 1); break;
	case 0x40: LD(reg_BC.hi, reg_BC.hi); op(1, 1); break;
	case 0x41: LD(reg_BC.hi, reg_BC.lo); op(1, 1); break;
	case 0x42: LD(reg_BC.hi, reg_DE.hi); op(1, 1); break;
	case 0x43: LD(reg_BC.hi, reg_DE.lo); op(1, 1); break;
	case 0x44: LD(reg_BC.hi, reg_HL.hi); op(1, 1); break;
	case 0x45: LD(reg_BC.hi, reg_HL.lo); op(1, 1); break;
	case 0x4F: LD(reg_BC.lo, reg_AF.hi); op(1, 1); break;
	case 0x48: LD(reg_BC.lo, reg_BC.hi); op(1, 1); break;
	case 0x49: LD(reg_BC.lo, reg_BC.lo); op(1, 1); break;
	case 0x4A: LD(reg_BC.lo, reg_DE.hi); op(1, 1); break;
	case 0x4B: LD(reg_BC.lo, reg_DE.lo); op(1, 1); break;
	case 0x4C: LD(reg_BC.lo, reg_HL.hi); op(1, 1); break;
	case 0x4D: LD(reg_BC.lo, reg_HL.lo); op(1, 1); break;
	case 0x57: LD(reg_DE.hi, reg_AF.hi); op(1, 1); break;
	case 0x50: LD(reg_DE.hi, reg_BC.hi); op(1, 1); break;
	case 0x51: LD(reg_DE.hi, reg_BC.lo); op(1, 1); break;
	case 0x52: LD(reg_DE.hi, reg_DE.hi); op(1, 1); break;
	case 0x53: LD(reg_DE.hi, reg_DE.lo); op(1, 1); break;
	case 0x54: LD(reg_DE.hi, reg_HL.hi); op(1, 1); break;
	case 0x55: LD(reg_DE.hi, reg_HL.lo); op(1, 1); break;
	case 0x5F: LD(reg_DE.lo, reg_AF.hi); op(1, 1); break;
	case 0x58: LD(reg_DE.lo, reg_BC.hi); op(1, 1); break;
	case 0x59: LD(reg_DE.lo, reg_BC.lo); op(1, 1); break;
	case 0x5A: LD(reg_DE.lo, reg_DE.hi); op(1, 1); break;
	case 0x5B: LD(reg_DE.lo, reg_DE.lo); op(1, 1); break;
	case 0x5C: LD(reg_DE.lo, reg_HL.hi); op(1, 1); break;
	case 0x5D: LD(reg_DE.lo, reg_HL.lo); op(1, 1); break;
	case 0x67: LD(reg_HL.hi, reg_AF.hi); op(1, 1); break;
	case 0x60: LD(reg_HL.hi, reg_BC.hi); op(1, 1); break;
	case 0x61: LD(reg_HL.hi, reg_BC.lo); op(1, 1); break;
	case 0x62: LD(reg_HL.hi, reg_DE.hi); op(1, 1); break;
	case 0x63: LD(reg_HL.hi, reg_DE.lo); op(1, 1); break;
	case 0x64: LD(reg_HL.hi, reg_HL.hi); op(1, 1); break;
	case 0x65: LD(reg_HL.hi, reg_HL.lo); op(1, 1); break;
	case 0x6F: LD(reg_HL.lo, reg_AF.hi); op(1, 1); break;
	case 0x68: LD(reg_HL.lo, reg_BC.hi); op(1, 1); break;
	case 0x69: LD(reg_HL.lo, reg_BC.lo); op(1, 1); break;
	case 0x6A: LD(reg_HL.lo, reg_DE.hi); op(1, 1); break;
	case 0x6B: LD(reg_HL.lo, reg_DE.lo); op(1, 1); break;
	case 0x6C: LD(reg_HL.lo, reg_HL.hi); op(1, 1); break;
	case 0x6D: LD(reg_HL.lo, reg_HL.lo); op(1, 1); break;
	case 0x3E: LD(reg_AF.hi, value); op(2, 2); break;
	case 0x06: LD(reg_BC.hi, value); op(2, 2); break;
	case 0x0E: LD(reg_BC.lo, value); op(2, 2); break;
	case 0x16: LD(reg_DE.hi, value); op(2, 2); break;
	case 0x1E: LD(reg_DE.lo, value); op(2, 2); break;
	case 0x26: LD(reg_HL.hi, value); op(2, 2); break;
	case 0x2E: LD(reg_HL.lo, value); op(2, 2); break;
	case 0x7E: LD(reg_AF.hi, reg_HL.reg); op(1, 2); break;
	case 0x46: LD(reg_BC.hi, reg_HL.reg); op(1, 2); break;
	case 0x4E: LD(reg_BC.lo, reg_HL.reg); op(1, 2); break;
	case 0x56: LD(reg_DE.hi, reg_HL.reg); op(1, 2); break;
	case 0x5E: LD(reg_DE.lo, reg_HL.reg); op(1, 2); break;
	case 0x66: LD(reg_HL.hi, reg_HL.reg); op(1, 2); break;
	case 0x6E: LD(reg_HL.lo, reg_HL.reg); op(1, 2); break;
		// 86
	case 0x77: LD(reg_HL.reg, reg_AF.hi); op(1, 2); break;
	case 0x70: LD(reg_HL.reg, reg_BC.hi); op(1, 2); break;
	case 0x71: LD(reg_HL.reg, reg_BC.lo); op(1, 2); break;
	case 0x72: LD(reg_HL.reg, reg_DE.hi); op(1, 2); break;
	case 0x73: LD(reg_HL.reg, reg_DE.lo); op(1, 2); break;
	case 0x74: LD(reg_HL.reg, reg_HL.hi); op(1, 2); break;
	case 0x75: LD(reg_HL.reg, reg_HL.lo); op(1, 2); break;
	case 0x36: LD(reg_HL.reg, value); op(2, 3); break;
	case 0x0A: LD(reg_AF.hi, reg_BC.reg); op(1, 2); break;
	case 0x1A: LD(reg_AF.hi, reg_DE.reg); op(1, 2); break;
	case 0xF2: LD(reg_AF.hi, (Address)(0xFF00 + reg_BC.lo)); op(1, 2); break;
		// 87
	case 0xE2: LD((Address)(0xFF00 + reg_BC.lo), reg_AF.hi); op(1, 2); break;
	case 0xF0: LD(reg_AF.hi, (Address)(0xFF00 + value)); op(2, 3); break; // this may need to consume 3 opbytes
	case 0xE0: LD((Address)(0xFF00 + value), reg_AF.hi); op(2, 3); break; // this also
	case 0xFA: { Address temp = (value2 << 8) | value; LD(reg_AF.hi, temp); op(3, 4); break; }// these may need swapped
	// 88
	case 0xEA: { Address temp = (value2 << 8) | value; LD(temp, reg_AF.hi); op(3, 4); break; }// these may need swapped
	case 0x2A: LD(reg_AF.hi, reg_HL.reg); reg_HL.reg += 1; op(1, 2); break;
	case 0x3A: LD(reg_AF.hi, reg_HL.reg); reg_HL.reg -= 1; op(1, 2); break;
	case 0x02: LD(reg_BC.reg, reg_AF.hi); op(1, 2); break;
	case 0x12: LD(reg_DE.reg, reg_AF.hi); op(1, 2); break;
		// 89
	case 0x22: LD(reg_HL.reg, reg_AF.hi); reg_HL.reg += 1; op(1, 2); break;
	case 0x32: LD(reg_HL.reg, reg_AF.hi); reg_HL.reg -= 1; op(1, 2); break;
		// 90
	case 0x01: LD(reg_BC, value2, value); op(3, 3); break;
	case 0x11: LD(reg_DE, value2, value); op(3, 3); break; // says DD in nintindo manual, assumed DE pair
	case 0x21: LD(reg_HL, value2, value); op(3, 3); break;
	case 0x31: reg_SP = (value2 << 8) | value; op(3, 3); break;
	case 0xF9: reg_SP = reg_HL.reg; op(1, 2); break;
	case 0xC5: PUSH(reg_BC.hi, reg_BC.lo); op(1, 4); break;
	case 0xD5: PUSH(reg_DE.hi, reg_DE.lo); op(1, 4); break;
	case 0xE5: PUSH(reg_HL.hi, reg_HL.lo); op(1, 4); break;
	case 0xF5: PUSH(reg_AF.hi, reg_AF.lo); op(1, 4); break;
		// 91
	case 0xC1: POP(reg_BC.hi, reg_BC.lo); op(1, 3); break;
	case 0xD1: POP(reg_DE.hi, reg_DE.lo); op(1, 3); break;
	case 0xE1: POP(reg_HL.hi, reg_HL.lo); op(1, 3); break;
	case 0xF1:
		POP(reg_AF.hi, reg_AF.lo);
		// After failing tests, apparently lower 4 bits of register F
		// (all flags) are set to zero.
		reg_AF.lo &= 0xF0;
		op(1, 3);
		break;
	case 0xF8: LDHL(value); op(2, 3); break;
	case 0x08: LDNN(value, value2); op(3, 5); break;
		// 92
	case 0x87: ADD(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0x80: ADD(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0x81: ADD(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0x82: ADD(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0x83: ADD(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0x84: ADD(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0x85: ADD(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xC6: ADD(reg_AF.hi, value); op(2, 2); break;
	case 0x86: ADD(reg_AF.hi, reg_HL.reg); op(1, 2); break;
	case 0x8F: ADC(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0x88: ADC(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0x89: ADC(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0x8A: ADC(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0x8B: ADC(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0x8C: ADC(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0x8D: ADC(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xCE: ADC(reg_AF.hi, value); op(2, 2); break;
	case 0x8E: ADC(reg_AF.hi, reg_HL.reg); op(1, 2); break;
		// 93
	case 0x97: SUB(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0x90: SUB(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0x91: SUB(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0x92: SUB(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0x93: SUB(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0x94: SUB(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0x95: SUB(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xD6: SUB(reg_AF.hi, value); op(2, 2); break;
	case 0x96: SUB(reg_AF.hi, reg_HL.reg); op(1, 2); break;
	case 0x9F: SBC(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0x98: SBC(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0x99: SBC(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0x9A: SBC(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0x9B: SBC(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0x9C: SBC(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0x9D: SBC(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xDE: SBC(reg_AF.hi, value); op(2, 2); break;
	case 0x9E: SBC(reg_AF.hi, reg_HL.reg); op(1, 2); break;
		// 94
	case 0xA7: AND(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0xA0: AND(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0xA1: AND(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0xA2: AND(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0xA3: AND(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0xA4: AND(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0xA5: AND(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xE6: AND(reg_AF.hi, value); op(2, 2); break;
	case 0xA6: AND(reg_AF.hi, reg_HL.reg); op(1, 2); break;
	case 0xB7: OR(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0xB0: OR(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0xB1: OR(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0xB2: OR(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0xB3: OR(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0xB4: OR(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0xB5: OR(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xF6: OR(reg_AF.hi, value); op(2, 2); break;
	case 0xB6: OR(reg_AF.hi, reg_HL.reg); op(1, 2); break;
	case 0xAF: XOR(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0xA8: XOR(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0xA9: XOR(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0xAA: XOR(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0xAB: XOR(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0xAC: XOR(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0xAD: XOR(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xEE: XOR(reg_AF.hi, value); op(2, 2); break;
	case 0xAE: XOR(reg_AF.hi, reg_HL.reg); op(1, 2); break;
		// 95 - 96
	case 0xBF: CP(reg_AF.hi, reg_AF.hi); op(1, 1); break;
	case 0xB8: CP(reg_AF.hi, reg_BC.hi); op(1, 1); break;
	case 0xB9: CP(reg_AF.hi, reg_BC.lo); op(1, 1); break;
	case 0xBA: CP(reg_AF.hi, reg_DE.hi); op(1, 1); break;
	case 0xBB: CP(reg_AF.hi, reg_DE.lo); op(1, 1); break;
	case 0xBC: CP(reg_AF.hi, reg_HL.hi); op(1, 1); break;
	case 0xBD: CP(reg_AF.hi, reg_HL.lo); op(1, 1); break;
	case 0xFE: CP(reg_AF.hi, value); op(2, 2); break;
	case 0xBE: CP(reg_AF.hi, reg_HL.reg); op(1, 2); break;
	case 0x3C: INC(reg_AF.hi); op(1, 1); break;
	case 0x04: INC(reg_BC.hi); op(1, 1); break;
	case 0x0C: INC(reg_BC.lo); op(1, 1); break;
	case 0x14: INC(reg_DE.hi); op(1, 1); break;
	case 0x1C: INC(reg_DE.lo); op(1, 1); break;
	case 0x24: INC(reg_HL.hi); op(1, 1); break;
	case 0x2C: INC(reg_HL.lo); op(1, 1); break;
	case 0x34: INC(reg_HL.reg); op(1, 3); break;
	case 0x3D: DEC(reg_AF.hi); op(1, 1); break;
	case 0x05: DEC(reg_BC.hi); op(1, 1); break;
	case 0x0D: DEC(reg_BC.lo); op(1, 1); break;
	case 0x15: DEC(reg_DE.hi); op(1, 1); break;
	case 0x1D: DEC(reg_DE.lo); op(1, 1); break;
	case 0x25: DEC(reg_HL.hi); op(1, 1); break;
	case 0x2D: DEC(reg_HL.lo); op(1, 1); break;
	case 0x35: DEC(reg_HL.reg); op(1, 3); break;
		// 97
	case 0x09: ADDHL(reg_BC); op(1, 2); break;
	case 0x19: ADDHL(reg_DE); op(1, 2); break;
	case 0x29: ADDHL(reg_HL); op(1, 2); break;
	case 0x39: ADDHLSP();                 op(1, 2); break;
	case 0xE8: ADDSP(value); op(2, 4); break;
	case 0x03: INC(reg_BC); op(1, 2); break;
	case 0x13: INC(reg_DE); op(1, 2); break;
	case 0x23: INC(reg_HL); op(1, 2); break;
	case 0x33: INCSP();                 op(1, 2); break;
	case 0x0B: DEC(reg_BC); op(1, 2); break;
	case 0x1B: DEC(reg_DE); op(1, 2); break;
	case 0x2B: DEC(reg_HL); op(1, 2); break;
	case 0x3B: DECSP();                 op(1, 2); break;
		// 98
	/*case 0x07: RL(reg_AF.hi, false, false);  op(1, 1); break; // RLCA
	case 0x17: RL(reg_AF.hi, true, false);   op(1, 1); break; // RLA
	case 0x0F: RR(reg_AF.hi, false, false);  op(1, 1); break;
	case 0x1F: RR(reg_AF.hi, true, false);   op(1, 1); break;*/
	case 0x07: RL(reg_AF.hi, false);  op(1, 1); break; // RLCA
	case 0x17: RL(reg_AF.hi, true);   op(1, 1); break; // RLA
	case 0x0F: RR(reg_AF.hi, false);  op(1, 1); break;
	case 0x1F: RR(reg_AF.hi, true);   op(1, 1); break;
		// 99 - 104
	case 0xCB: parseBitOp(value); break;
		// 105
	case 0xC3: { Register temp; temp.reg = (value2 << 8 | value); op(3, 3); JP(temp); break; } // 1 cycle added in JP();
	case 0xC2: { Register temp; temp.reg = (value2 << 8 | value); op(3, 3); JPNZ(temp); break; }
	case 0xCA: { Register temp; temp.reg = (value2 << 8 | value); op(3, 3); JPZ(temp);  break; }
	case 0xD2: { Register temp; temp.reg = (value2 << 8 | value); op(3, 3); JPNC(temp); break; }
	case 0xDA: { Register temp; temp.reg = (value2 << 8 | value); op(3, 3); JPC(temp);  break; }
		// 106
	case 0x18: op(2, 2); JR(value); break; // 1 cycle added in JR();
	case 0x20: op(2, 2); JRNZ(value); break;
	case 0x28: op(2, 2); JRZ(value); break;
	case 0x30: op(2, 2); JRNC(value); break;
	case 0x38: op(2, 2); JRC(value); break;
	case 0xE9: op(1, 1); JPHL(); break;
		// 107
	case 0xCD: op(3, 3); CALL(value, value2); break; // 3 cycles added in CALL();
	case 0xC4: op(3, 3); CALLNZ(value, value2); break; // op() must be called before CALL() because it relies on updated PC
	case 0xCC: op(3, 3); CALLZ(value, value2); break;
	case 0xD4: op(3, 3); CALLNC(value, value2); break;
	case 0xDC: op(3, 3); CALLC(value, value2); break;
		// 108
	case 0xC9: op(1, 1); RET(); break; // 3 cycles added in RET();
	case 0xC0: op(1, 2); RETNZ(); break;
	case 0xC8: op(1, 2); RETZ(); break;
	case 0xD0: op(1, 2); RETNC(); break;
	case 0xD8: op(1, 2); RETC(); break;
	case 0xD9: op(1, 1); RETI(); break;
		// 109
	case 0xC7: op(1, 4); RST(0x00); break; // RST() relies on updated PC, op() must be first
	case 0xCF: op(1, 4); RST(0x08); break;
	case 0xD7: op(1, 4); RST(0x10); break;
	case 0xDF: op(1, 4); RST(0x18); break;
	case 0xE7: op(1, 4); RST(0x20); break;
	case 0xEF: op(1, 4); RST(0x28); break;
	case 0xF7: op(1, 4); RST(0x30); break;
	case 0xFF: op(1, 4); RST(0x38); break;
		// 110-111
	case 0x27: DAA(); op(1, 1); break;
	case 0x2F: CPL(); op(1, 1); break;
	case 0x00: NOP(); op(1, 1); break;

		// GBCPUMAN
	case 0xF3: DI(); op(1, 1); break; // Disable interrupts
	case 0xFB: EI(); op(1, 1); break; // Enable interrupts
	// 112
	case 0x76: HALT(); op(1, 1); break; //In HALT, op(-1,1) is called or subtract so the op(1,1) cancels that instruction out & PC remains the same --> done for cycles?
	// case 0x10: STOP(); op(2, 1); break; // UNIMPLEMENTED

	// Pandocs
	case 0x37: SCF(); op(1, 1); break;
	case 0x3F: CCF(); op(1, 1); break;

	default: op(1, 0); break;
	}
}

void Emulator::executeNextOpcode()
{
	Address addr = readMemory(reg_PC);
	parseOpcode(addr);
}
