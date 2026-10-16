/*
    ForthStringBlitter.h
    R.A.Lincoln   October 2026

		String blitter
		Manage all things null terminated string, moving, comparison, searches etc
		Assume these are available on the host in most environments faster than running in the vm

		IF it was ever silicon these would either move into forth implementation or a microcontroller

*/

#pragma once
#include <cstring>
#include <cstdint>


#define strCompare				0x01
#define strMove						0x02
#define strScanFor				0x03
#define strScanUntil			0x04
#define strScanWhile			0x05


class StringBlitterClass {


private:

	char*		memBot;
	char*		memTop;

	uint16_t		cmdReg;
	char*				aReg;
	char*				bReg;
	uint16_t		cReg;
	uint16_t		resultReg;


public:

	//	Constructor - take memory bounds to allow validation of pointer values
	//
	StringBlitterClass(char* bot, char* top)
	: memBot(bot), memTop(top) {
	}


	//	Internal register access
	//
	void setCmd(uint16_t cmd) {
		cmdReg = cmd;
		run();																		// Setting the command runs it
	}

	void setA(char* addr) {
		aReg = addr;
	}

	void setB(char* addr) {
		bReg = addr;
	}

	void setC(uint16_t c) {
		cReg = c;
	}

	char* getA() {
		return aReg;
	}

	char* getB() {
		return bReg;
	}

	uint16_t getC() {
		return cReg;
	}

	uint16_t getResult() {
		return resultReg;
	}


	//	Run the command
	//
	void run() {
		switch(cmdReg) {

		case strCompare:
		  resultReg = std::strcmp(aReg, bReg);						// 0: same, -ve: A<B, +ve: A>B
			break;

		case strMove:
			move();
			break;

		case strScanFor:
			scanFor();
			break;
		case strScanWhile:
			scanWhile();
			break;
		case strScanUntil:
			scanUntil();
			break;
		default:
			break;
		}
	}


private:


	//	Move C bytes from A to B
	//	Intelligent move, works for overlaps too
	//
	void move() {
		if (aReg + cReg > memTop) {														// Clamp source if it would read past the end of memory
    	cReg = memTop - aReg;
		}
		if (bReg + cReg > memTop) {														// Clamp destination if it would write past the end of memory
	    cReg = memTop - bReg;
	  }

	  std::memmove(bReg, aReg, cReg);
	  resultReg = cReg;																				// Bytes moved
	}

	//	Scan the string A for char C
	//	Result is the length of the string found
	//	A is advanced to the found char or end of string
	//
	void scanFor() {
  	resultReg = 0;																				// Posit not found

    char* found = std::strchr(aReg, (char)cReg);

    if (found != nullptr) {
      resultReg = found - aReg; 													// Length
      aReg = found;             													// Advance A directly to the matched char

    } else {																							// not found
    	resultReg = 0;
    	aReg += strlen(aReg);																// Advance A to the end of string null
    }
	}


	//	Scan the string A while chars are in the pattern B
	//	Result is the length of string found
	//	A is advanced to the first char that doesn't match, or end of string
	//
	void scanWhile() {
    char* start = aReg;

		// std::cout << "1scanW >" << aReg << "<  >" << bReg << "< \n";

    while (*aReg != '\0') {
        if (std::strchr(bReg, *aReg) == nullptr) {
            break; 																			// -> Stop when character is NOT in the set
        }
        aReg++;
    }

		// std::cout << "2scanW " << aReg << ", " << start << ", " << aReg - start << "\n";

    resultReg = aReg - start;                      			// Length
	}


	//	Scan the string A until chars are in the pattern B
	//	Result is the length of string found
	//	A is advanced to the first char that matches, or end of string
	//
	void scanUntil() {
    char* start = aReg;

		// std::cout << "1scanU >" << aReg << "<  >" << bReg << "< \n";

    while (*aReg != '\0') {
        if (std::strchr(bReg, *aReg) != nullptr) {
            break; 																			// -> Stop when character is NOT in the set
        }
        aReg++;
    }

		// std::cout << "2scanU " << aReg << ", " << start << ", " << aReg - start << "\n";

    resultReg = aReg - start;                      			// Length
	}

};


