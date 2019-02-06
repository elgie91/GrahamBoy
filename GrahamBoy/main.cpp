#include <SDL.h>
#include <iostream>
#include <fstream> //for file


#include "types.h"
#include "Emulator.h"

using namespace std;

Byte cartridgeMemory[0x200000];

int main(int argc, char *args[])
{
	
	Emulator gameBoy;
	
	//char * juba = "jfoaf"; --> doesn't work, gives an error!
	
	//char game[] = "C:/Users/lemar/source/repos/Test/x64/Release/gb-test-roms-master/cpu_instrs/individual/02-interrupts.gb";  //"C:/Users/lemar/source/repos/GrahamBoy/Super Mario Land (World).gb";
	//char game[] = "C:/Users/lemar/source/repos/Test/x64/Release/gb-test-roms-master/cpu_instrs/cpu_instrs.gb";  
	char game[] =  "C:/Users/lemar/source/repos/GrahamBoy/Super Mario Land (World).gb";
	//char game[] =  "C:/Users/lemar/source/repos/GrahamBoy/Tetris (World) (Rev A).gb";
	//char game[] =  "C:/Users/lemar/source/repos/Test/x64/Release/kirby.gb";
	gameBoy.loadRom(game);
	
	gameBoy.run();
	return 0;
}