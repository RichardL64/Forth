/*
    ForthIODispatcher.h
    R.A.Lincoln   October 2026

		Dispatch In/Out opcodes to implementations

		Loosely based on various approaches to I/O including C64 device number allocations
		The model assumes devices can access the VM memory.

		VM Memory addresses are converted to/from native memory pointers for the implementations

*/

#pragma once
#include "ForthHostFileSystem.h"
#include "ForthStringBlitter.h"


//	Device port mapping
//
#define IO_KEYBOARD 		0x00											// ?key

#define IO_TAPE					0x01


//	Terminal
//
#define	IO_TERMINAL			0x02											// Primary emit/key I/O route, local/usb
#define IO_REMOTE				0x20											// Remote terminal over wifi
#define IO_DCC_EX				0x21											// Dcc-ex native commands terminal


//	Screen
//
#define IO_SCREEN				0x03


//	File
//
#define IO_FILE_CMD			0x80											// File system
																									//	01 = open, 02 = close, 03 = read, 04 = write, 05 = seek
																									//	11 = read
																									//	21 = write
																									//	31 = r/w
																									//	41 = append
#define IO_FILE_ID  		0x81
#define IO_FILE_NAME 		0x82
#define IO_FILE_BUFFER 	0x83
#define IO_FILE_SIZE		0x84


//	String
//
#define IO_STRING_CMD		0xc0											// String blitter
																									//	01 = compare
																									//	02 = intelligent move
																									//	03 = scanf	scan for a character
																									//	04 = scanu	scan until a character list
																									//	05 = scanw	scan while a character list
#define IO_STRING_A			0xc1											// String A
#define IO_STRING_B			0xc2											// String B
#define IO_STRING_C			0xc3											// Count for move or char for scanf


//	C64 hardware
//
#define	IO_VIC					0xd0											// Bit 15-8: Register, 7-0: value
#define IO_SID					0xd4
#define	IO_COLOUR				0xd8
#define IO_CIA1					0xdc
#define IO_CIA2					0xdd
#define IO_1						0xde
#define IO_2						0xdf


#define	IO_STATUS				0xff


//
//	Route In/Out opcodes to the port handler
//
class IODispatcherClass {
	HostFileSystemClass hostFS;
	StringBlitterClass stringBlitter;

private:

	uint8_t*	memBot;
	uint8_t*	memTop;

	uint16_t	status;																	// Shared status bits


public:

	//	Constructor - take memory bounds to allow validation of pointer values
	//
	IODispatcherClass(uint8_t* bot, uint8_t* top)
		: memBot(bot), memTop(top), stringBlitter((char*)bot, (char*)top) {
	}


	//	IN - Read a word from a port
	//
	uint16_t handleIn(uint8_t port) {

  	switch (port) {
    case IO_STATUS:																	//	Shared ready flags 1 bit per device
      return status;

    case IO_KEYBOARD:
			return -1;

		case IO_TERMINAL:
			return getchar();

		case IO_FILE_CMD:																//	Returns status of the last action
			return hostFS.getStatus();
		case IO_FILE_ID:
			return hostFS.getID();

		case IO_STRING_CMD:															// Returns the result of the last action
			return stringBlitter.getResult();
		case IO_STRING_A:
			return (uint8_t*)stringBlitter.getA() - memBot;
		case IO_STRING_B:
			return (uint8_t*)stringBlitter.getB() - memBot;
		case IO_STRING_C:
			return stringBlitter.getC();

	  default:																				//  Floating bus from unmapped ports
      return 0xFFFF;
  	}
	}


	//	OUT - Write a word to a port
	//
	void handleOut(uint8_t port, uint16_t val) {

  	switch (port) {
		case IO_STATUS:																	//	Shared ready flags
			status = val;
			break;

    case IO_TERMINAL:																// Character stream
			putchar((char)(val & 0xFF));
    	fflush(stdout);
      break;

		case IO_FILE_CMD:																// File device
			hostFS.setCmd(val);
		  break;
		case IO_FILE_ID:
			hostFS.setID(val);
		  break;
		case IO_FILE_NAME:
			hostFS.setName((char*)(memBot + val));
		  break;
		case IO_FILE_BUFFER:
			hostFS.setBuffer((char*)(memBot + val));
		  break;
		case IO_FILE_SIZE:
			hostFS.setSize(val);
		  break;

		case IO_STRING_CMD:															// String blitter
			stringBlitter.setCmd(val);
		  break;
		case IO_STRING_A:
			stringBlitter.setA((char*)(memBot + val));
		  break;
		case IO_STRING_B:
			stringBlitter.setB((char*)(memBot + val));
		  break;
		case IO_STRING_C:
			stringBlitter.setC(val);
		  break;


    default:	          														// Ignore writes to unmapped ports
      break;
    }
	}

};





