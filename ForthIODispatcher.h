/*
    ForthIODispatcher.h
    R.A.Lincoln   October 2026

		Called from the VM IN & OUT opcodes
		Dispatch I/O to implementations

		Loosely based on various approaches to I/O including C64 device allocations
		The model assumes devices can access the VM memory, i.e. passing pointers.

*/

#pragma once
#include "ForthHostFileSystem.h"


//	Device port mapping
//
#define IO_KEYBOARD 		0x00											// ?key

#define IO_TAPE					0x01

#define	IO_TERMINAL			0x02											// Primary emit/accept I/O route, local/usb
#define IO_REMOTE				0x20											// Remote terminal over wifi
#define IO_DCC_EX				0x21											// Dcc-ex native commands terminal

#define IO_SCREEN				0x03											//

#define IO_FILE_CMD			0x80											// File Low		command 01 = open, 02 = close, 03 = read, 04 = write
																									//		   			mode		11 = read
																									//										21 = write
																									//										31 = r/w
																									//										41 = append
#define IO_FILE_ID  		0x81
#define IO_FILE_NAME 		0x82
#define IO_FILE_BUFFER 	0x83
#define IO_FILE_SIZE		0x84


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

private:
	uint16_t	status;																	// Shared status bits

public:

	//	IN - Read a word from a port and trigger action
	//
	uint16_t handleIn(uint8_t port, uint8_t* memory) {

  	switch (port) {
    	case IO_STATUS:																//	Shared ready flags 1 bit per device
      	return status;

    	case IO_KEYBOARD:

			case IO_TERMINAL:
				return getchar();

			case IO_FILE_CMD:															//	File read from CMD triggers action
				return hostFS.run();

	    default:																			//  Floating bus from unmapped ports
      	return 0xFFFF;
  	}
	}


	//	OUT - Write a word to a port
	//
	void handleOut(uint8_t port, uint16_t val, uint8_t* memory) {

  	switch (port) {
		case IO_STATUS:																	//	Shared ready flags
			status = val;
			break;

    case IO_TERMINAL:																// Terminal device
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
			hostFS.setName((char*)&memory[val]);
		  break;
		case IO_FILE_BUFFER:
			hostFS.setBuffer((char*)&memory[val]);
		  break;
		case IO_FILE_SIZE:
			hostFS.setSize(val);
		  break;

    default:	          													// Ignore writes to unmapped ports
      break;
    }
	}

} IODispatcher;





