#include "Emulator.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <SDL.h>

//#define OPCODES
Emulator::Emulator()
{
	memset(cartridgeMemory, 0, sizeof(cartridgeMemory));
	memset(ramBank, 0, sizeof(ramBank));
	quit = false;

	reg_AF.reg = 0x01B0;
	reg_BC.reg = 0x0013;
	reg_DE.reg = 0x00D8;
	reg_HL.reg = 0x014D;
	reg_SP = 0xFFFE;
	reg_PC = 0x0100;

	memory[0xFF05] = 0x00;
	memory[0xFF06] = 0x00;
	memory[0xFF07] = 0x00;
	memory[0xFF10] = 0x80;
	memory[0xFF11] = 0xBF;
	memory[0xFF12] = 0xF3;
	memory[0xFF14] = 0xBF;
	memory[0xFF16] = 0x3F;
	memory[0xFF17] = 0x00;
	memory[0xFF19] = 0xBF;
	memory[0xFF1A] = 0x7F;
	memory[0xFF1B] = 0xFF;
	memory[0xFF1C] = 0x9F;
	memory[0xFF1E] = 0xBF;
	memory[0xFF20] = 0xFF;
	memory[0xFF21] = 0x00;
	memory[0xFF22] = 0x00;
	memory[0xFF23] = 0xBF;
	memory[0xFF24] = 0x77;
	memory[0xFF25] = 0xF3;
	memory[0xFF26] = 0xF1;
	memory[0xFF40] = 0x91;
	memory[0xFF42] = 0x00;
	memory[0xFF43] = 0x00;
	memory[0xFF45] = 0x00;
	memory[0xFF47] = 0xFC;
	memory[0xFF48] = 0xFF;
	memory[0xFF49] = 0xFF;
	memory[0xFF4A] = 0x00;
	memory[0xFF4B] = 0x00;
	memory[0xFFFF] = 0x00;

	m_MBC1 = m_MBC2 = false;
	enableRam = false;
	romBanking = true; //defaults to true
	interruptMasterEnable = true;
	halted = false;

	
	currentRamBank = 0; //values are from 0-3 --> 4 ram banks
	//RAM Banking is not used in MBC2! Therefore m_CurrentRAMBank will always be 0!

	num_cycles = 0;

	initDisplay();
}

void Emulator::loadRom(char * location)
{
	FILE *in;
	fopen_s(&in, location, "rb");
	fread(cartridgeMemory, 1, 0x200000, in);
	fclose(in);

	/* To detect what ROM mode the game is you have to read memory 0x147 after the game has been loaded into memory. 
	If 0x147 is 0 then the game has no memory banking (like tetris), however if it is 1,2 or 3 
	then it is MBC1 and if it is 5 or 6 then it is MBC2.*/
	switch (cartridgeMemory[0x147]) //1-3 = mbc1 5-6 = mbc2
	{
		case 1: case 2: case 3: m_MBC1 = true; break;
		case 5: case 6: m_MBC2 = true; break;
		default: break;

	}
	
	memcpy(&memory[0], &cartridgeMemory[0], 0x8000);
	currentRomBank = 1;
}

void Emulator::run()
{
	/* According to game pan docs site the amount of clock cycles the gameboy can exectue every second 
	is 4194304 which means that if each frame we update the emulator 60 times a second the each frame 
	will execute 69905(4194304/60) clock cycles. This will ensure the emulator is run at the correct 
	speed.
	*/
	int cyclesThisUpdate = 0;

#ifdef OPCODES
	FILE *fp;
	fopen_s(&fp, "C:/Users/lemar/Desktop/Home/New folder/C++/gramma.txt", "w+");
#endif

	while (!quit)
	{
		handleEvents();

		while (cyclesThisUpdate <= MAXCYCLES)
		{
			
			/*#ifdef TEST
			fprintf(fp, "PC: %4X\n", reg_PC);
			#endif*/

#ifdef OPCODES
			Byte code = readMemory(reg_PC); 
			Address addr = reg_PC;
			addr++;
			Opcode temp = readMemory(addr);
			if (code == 0xCB)
			{
				
				fprintf(fp, "%04X: " //X means uppercase hex
					"A:%02x "
					"B:%02x "
					"C:%02x "
					"D:%02x "
					"E:%02x "
					"F:%02x "
					"H:%02x "
					"L:%02x "
					"SP:%04x "
					"Opcode:CB %02x "
					"Memory: %02X\n",
					reg_PC, reg_AF.hi, reg_BC.hi, reg_BC.lo, reg_DE.hi, reg_DE.lo, reg_AF.lo, reg_HL.hi, reg_HL.lo, reg_SP, temp, readMemory(temp));
			}
			else 
				fprintf(fp, "%04X: " //X means uppercase hex
					"A:%02x "
					"B:%02x "
					"C:%02x "
					"D:%02x "
					"E:%02x "
					"F:%02x "
					"H:%02x "
					"L:%02x "
					"SP:%04x "
					"Opcode:%02x "
					"Memory: %02X\n",
					reg_PC, reg_AF.hi, reg_BC.hi, reg_BC.lo, reg_DE.hi, reg_DE.lo, reg_AF.lo, reg_HL.hi, reg_HL.lo, reg_SP, code, readMemory(temp));
			
#endif // DEBUG
			executeNextOpcode();
			cyclesThisUpdate += num_cycles;
			
			/*timers and graphics are being passed how many clock cycles 
			the opcode took so they can update at the same rate as the cpu*/
			updateTimers(num_cycles); 
			updateGraphics(num_cycles);

			handleInterrupts();

			num_cycles = 0;

		}

		cyclesThisUpdate = 0;

	}

	#ifdef TEST
		fclose(fp);
	#endif
}


/*If IsClockEnabled() returns false then the timer does not reset itself, neither does the timercounter 
but they both just pause until it is enabled again. SetClockFrequency's purpose will be to reset timerCounter 
upon reaching zero to the correct value for the current clock frequency so it can start counting down at the 
correct rate again. The rest of the code just increments the current timer (TIMA) value and checks to see if 
it is about to overflow. If it does overflow then it resets the timer(TIMA) to the value in the timer modulator
(TMA) and requests a timer interupt. */
void Emulator::updateTimers(int cyc)
{
	doDividerRegisters(cyc);

	//the clock must be enabled to update the clock
	if (isClockEnabled()) //checks a setting in the timer controller (TMC) which pauses or resumes the timer counting.
	{
		timerCounter -= cyc;

		if (timerCounter <= 0) //enough cpu cycles have passed to update the timer
		{
			setClockFreq(); //reset timerCounter to the correct value for the current frequency

			if (readMemory(TIMA) == 255) //check if the timer about to overflow
			{
				writeMemory(TIMA, readMemory(TMA)); //reset the timer to the value in the TMA
				requestInterrupt(INTERRUPT_TIMER);
			}

			else
				writeMemory(TIMA, readMemory(TIMA) + 1);
		}
	}
	
	return;
} //in simple words based on the timerCounter (CLOCK/freq), we either update the time +1 or if timer is about to overflow, reset

/*The way the Divider Register works is it continually counts up from 0 to 255 and then when it overflows it 
starts from 0 again. It does not cause an interupt when it overflows and it cannot be paused 
like the timers. It counts up at a frequency of 16382 which means every 256 CPU clock cycles 
the divider register needs to increment. We need another int counter like timerCounter to 
keep track of when it needs to increment, this is called dividerCounter which initially 
is set to 0 and constantly increments to 255 then starts again. The Divider Register is found 
at register address 0xFF04.*/
void Emulator::doDividerRegisters(int cyc)
{
	dividerCounter += cyc;
	if (dividerCounter >= 255)
	{
		dividerCounter = 0;
		memory[0xFF04] += 1; //cannot write to the divider register b/c whenever the game tries to do so, reset to 0.
	}
}

void Emulator::setClockFreq()
{
	Byte freq = readMemory(TMC) & 0x3;

	switch (freq)
	{
		case 0x00: frequency = 4096; break;
		case 0x01: frequency = 262144; break;
		case 0x10: frequency = 65536; break;
		case 0x11: frequency = 16384; break;
		default: frequency = 4096; break;
	}

	timerCounter = CLOCK / frequency;
}

/*The timer controller (TMC) is a 3 bit register. Bit 1 and 0 combine together to specify 
which frequency the timer should increment at. This is the mapping:
00: 4096 Hz
01: 262144 Hz
10: 65536 Hz
11: 16384 Hz
Bit 2 specifies whether the timer is enabled(1) or disabled(0).*/
bool Emulator::isClockEnabled()
{
	return testBit(readMemory(TMC), 2);
}

//clock freq is combo of bit 0 & bit1
Byte Emulator::getClockFreq() const
{
	return readMemory(TMC) & 0x3;
}

/*Call this whenever an event happens that needs to request an interupt*/
void Emulator::requestInterrupt(int id)
{
	Byte request = readMemory(0xFF0F); //IR register
	request = bitSet(request, id); //set the corresponding BIT
	writeMemory(0xFF0F, request); //write it back

	return;
}

void Emulator::handleInterrupts() //halted variable allows the reg_PC to be finally increased pass the halt instruction
{
	bool IE_set = (readMemory(0xFFFF) > 0) ? true : false; //check if IE = 1
	bool IF_set = (readMemory(0xFF0F) > 0) ? true : false; //check if IF = 1

	switch (interruptMasterEnable)
	{
	case true:
		if (IF_set && IE_set) //There was an interrupt b/c IF & IE were set
		{
			if (halted) //check if halted so we can pass that instruction
			{
				halted = false;
				reg_PC += 1; //move pass HALT instruction
			}

			for (Byte i = 0; i < 5; i++) //service the interrupts
			{
				Byte IF = readMemory(0xFF0F); Byte IE = readMemory(0xFFFF);
				if (testBit(IF,i) && testBit(IE,i))
					serviceInterrupt(i);
			}
		}
		break;

	default:
		if (IF_set && IE_set) //There was an interrupt b/c IF & IE were set
		{
			if (halted) //check if halted so we can pass that instruction
			{
				halted = false;
				reg_PC += 1; //move pass HALT instruction
			}
			// don't service any interrupts --> HALT bug
		}
		break;
	}
}

void Emulator::serviceInterrupt(int interrupt)
{
	interruptMasterEnable = false; // don't allow any interrupts while servicing current one
	Byte request = readMemory(0xFF0F); //IR register
	request = bitClear(request, interrupt); //clear the corresponding BIT
	writeMemory(0xFF0F, request); //write it back

	// Push current execution address to stack
	writeMemory(--reg_SP, highByte(reg_PC));
	writeMemory(--reg_SP, lowByte(reg_PC));
	

	//printf("Interrupt %d\n", interrupt);
	switch (interrupt)
	{
	case INTERRUPT_VBLANK: reg_PC = 0x40; break;
	case INTERRUPT_LCD:   reg_PC = 0x48; break;
	case INTERRUPT_TIMER:  reg_PC = 0x50; break;
	case INTERRUPT_SERIAL: reg_PC = 0x58; break;
	case INTERRUPT_JOYPAD: reg_PC = 0x60; break;
	}
	return;
}

void Emulator::handleEvents()
{
	SDL_Event e; 

	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
			quit = true;
	}
}