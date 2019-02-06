#pragma once
#include <stdio.h>
#include <SDL.h>
#include "types.h"

#define TIMA 0xFF05 //actual timer which counts up @ a certain frequency
#define TMA 0xFF06 //timer modulator (sets the frequency)
#define TMC 0xFF07 //timer controller (enables/disables timer)

class Emulator
{
public:
	Emulator();
	void loadRom(char * location);
	void run();
	
	void parseBitOp(Byte code);
	void parseOpcode(Byte code);

	void executeNextOpcode();
	void updateTimers(int cyc);
	void updateGraphics(int cyc);
	void renderScreen();

private:
	bool quit;
	int CLOCK = 4194304;
	int frameRate = 60;
	int frequency = 4096;
	const int MAXCYCLES = CLOCK / frameRate;

	/*the cpu clock speed runs at 4194304Hz so if we know the current timer frequency 
	we can work out how many clock cycles need to of passed until we increment our timer register
	So if the current frequency is 4096Hz then our timer counter will be 1024 (CLOCKSPEED/4096). 
	Means that "for every 1024 clock cycles our cpu does, our timer should increment itself once"*/
	int timerCounter = CLOCK / frequency;
	int dividerCounter = 0;


	int scanlineCounter = 456; 
	int bgData [144 * 160]; //first part is the screen y axis. The second is the x axis and the third is the colour (three elements for red, green and blue), last = alpha
	Byte windowData [144 * 160]; //first part is the screen y axis. The second is the x axis and the third is the colour (three elements for red, green and blue), last = alpha
	Byte spriteData [144][160][4]; //first part is the screen y axis. The second is the x axis and the third is the colour (three elements for red, green and blue), last = alpha
	
	SDL_Rect rec;
	SDL_Surface * bgSurf;
	SDL_Surface * windowSurf;
	SDL_Surface * spriteSurf;
	SDL_Texture * texture;


	Byte memory[0x10000];
	
	Register reg_AF;
	Register reg_BC;
	Register reg_DE;
	Register reg_HL;
	Word reg_SP;
	Word reg_PC;

	bool m_MBC1;
	bool m_MBC2;

	int num_cycles;
	
	/*Need a variable declaration to specify which ROM bank is currently loaded into internal memory address 
	0x4000 - 0x7FFF. As ROM Bank 0 is fixed into memory address 0x0 - 0x3FFF 
	this variable should never be 0, it should be at least 1. We need to initialize this variable on emulator 
	load to 1*/

	Byte currentRomBank;

	/*Cartridge memory address 0x148 tells how much RAM banks the game has.The maximum is 4. The size of 1 
	RAM bank is 0x2000 bytes so if we have an array of size 0x8000 this is enough space for all the RAM banks.
	Like ROM banking we also need a variable to point at which RAM bank the game is using between values of 0 - 3.*/

	Byte ramBank[0x8000];
	Byte currentRamBank;
	void createRamBanks(int numBanks);

	bool enableRam;
	bool romBanking; //variable is responsible for how to act when the game writes to memory address 0x4000-0x6000
	bool interruptMasterEnable; 
	bool halted;

	void writeMemory(Word address, Byte data);
	Byte readMemory(Word address) const;

	void handleBanking(Word address, Byte data);
	void doRamBankEnable(Word address, Byte data);
	void changeRamBank(Byte data);
	void changeLowRomBank(Byte data);
	void changeHighRomBank(Byte data);
	void changeRomRamMode(Byte data);

	void doDividerRegisters(int cyc);
	void setClockFreq();
	bool isClockEnabled();
	Byte getClockFreq() const;


	/*There are two special registers to do with the state of interrupt handling in the gameboy.
	The first is the Interrupt Enabled register (aka IE) located at memory addres 0xFFFF. This is
	written to by the game to enable and disable specific interrupts. Interrupt would only get
	serviced if its corresponding bit is enabled in the Interrupt Enabled Register (aka IE).
	If it is enabled then the interrupt would be serviced but if it is not enabled the interrupt
	would sit pending until it becomes enabled or the game resets its request. The second interrupt
	register is the Interrupt Request Register (aka IF) located at memory address 0xFF0F. Using the
	timer interrupt as an example, whenever the timer overflows it requests its interrupt by setting
	its corresponding bit in the Interrupt Request Register where it will stay set until servicing
	of the interrupt begins or the game resets it.  Also, interrupt handling is the master interupt
	enabled switch. This is not part of game memory and is just a bool that the game sets on and off.
	When this bool is set to false no interrupts will get serviced.

	Below is the criteria needed to handle an interupt:
	1. When an event occurs that needs to trigger its interrupt it must make an interrupt request by
	   setting its corresponding bit in the Interupt Request Register (0xFF0F).
	2. An interrupt can only be serviced if the Interupt Master Enable switch is set to true
	3. If the above two conditions are true and their is no other interrupt with a higher priority
       awaiting to be serviced then it checks the Interrupt Enabled Register(0xFFFF) to see if its
       corresponding interrupt bit is set to 1 to allow servicing of this particular interrupt.

	Step 1 gets set to true and false by the EI and DI cpu instructions respectively. If either
	step 2 or 3 is false then the interrupt continues to wait until both 2 and 3 are true and the
	game hasn't turned the interrupt request off by resetting its corresponding interrupt bit the
	Interrupt Request Register(0xFF0F).*/
	void requestInterrupt(int id);
	void handleInterrupts();
	void serviceInterrupt(int interrupt);

	void setLCDStatus();
	bool isLCDEnabled() const;
	void drawScanLine();
	void doDMATransfer(Byte data);
	
	void renderBackground();
	void renderWindow();
	void renderSprites();
	int getColour(Byte palette, Byte top, Byte bottom, int bit, bool isSprite);
	void initDisplay();
	void destroySDL();

	SDL_Window * window;
	SDL_Renderer * renderer;
	SDL_Surface * screenSurface;
	SDL_Event e;

	Byte joypadButtons;
	Byte joypadDirections;
	void handleEvents();
	void keyPressed(int key);
	void keyReleased(int key);
	Byte getJoypadState() const;


	//CPU INSTRS
	void op(int pc, int cycle);
	void set_flag(int flag, bool value);
	void LD(Byte & destination, Byte value);
	void LD(Byte& destination, Address addr);
	void LD(Address addr, Byte value);
	void LD(Register & pair, Byte upper, Byte lower);
	void LDHL(Byte value);
	void LDNN(Byte low, Byte high);
	void PUSH(Byte high, Byte low);
	void POP(Byte & high, Byte & low);
	void ADD(Byte & target, Byte value);
	void ADD(Byte & target, Address addr);
	void ADC(Byte & target, Byte value);
	void ADC(Byte & target, Address addr);
	void SUB(Byte & target, Byte value);
	void SUB(Byte & target, Address addr);
	void SBC(Byte & target, Byte value);
	void SBC(Byte & target, Address addr);
	void AND(Byte & target, Byte value);
	void AND(Byte & target, Address addr);
	void OR(Byte & target, Byte value);
	void OR(Byte & target, Address addr);
	void XOR(Byte & target, Byte value);
	void XOR(Byte & target, Address addr);
	void CP(Byte & target, Byte value);
	void CP(Byte & target, Address addr);
	void INC(Byte & target);
	void INC(Address addr);
	void DEC(Byte & target);
	void DEC(Address addr);
	void ADD16(Word target, Word value);
	void ADDHL(Register & pair);
	void ADDHLSP();
	void ADDSP(Byte value);
	void INC(Register & pair);
	void INCSP();
	void DEC(Register & pair);
	void DECSP();
	void SL(Byte & target);
	void SL(Address addr);
	void SR(Byte & target, bool include_top_bit);
	void SR(Address addr, bool include_top_bit);
	void RL(Byte & target, bool carry, bool zero_flag = false);
	void RL(Address addr, bool carry);
	void RR(Byte & target, bool carry, bool zero_flag = false);
	void RR(Address addr, bool carry);
	void SRA(Byte & target);
	void SRA(Address addr);
	void SRL(Byte & target);
	void SRL(Address addr);
	void SWAP(Byte & target);
	void SWAP(Address addr);
	void BIT(Byte target, int bit);
	void BIT(Address addr, int bit);
	void SET(Byte & target, int bit);
	void SET(Address addr, int bit);
	void RES(Byte & target, int bit);
	void RES(Address addr, int bit);
	void SCF();
	void CCF();
	void JP(Register target);
	void JPNZ(Register target);
	void JPZ(Register target);
	void JPNC(Register target);
	void JPC(Register target);
	void JR(Byte value);
	void JRNZ(Byte value);
	void JRZ(Byte value);
	void JRNC(Byte value);
	void JRC(Byte value);
	void JPHL();
	void CALL(Byte low, Byte high);
	void CALLNZ(Byte low, Byte high);
	void CALLZ(Byte low, Byte high);
	void CALLNC(Byte low, Byte high);
	void CALLC(Byte low, Byte high);
	void RET();
	void RETI();
	void RETNZ();
	void RETZ();
	void RETNC();
	void RETC();
	void RST(Address addr);
	void DAA();
	void CPL();
	void NOP();
	void HALT();
	void STOP();
	void DI();
	void EI();
};