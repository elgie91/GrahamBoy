#include "Emulator.h"
#include "types.h"
#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

/*The screen resolution is 160x144 meaning there are 144 visible scanlines. The Gameboy 
draws each scanline one at a time starting from 0 to 153, this means there are 144 
visible scanlines and 8 invisible scanlines. When the current scanline is between 144 
and 153 this is the vertical blank period. The current scanline is stored in register 
address 0xFF44. The pandocs tell us that it takes 456 cpu clock cycles to draw one scanline 
and move onto the next, so we will need a counter to know when to move onto the next line, 
we'll call this scanlineCounter. Just like the timer and divider registers we can control 
the scanline counter by subtracting its value by the amount of clock cycles the last opcode 
took to exectue.*/
void Emulator::updateGraphics(int cyc)
{
	setLCDStatus();

	if (isLCDEnabled())
		scanlineCounter -= cyc;
	else
		return;

	if (scanlineCounter <= 0)
	{
		scanlineCounter = 456;
		
		/*increase the scanline --> cannot use WriteMemory because when the game tries
		to write to 0xFF44 it resets the current scaline to 0*/
		Byte currentLine = readMemory(0xFF44);
		memory[0xFF44] += 1;
		

		//entered VBlank period
		if (currentLine == 144)
			requestInterrupt(INTERRUPT_VBLANK);

		else if (currentLine > 153) //went past 153, need to reset
			memory[0xFF44] = 0; //reset the scanline

		else if (currentLine < 144) //not @ end or b/w VBlank period
			drawScanLine();

	}

	return;
}

void Emulator::renderScreen()
{
	SDL_Rect rec;
	rec.w = width * 5; rec.h = height * 5; rec.x = 0; rec.y = 0;

	//need to use surfaces b/c overlapping background/sprites/window together which apparently can't be done on textures
	SDL_Surface * bgSurf = SDL_CreateRGBSurfaceFrom((void*) bgData, 160, 144, 32, 160 * sizeof(int), 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

	SDL_Surface * windowSurf = SDL_CreateRGBSurfaceFrom((void*) windowData, 160, 144, 32, 160 * sizeof (int), 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	//SDL_SetColorKey(spriteSurf, SDL_TRUE, SDL_MapRGB(spriteSurf->format,255, 255, 255));
	//SDL_Surface * windowSurf = SDL_CreateRGBSurfaceFrom(windowArray, 160, 144, 32, 160 * sizeof(int), 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	//SDL_SetColorKey(windowSurf, SDL_TRUE, SDL_MapRGBA(windowSurf->format, 0xFF, 0xFF, 0xFF, 255));
	SDL_Surface * screenSurface = SDL_GetWindowSurface(window);
	//SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));

	//Apply the image --> blit onto the screenSurface
	SDL_BlitScaled(bgSurf, NULL, screenSurface, &rec);
	//SDL_BlitScaled(spriteSurf, NULL, screenSurface, &rec);
	SDL_BlitScaled(windowSurf, NULL, screenSurface, &rec);

	//windowTexture = SDL_CreateTextureFromSurface(renderer, screenSurface);

	//Update the surface
	SDL_UpdateWindowSurface(window);

	SDL_FreeSurface(bgSurf);
	//SDL_FreeSurface(spriteSurf);
	SDL_FreeSurface(windowSurf);
	SDL_FreeSurface(screenSurface);
	
	return;
}

void Emulator::initDisplay()
{
	int scale = 5;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		exit(-1);

	window = SDL_CreateWindow("Gameboy Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width * scale, height * scale, SDL_WINDOW_SHOWN);
	if (window == NULL)
		exit(-1);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if (renderer == NULL)
		exit(-1);
	
	SDL_RenderSetScale(renderer, scale, scale); 

	//texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, width, height);
	
	//Attempt to set texture filtering to linear
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		printf("Warning: Linear texture filtering not enabled!");

	memset(bgData, 0xFFFFFFFF, width * height * sizeof(int)); //RGBA 255,255,255,255 ==> White
	memset(windowData, 0x00000000, width * height * sizeof(int)); //RGBA 0 ==> Black
	memset(spriteData, 0x00000000, width * height * sizeof(int)); //RGBA 0 ==> Black


	return;
}

/* The memory address 0xFF41 holds the current status of the LCD. The LCD goes through 4 different modes. These are 
"V-Blank Period", "H-Blank Period", "Searching Sprite Attributes" and "Transferring Data to LCD Driver". 
Bit 1 and 0 of the lcd status at address 0xFF41 reflects the current LCD mode like so:
00: H-Blank
01: V-Blank
10: Searching Sprites Atts
11: Transfering Data to LCD Driver

When starting a new scanline the lcd status is set to 2, it then moves on to 3 and then to 0. It then goes back to and 
continues then pattern until the v-blank period starts where it stays on mode 1. When the vblank period ends it goes 
back to 2 and continues this pattern over and over. As previously mentioned it takes 456 clock cycles to draw one scanline 
before moving onto the next. This can be split down into different sections which will represent the different modes. 
Mode 2 (Searching Sprites Atts) will take the first 80 of the 456 clock cycles. Mode 3 (Transfering to LCD Driver) will 
take 172 clock cycles of the 456 and the remaining clock cycles of the 456 is for Mode 0 (H-Blank).

When the LCD status changes its mode to either Mode 0, 1 or 2 then this can cause an LCD Interupt Request to happen. Bits 
3, 4 and 5 of the LCD Status register (0xFF41) are interrupt enabled flags. These bits are set by the game not the emulator 
and they represent the following:
Bit 3: Mode 0 Interupt Enabled
Bit 4: Mode 1 Interupt Enabled
Bit 5: Mode 2 Interupt Enabled

So when the mode changes to 0,1 or 2 then if the corresponding bit 3,4,5 is set then an LCD interupt is requested. This is 
only tested when the LCD mode changes to 0,1 or 2 and not the duration of these modes. One important part to emulate with 
the lcd modes is when the lcd is disabled the mode must be set to mode 1. You also need to reset the scanlineCounter and 
current scanline*/
void Emulator::setLCDStatus()
{
	Byte status = readMemory(0xFF41);

	if (isLCDEnabled() == false) //set to mode 1 & change current scanline to 0
	{
		scanlineCounter = 456; //reset scanlineCounter
		memory[0xFF44] = 0; //could also do writeMemory(0xFF44,any #);
		status &= 0xFC; //? keep all the bits except for bits 0 & 1 - the mode bits
		
		status = bitSet(status, 0); //the second digit is already = 0
		writeMemory(0xFF41, status);
		return;
	}

	Byte currentLine = readMemory(0xFF44);
	Byte currentMode = status & 0x3; //take the first two bits

	Byte newMode;
	bool reqInterrupt = false;

	//check if Vblank period
	if (currentLine >= 144)
	{
		//change to mode 1
		newMode = 1;
		status = bitSet(status, 0);
		status = bitClear(status, 1); //turn off the second digit of the mode
		reqInterrupt = testBit(status, 4);
	}

	else
	{
		int mode2Bounds = 456 - 80; 
		int mode3Bounds = mode2Bounds - 172;

		if (scanlineCounter >= mode2Bounds) //scanline counter is being declined by the # of CPU cycles in updateGraphics
		{
			newMode = 2;
			status = bitSet(status, 1);
			status = bitClear(status, 0); //turn off the first digit of the mode
			reqInterrupt = testBit(status, 5);
		}
		
		else if (scanlineCounter >= mode3Bounds) //no request interrupt for mode 3
		{
			newMode = 3;
			status = bitSet(status, 1);
			status = bitSet(status, 0);
		}

		else
		{
			newMode = 0;
			status = bitClear(status, 0);
			status = bitClear(status, 1);
			reqInterrupt = testBit(status, 3);
		}
	}

	if (reqInterrupt && (newMode != currentMode)) //used to verify that there is an actual mode change
		requestInterrupt(INTERRUPT_LCD);

	/*The last part of the LCD status register (0xFF41) is the Coincidence flag. Basically Bit 2 of the 
	status register is set to 1 if register (0xFF44) = (0xFF45) otherwise it is set to 0. If the conicidence 
	flag (bit 2) is set and the conincidence interupt enabled flag (bit 6) is set then an LCD Interupt 
	is requested. The conicidence flag means the current scanline (0xFF44) is the same as a scanline 
	the game is interested in (0xFF45). The reason why the game would be interested in the current 
	scanline is to do special effects. So when 0xFF44 == 0xFF45 then an interupt can be requested to let 
	the game know that the values are the same.*/
	if (readMemory(0xFF44) == readMemory(0xFF45))
	{
		status = bitSet(status, 2);
		if (testBit(status, 6))
			requestInterrupt(INTERRUPT_LCD);
	}
	else
		status = bitClear(status, 2);

	writeMemory(0xFF41, status);
	return;
}

//Bit 7 of the LCD control register 0xFF40 is responsible for enabling/disabling the LCD
bool Emulator::isLCDEnabled() const
{
	return testBit(readMemory(0xFF40), 7);
}

/*The CPU can only access the Sprite Attributes table during the duration of one of the LCD modes 
(mode 2). The Direct Memory Access (DMA) is a way of copying data to the sprite RAM at the 
appropriate time removing all responsibility from the main program. Called when attempting to write to 
memory address 0xFF46

 the destination address of the DMA is the sprite RAM between memory adddress (0xFE00-0xFE9F) which 
 means that a total of 0xA0 bytes will be copied to this region. The source address is represented by 
 the data being written to address 0xFF46 except this value is the source address divided by 100. 
 So to get the correct start address it is the data being written to * 100 */
void Emulator::doDMATransfer(Byte data)
{
	//Word address = (data << 6) + (data << 5) + (data << 2); //https://stackoverflow.com/questions/7286226/bitshift-to-multiply-by-any-number
	Word address = data << 8; //? is the same as multiplying by 100

	for (int i = 0; i < 0xA0; i++)
		writeMemory(0xFE00 + i, readMemory(address + i));
}

/*Real resolution is 256x256 (32x32 tiles). The visual display can show any 160x144 pixels of the 256x256 background, 
this allows for scrolling the viewing area over the background. Additionally, as having a 256x256 background and a 
160x144 viewing the display the gameboy has a window which appears above the background but behind the sprites 
(unless the attributes of the sprite specify otherwise). The purpose of the window is to put a fixed panel 
over the background that does not scroll. For example some games have a panel on the screen which displays the 
characters health and collected items, and this panel does not scroll with the background when the character moves. 
This is the window.*/

/*Breakdown of the 8-bit LCD register 0xFF40:
Bit 7 - LCD Display Enable (0=Off, 1=On)
Bit 6 - Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
Bit 5 - Window Display Enable (0=Off, 1=On)
Bit 4 - BG & Window Tile Data Select (0=8800-97FF, 1=8000-8FFF)
Bit 3 - BG Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
Bit 2 - OBJ (Sprite) Size (0=8x8, 1=8x16)
Bit 1 - OBJ (Sprite) Display Enable (0=Off, 1=On)
Bit 0 - BG Display (0=Off, 1=On)

Bit 7: I have already discussed Bit7. Basically it says if the lcd is enabled, if not we dont draw anything. 
       This is already handled in the UpdateGraphics function.
Bit 6: This is where to read to read the tile identity number to draw onto the window
Bit 5: If this is set to 0 then the window is not enabled so we dont draw it
Bit 4: You use the identity number for both the window and the background tiles that need to be draw to the 
       screen here to get the data of the tile that needs to be displayed. The important thing to remember 
	   about this bit is that if it is set to 0 (i.e. read from address 0x8800) then the tile identity number 
	   we looked up is actually a signed byte not unsigned
Bit 3: This is the same as Bit6 but for the background not the window
Bit 2: This is the size of the sprites that need to draw. Unlike tiles that are always 8x8 sprites can be 8x16
Bit 1: Same as Bit5 but for sprites
Bit 0: Same as Bit5 and 1 but for the background */
void Emulator::drawScanLine()
{
	Byte control = readMemory(0xFF40);

	if (testBit(control, 0))
		renderBackground();
	
	if (testBit(control, 5))
		renderWindow();
		
	if (testBit(control, 1))
		renderSprites();
	
	renderScreen(); //does the actual drawing
	return;
}

void Emulator::renderBackground()
{
	Address tileLocation = 0, backgroundLocation = 0;
	Address offset;
	Byte lcdControl = readMemory(0xFF40);
	Byte currentScanline = readMemory(0xFF44);
	bool unsig = true;

	backgroundLocation = testBit(lcdControl, 3) ? 0x9C00 : 0x9800;
	tileLocation = testBit(lcdControl, 4) ? 0x8000 : 0x8800;
	
	//check tile data
	if (!testBit(lcdControl, 4))
		unsig = false; //using unsigned values
	

	/*ScrollY (0xFF42): The Y Position of the 256x256 pixel BACKGROUND where to start drawing the viewing area from
	ScrollX (0xFF43): The X Position of the BACKGROUND to start drawing the viewing area from*/
	Byte scrollY = readMemory(0xFF42);
	Byte scrollX = readMemory(0xFF43);

	// For each pixel in the 160x1 scanline:
	// 1. Calculate where the pixel resides in the overall 256x256 background map
	// 2. Get the tile ID where that pixel is located
	// 3. Get the pixel color based on that coordinate relative to the 8x8 tile grid
	// 4. Plot pixel in 160x144 display view
	
	int y = currentScanline;
	
	// Iterate from left to right of display screen (x = 0 -> 160)
	for (int x = 0; x < 160; x++)
	{
		//1. Calculate where the pixel resides in the overall 256x256 background map (including the offset of scroll)
		int map_x = (int)scrollX + x;
		int map_y = (int)scrollY + y;

		// wrap around if the map_x is > than the 256x256 background map
		map_x = (map_x > 256) ? (map_x - 256) : map_x;
		map_y = (map_y > 256) ? (map_y - 256) : map_y;

		//2. Get the tile ID where that pixel is located
		int tile_col = map_x / 8;
		int tile_row = map_y / 8;
		int tile_map_id = tile_col + (tile_row * 32);

		Address backAddr = backgroundLocation + tile_map_id;
		Byte tile_id = readMemory(backAddr);

		// 3. Get the pixel color based on that coordinate relative to the 8x8 tile grid
		// 4. Plot pixel in 160x144 display view
		int tile_x_pixel = map_x % 8;
		int tile_y_pixel = map_y % 8;

		//Invert the x pixels b/c they are stored backwards
		tile_x_pixel = abs(tile_x_pixel - 7);

		offset = (unsig) ? (tile_id * 16 + tileLocation) : (Address) (((Byte_Signed)tile_id) * 16 + tileLocation);
		
		//Palette Bytes are stored as two bytes per pixel
		Byte high = readMemory(offset + (tile_y_pixel * 2) + 1);
		Byte low = readMemory(offset + (tile_y_pixel * 2));

		//now have the colour id get the actual colour from palette 0xFF47
		Byte palette = readMemory(0xFF47);
		uint32_t colourID = getColour(palette, high, low, tile_x_pixel, false);
		int red = 0, green = 0, blue = 0;
		
		
		//setup RGB values
		switch (colourID)
		{
		case WHITE: red = blue = green = 0xFF; break;
		case LIGHT_GREY: red = green = blue = 0xCC; break;
		case DARK_GREY: red = green = blue = 0x77; break;
		default: break;
		}

		bgData[y * 160 + x] = colourID;

	}
}

void Emulator::renderWindow()
{
	Address tileLocation = 0, windowLocation = 0;
	Address offset;
	Byte lcdControl = readMemory(0xFF40);
	Byte currentScanline = readMemory(0xFF44);
	bool unsig = true;

	//Get current window tile map
	windowLocation = testBit(lcdControl, 6) ? 0x9C00 : 0x9800;

	tileLocation = testBit(lcdControl, 4) ? 0x8000 : 0x8800;

	//check tile data
	if (!testBit(lcdControl, 4))
		unsig = false; //using unsigned values

	/*WindowY (0xFF4A): The Y Position of the VIEWING AREA to start drawing the window from
	WindowX (0xFF4B): The X Positions -7 of the VIEWING AREA to start drawing the window from */
	Byte windowY = readMemory(0xFF4A);
	Byte windowX = readMemory(0xFF4B) - 7;

	// For each pixel in the 160x1 scanline:
	// 1. Calculate where the pixel resides in the overall 256x256 background map
	// 2. Get the tile ID where that pixel is located
	// 3. Get the pixel color based on that coordinate relative to the 8x8 tile grid
	// 4. Plot pixel in 160x144 display view

	int y = currentScanline;

	// Iterate from left to right of display screen (x = 0 -> 160)
	for (int x = 0; x < 160; x++)
	{
		// 1. Calculate where the pixel resides in the overall 256x256 background map
		// WINDOW IS RELATIVE TO THE SCREEN
		// Shift X & Y pixels based on window register value
		int display_x = x + (int) windowX;
		int display_y = y;

		//2. Get the tile ID where that pixel is located
		int tile_col = x / 8;
		int tile_row = (y - windowY) / 8; //?
		int tile_map_id = tile_col + (tile_row * 32);

		Address windowAddr = windowLocation + tile_map_id;
		Byte tile_id = readMemory(windowAddr);

		// 3. Get the pixel color based on that coordinate relative to the 8x8 tile grid
		// 4. Plot pixel in 160x144 display view
		int tile_x_pixel = x % 8;
		int tile_y_pixel = y % 8;


		//Invert the x pixels b/c they are stored backwards
		tile_x_pixel = abs(tile_x_pixel - 7);
		
		if (currentScanline < windowY) //set x,y to black and transparent?
		{
			windowData[currentScanline*160 + x] = 0x00000000;
			
		}

		else
		{
			if (display_x >= 160 || display_x < 0) return;
			if (display_y >= 144 || display_y < 0) return;
			
			offset = (unsig) ? (tile_id * 16 + tileLocation) : (Address)(((Byte_Signed)tile_id) * 16 + tileLocation);

			//Palette Bytes are stored as two bytes per pixel
			Byte high = readMemory(offset + (tile_y_pixel * 2) + 1);
			Byte low = readMemory(offset + (tile_y_pixel * 2));

			//now have the colour id get the actual colour from palette 0xFF47
			Byte palette = readMemory(0xFF47);
			uint32_t colourID = getColour(palette, low, high, tile_x_pixel, false);
			int red = 0, green = 0, blue = 0, alpha = 0xFF;


			//setup RGB values
			switch (colourID)
			{
			case WHITE: red = blue = green = 0xFF; break;
			case LIGHT_GREY: red = green = blue = 0xCC; break;
			case DARK_GREY: red = green = blue = 0x77; break;
			default: break;
			}

			windowData[display_x + display_y*160] = colourID;
		}

	}
}

/*The gameboy has two regions of memory for the background layout which is shared by the window. 
The memory regions are 0x9800-0x9BFF and 0x9C00-9FFF. We need to check bit 3 of the lcd contol 
register to see which region we are using for the background and bit 6 for the window. 
Each byte in the memory region is a tile identification number of what needs to be drawn. This 
identification number is used to lookup the tile data in video ram so we know how to draw it.*/

/*The sprite data is located in memory address 0x8000-0x8FFF which means the sprite identifiers 
are all unsigned values which makes finding them easier. There are 40 tiles located in memory 
region 0x8000-0x8FFF and we need to scan through them all and check their attributes to find 
where they need to be rendered. The sprite attributes are found in the sprite attribute table 
located in memory region 0xFE00-0xFE9F. In this memory region each sprite has 4 bytes of 
attributes associtated to it, these are:

0: Sprite Y Position: Position of the sprite on the Y axis of the viewing display minus 16
1: Sprite X Position: Position of the sprite on the X axis of the viewing display minus 8
2: Pattern number: This is the sprite identifier used for looking up the sprite data in 
	memory region 0x8000-0x8FFF
3: Attributes: These are the attributes of the sprite, discussed later.

A sprite can either be 8x8 pixels or 8x16 pixels, this can be determined by the sprites 
attributes. This is a break down of the sprites attributes:

Bit7: Sprite to Background Priority
Bit6: Y flip
Bit5: X flip
Bit4: Palette number
Bit3: Not used in standard gameboy
Bit2-0: Not used in standard gameboy */
void Emulator::renderSprites()
{
	return;
	Byte lcdControl = readMemory(0xFF40);
	bool use8x16 = false;
	
	if (testBit(lcdControl, 2)) //bit 2 of 0xFF40 controls the sprite size
		use8x16 = true;

	for (int sprite = 0; sprite < 40; sprite++)
	{
		//sprite occupies 4 bytes in the sprite attribute table
		Byte index = sprite * 4;
		Byte yPos = readMemory(0xFE00 + index) - 16;
		Byte xPos = readMemory(0xFE00 + index + 1) - 8;
		Byte tileLocation = readMemory(0xFE00 + index + 2);
		Byte attributes = readMemory(0xFE00 + index + 3);

		bool yFlip = testBit(attributes, 6);
		bool xFlip = testBit(attributes, 5);

		int scanline = readMemory(0xFF44);

		int ySize;
		ySize = (use8x16) ? 16 : 8;

		/*check if sprite intercepts w/ the scanline
		y-------------
		|				<-- yPos
		|				
		|===============] <-- scanline
		|				<--yPos + ySize
		y------------- */
		/*if ( scanline >= yPos && (scanline < (yPos + ySize)) )
		{
			int line = scanline - yPos; //determine how far into the sprite are we drawing

			
			if (yFlip) //read the sprite in backwards in the y axis if there's a flip
			{
				line -= ySize;
				line *= -1; //?
			}

			line *= 2; //represented as two bytes in memory
			Word dataAddress = 0x8000 + (tileLocation * 16) + line;
			Byte data1 = readMemory(dataAddress);
			Byte data2 = readMemory(dataAddress + 1);

			//pixel 0 in the tile is bit 7 of data1 & data2 pixel 1 is bit 6 etc....
			for (int tilePixel = 7; tilePixel >= 0; tilePixel--) //easier to read from right left
			{
				int colourBit = tilePixel;
				if (xFlip)
				{
					colourBit -= 7;
					colourBit *= -1;
				}



				//combine data2 & data1 to get the colour id for this pixel in the tile
				int colourNum = getBitVal(data2, colourBit);
				colourNum <<= 1;
				colourNum |= getBitVal(data1, colourBit);

				Word colourAddress = testBit(attributes, 4) ? 0xFF49 : 0xFF48;
				Byte col = getColour(colourNum, colourAddress);

				if (col == WHITE) //white is transparent for sprites
					continue;
				
				int red = 0, green = 0, blue = 0;

				//setup RGB values
				switch (col)
				{
				case WHITE: red = green = blue = 0xFF; break;
				case LIGHT_GREY: red = green = blue = 0xCC; break;
				case DARK_GREY: red = green = blue = 0x77; break;
				default: red = green = blue = 0; break;
				}

				//reverse the pixel so it is correct orientation
				int xPix = 0 - tilePixel;
				xPix += 7;

				int pixel = xPos + xPix;

				//safety check
				if (scanline < 0 || scanline > 143 || pixel < 0 || pixel > 159)
					continue;

				screenData[scanline][pixel][0] = red;
				screenData[scanline][pixel][1] = green;
				screenData[scanline][pixel][2] = blue;
			}
		}*/
	}
}

int Emulator::getColour(Byte palette, Byte top, Byte bottom, int bit, bool isSprite)
{
	
	// Figure out what colors to apply to each color code based on the palette data
	Byte colorShade3 = palette & 0x03; //extract bits 7 & 6
	Byte colorShade2 = (palette & 0x0C) >> 2; //extract bits 5 & 4
	Byte colorShade1 = (palette & 0x30) >> 4; //extract bits 3 & 2
	Byte colorShade0 = (palette & 0xC0) >> 6; //extract bits 1 & 0
	
	// Get color code from the two defining bytes
	Byte first = (Byte)testBit(top, bit);
	Byte second = (Byte)testBit(bottom, bit);
	Byte colorCode = (second << 1) | first;
	
	switch (colorCode)
	{
		case 0: return (isSprite)? 0xFFFFFFFF : WHITE; break;
		case 1: return LIGHT_GREY; break;
		case 2: return DARK_GREY; break;
		case 3: return BLACK; break;
		default: return 0xFFFFFF; break;
	}

}
