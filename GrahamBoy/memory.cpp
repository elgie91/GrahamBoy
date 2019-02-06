#include "Emulator.h"

/*Internal memory only has room for 0x8000 (0x0000 - 0x7FFF) of the game memory. 
However most games are bigger in size than 0x8000 which is why memory banking is needed. 
The first 0x4000 bytes is where the first 0x4000 (0000-0x3FFF) (Rom bank 0) bytes of the game memory is placed. 
The second 0x4000 (0x4000 - 0x7FFF) is also for the game memory however this area is the rom bank area 
so depending on the current state of the game this memory area will contain different game memory banks. 
Memory area 0xA000-0xBFFF is also for banking but this time it's ram banking. The ECHO memory region (0xE000-0xFDFF) 
is quite different because any data written here is also written in the equivelent ram memory region 0xC000-0xDDFF. 
Hence why it is called echo. */

/* First 0x8000 bytes are read only so nothing should ever get written there. Also anything that gets written 
to ECHO memory needs to be reflected in work RAM. Also when reading from one of the banks it is important 
that it gets read from the correct bank. */
void Emulator::writeMemory(Word address, Byte data)
{
	if (address < 0x8000)
		handleBanking(address, data);

	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (enableRam)
		{

			if (m_MBC1)
			{
				Word newAddress = address - 0xA000;
				ramBank[newAddress + (currentRomBank * 0x2000)] = data;
			}
		}

		else if (m_MBC2 && address < 0xA200)
		{
			Word newAddress = address - 0xA000;
			ramBank[newAddress + (currentRomBank * 0x2000)] = data;
		}
	}

	// we're writing to internal RAM, so this is the cho
	else if (address >= 0xC000 && address <= 0xDFFF)
		memory[address] = data;

	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		memory[address] = data;
		writeMemory(address - 0x2000, data); //do the echoing
	}

	else if (address >= 0xFEA0 && address <= 0xFEFF)
		return; //not usable

	/*As stated earlier the frequency defaults to 4096Hz but we need to monitor a way of checking if it has changed.
	The easiest way to do this is by editing our WriteMemory function to detect if the game is trying to change the
	timer controller. If the game is changing the timer controller then we need to check if the current clock
	frequency is different to what the game is trying to change it to and if it is then we much reset the timer
	counter so it counts at the new frequency*/
	else if (address == TMC)
	{
		Byte currentFreq = getClockFreq();
		memory[address] = data; //update the frequency
		Byte newFreq = getClockFreq();

		if (newFreq != currentFreq)
			setClockFreq();
	}

	else if (address == 0xFF04) //divider register
		memory[address] = 0;

	// reset the current scanline if the game tries to write to it
	else if (address == 0xFF44)
	{
		memory[address] = 0;
	}

	else if (address == 0xFF45)
		memory[address] = data;

	/*the CPU can only access the Sprite Attributes table during the duration of one of the LCD modes (mode 2).
	The Direct Memory Access (DMA) is a way of copying data to the sprite RAM at the appropriate time removing all
	responsibility from the main program*/
	else if (address == 0xFF46)
	{
		doDMATransfer(data);
	}

	//restricted area
	else if ((address >= 0xFF4C) && (address <= 0xFF7F))
		return;

	else if (address == 0xFF00) //Joypad
		memory[address] = data & 0x30;

	else
		memory[address] = data;

	return;
}

// read memory should never modify member variables hence const
Byte Emulator::readMemory(Word address) const
{
	
	//reading from the cartridge rom bank
	if (address >= 0x4000 && address <= 0x7FFF)
	{
		Word newAddress = address - 0x4000;
		/*4000 in HEX = 4 * 2^16 = 16KB, each bank is 16KB, so we jump by 4000 for each bank chunk*/
		return cartridgeMemory[newAddress + ( currentRomBank * 0x4000)]; 
	}

	//reading from the cartridge ram bank
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		Word newAddress = address - 0xA000;
		/*4000 in HEX = 2 * 2^16 = 2KB, each bank is 2KB, so we jump by 2000 for each bank chunk*/
		return ramBank[newAddress + (currentRamBank * 0x2000)]; 
	}

	else if (address == 0xFF00)
		return getJoypadState();

	else
		return memory[address];
}

/*If the address is between 0x2000 - 0x4000 then it is a ROM bank change.
If the address is 0x4000 - 0x6000 then it is a RAM bank change or a ROM 
bank change depending on what current ROM / RAM mode is selected.
If the value is between 0x0 - 0x2000 then it enables RAM bank writing*/
void Emulator::handleBanking(Word address, Byte data)
{
	//ram enabling
	if (address < 0x2000)
	{
		if (m_MBC1 || m_MBC2)
		{
			doRamBankEnable(address, data);
		}
	}

	//rom banking
	else if (address >= 0x2000 && address < 0x4000)
	{
		if (m_MBC1 || m_MBC2)
			changeLowRomBank(data);
	}

	//RAM or ROM bank change
	else if (address >= 0x4000 && address < 0x6000)
	{
		if (m_MBC1) //no ram bank in MBC2 so use ram bank 0
		{
			if (romBanking)
				changeHighRomBank(data);
			else
				changeRamBank(data);
		}
	}

	else if (address >= 0x6000 && address < 0x8000)
	{
		if (m_MBC1)
			changeRomRamMode(data);
	}

	return;
}

/*In order to write to RAM banks the game must specifically request that ram bank writing is enabled. 
It does this by attempting to write to internal ROM address between 0 and 0x2000. 
For MBC1 if the lower nibble of the data the game is writing to memory is 0xA then ram bank writing is 
enabled else if the lower nibble is 0 then ram bank writing is disabled. MBC2 is exactly the same 
except there is an additional clause that bit 4 of the address byte must be 0.*/
void Emulator::doRamBankEnable(Word address, Byte data)
{
	if (m_MBC2)
	{
		if (testBit(address, 4))
			return;
	}

	Byte testData = data & 0xF;

	if (testData == 0xA)
		enableRam = true;
	else if (testData == 0x0)
		enableRam = false;

	return;
}

/*To change RAM Banks in MBC1 the game must again write to memory address 0x4000-0x6000 but 
this time m_RomBanking must be false, the current ram bank gets set to the lower 2 bits of the data*/
void Emulator::changeRamBank(Byte data)
{
	currentRamBank = data & 0x3;
}

/*If the memory bank is MBC1 then there is two parts to changing the current rom bank. 
The first way is if the game writes to memory address 0x2000-0x3FFF then it changes the 
lower 5 bits of the current rom bank but not bits 5 and 6. The second way is writing to
memory address 0x4000-0x5FFF during rombanking mode which only changes bits 5 and 6 not bits 0-4. 
So combining these two methods you can change bits 0-6 of which rom bank is currently in use. 
However if the game is using MBC2 then this is much easier. If the game writes to address 
0x2000-0x3FFF then the current ram bank changes bits 0-3 and bits 5-6 are never set. 
This means writing to address 0x4000-0x5FFF in MBC2 mode does nothing. 
This section explains what happens when the game writes to memory address 0x2000-0x3FFF*/
void Emulator::changeLowRomBank(Byte data)
{
	if (m_MBC2)
	{
		currentRomBank = data & 0xF;
		if (currentRomBank == 0)
			currentRomBank += 1;
		return;
	}

	Byte bankID = data & 0x1F; //bottom 5 bits represents bank # from 0x00 - 0x1F;
	currentRomBank = (currentRomBank & 0xE0) | bankID; //keep the top 3 bits of the current romBank & combine the bank id

	switch (currentRomBank) //prevents these banks from being accessed --> pandocs
	{
	case 0x00: case 0x20: case 0x40: case 0x60: currentRomBank += 1; break;
	}

	return;
}

void Emulator::changeHighRomBank(Byte data)
{
	currentRamBank = 0;
	Byte bankID = data & 0xE0; //keep the top 3 bits of the bank #
	currentRomBank = (currentRomBank & 0x1F) | bankID; //keep the bottom 5 bits of current bank & combine top 3 digits of bank id

	switch (currentRomBank) //prevents these banks from being accessed --> pandocs
	{
	case 0x00: case 0x20: case 0x40: case 0x60: currentRomBank += 1; break;
	}

	return;
}

/* romBanking variable is responsible for how to act when the game writes to memory address 0x4000 - 0x6000
This variable defaults to true but is changes during MBC1 mode when the game writes to memory address 
0x6000-0x8000. If the least significant bit of the data being written to this address range is 0 then 
romBanking is set to true, otherwise it is set to false meaning there is about to be a ram bank change. 
It is important to set currentRAMBank to 0 whenever you set romBanking to true because the gameboy can only
use rambank 0 in this mode*/
void Emulator::changeRomRamMode(Byte data)
{
	Byte newData = data & 0x1;
	romBanking = (newData == 0) ? true : false;
	
	if (romBanking)
		currentRamBank = 0;

	return;
}

void Emulator::createRamBanks(int numBanks)
{
	return;
}

