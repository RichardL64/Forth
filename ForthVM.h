/*
    ForthVM.h
    R.A.Lincoln   September 2026

    Proof of concept Forth based scripting engine
    To be accessible via terminal connectivity on the DCC-Ex AP network
    Target to provide interactive capability similar to EXRAIL

    Potential target to make it compatible with running on 65xx C64

    64k     byte system
    127     max opcodes

    Subroutine threaded, word aligned 16 bit instructions
    All strings null terminated instead of counted

    Registers

    PC	Program counter
    I		Instruction
    A		Address

    SP	Stack pointer - points to stack item
    T		Top of stack

    RSP	Return stack pointer
    L		Link address (top of return stack)


    Instruction layout

    16bit   Words
            Branch and Link
                    15-1    Address
                    0       0

            Packed opcodes
                    15-9    Opcode 2
                    8       1
                    7-1     Opcode 1
                    1       1

            Small parameter
                    15-8    Parameter 0-255
                    7-1     Opcode 1
                    1       1
*/

#pragma once
#include <cstring>


//  Primitive setup
//	Allocate the next opcode, fill in the opcode table, open the function definition
//    op_..name..   is the function
//    opc_..name..  is the opcode - start at 1 odd numbers only
//
#define PRIMITIVE_(name, counter_val) \
    static constexpr uint8_t opc_##name = (counter_val * 2) + 1; \
    struct Init_##name##_##counter_val { \
        Init_##name##_##counter_val() { \
            ForthVMClass::opcodes[opc_##name] = &ForthVMClass::op_##name; \
        } \
    } init_##name##_##counter_val; \
    void op_##name()

#define PRIMITIVE(name) PRIMITIVE_(name, __COUNTER__)


#define MEMORY_SIZE 65536
#define COLD_START_VECTOR 4

//
//	The VM
//
class ForthVMClass {


public:

	//
  //  Memory
  //
  uint8_t memory[MEMORY_SIZE];


private:

  inline uint16_t fetch(uint16_t addr) {
      return *(uint16_t*)&memory[addr];
  }

  inline void store(uint16_t addr, uint16_t val) {
      *(uint16_t*)&memory[addr] = val;
  }

  inline void push(uint16_t val) {
    SP -= 2;
    //                                could check for overflow here
    store(SP, val);
  }

  inline uint16_t pop() {
    uint16_t t = fetch(SP);
    SP += 2;
    //                                could check for underflow here
    return t;
  }


public:

	//
  //  Registers
  //
  uint16_t PC;              // Program counter a.k.a IP

  union {										// Instruction register, various layouts
    uint16_t bit0 : 1;      // Instruction type discriminator
    uint16_t word;          // Full 16-bit word

    struct {                // Packed twin opcodes view (active when bit0 == 1)
      uint8_t op1;          // Low byte (Bits 0-7)
      uint8_t op2;          // High byte (Bits 8-15)
    };
    struct {                // Packed single opcode and signed parameter
      uint8_t op;           // Low byte (Bits 0-7)
      int8_t parm;          // Signed 8 bit parameter
    };
  } I;

  uint16_t A;               // address register

  uint16_t SP;             	// Parameter stack pointer - points to NOS
  uint16_t T;               // Top of stack register aka accumulator

  uint16_t RSP;             // Return stack pointer
  uint16_t L;               // Link register ~top of paramater stack


public:

	//
  //  Called once at startup cold boot the system
  //
  void init() {
      for (size_t i = 0; i < MEMORY_SIZE; i++) {           // Clear memory
          memory[i] = 0;
      }

      auto core_image = generate_core_image();             // Copy over core image
      if (core_image.size() <= MEMORY_SIZE) {
          for (size_t i = 0; i < core_image.size(); i++) {
              memory[i] = core_image[i];
          }
      }

      PC = COLD_START_VECTOR;                            	// cold start defined in the bootstrap memory
  }


	//
  //  VM execution loop
  //  Called externally to execute the next VM instruction
  //
  void tick() {
    I.word = fetch(PC);               							// Fetch 16-bit word at PC
    PC +=2;                           							// PC points to the next instruction word

    if (I.bit0 == 0) {                							// Bit 0 is 0: Even address jump target (Load PC)
      L = PC;																				// Link to previous address
      PC = I.word;

    } else {                          							// Bit 0 is 1: Packed instruction word
    	(this->*opcodes[I.op1])();     								// Execute Op1
    	(this->*opcodes[I.op2])();     								// Execute Op2
    }
  }


public:

	//
	//	VM Primitive instructions
  //
	PRIMITIVE(nop) {}


	//
  //	Opcode to Primitive function lookup table
	//
  inline static std::array<void (ForthVMClass::*)(), 256> opcodes = []() {
    std::array<void (ForthVMClass::*)(), 256> arr;
    arr.fill(&ForthVMClass::op_nop);                // Pre-fill all 256 slots with the member NOP
    return arr;
  }();


	//
  //  Next from any leaf word which only uses primitives
  //

  PRIMITIVE(next) {																	// From leaf words which didn't push L
    PC = L;
    I.op2 = opc_nop;																// If I'm in op1 - stop op2 running
  	//  std::cout << std::hex << "next " << PC << "\n";
  }


	//
  //  Enter/Exit from any words which call other words
  //	Stack the link register
  //

  PRIMITIVE(enter) {                                // Store the link register on the return stack
    RSP -= 2;
    store(RSP, L);
  }

  PRIMITIVE(exit) {                                 // Restore the link register into PC
  	PC = fetch(RSP);
    RSP += 2;
  }

  PRIMITIVE(execute) {       												// ( n1 -- )
    PC = T;																					// Leave L at the caller to execute
    T = pop();
  }


	//
  //  Registers
  //

	PRIMITIVE(split16) {						// ( -- )
		SP = fetch(PC);                      						// literal is in the next word
    PC +=2;                                     		// skip it
	}

  PRIMITIVE(spstore) {
    SP = T;
    T = 0;
  }

  PRIMITIVE(rpstore) {
    RSP = T;
    T = pop();
  }

	PRIMITIVE(spfetch) {
		uint16_t t = SP;
		push(T);
		T = t;
	}

	PRIMITIVE(rpfetch) {
		uint16_t t = RSP;
		push(T);
		T = t;
	}

	//
  //  Literals
  //

  PRIMITIVE(lit8) {        // ( -- n1 )
		push(T);
    T = I.parm;                             // signed literal in high byte sign extend to 16bit
    I.op2 = opc_nop;                        // change opcode 2 to nop
  }

  PRIMITIVE(lit16) {       // ( -- n1 )
		push(T);
    T = fetch(PC);                      		// literal is in the next word
    PC +=2;                                 // skip it
  }

	PRIMITIVE(lits) {				// ( -- addr )
		push(T);
		T = PC;																	// Literal is the next execution address
		PC += I.parm *2;												// Jump over the string
    I.op2 = opc_nop;                        // skip the branch offset
	}


	//
  //  Control
  //

  PRIMITIVE(br) {         // ( -- ) branch
    PC += I.parm *2;                        // Parameter is the number of words +- 0 = next instruction
    I.op2 = opc_nop;                        // skip the branch offset
  }

  PRIMITIVE(brz) {        // ( bool -- ) 0branch if false
    if(T == 0) {
      PC += I.parm *2;                      // Parameter is the number of words +- 0 = next instruction
    }
    T = pop();															// pop the bool
    I.op2 = opc_nop;                        // skip the branch offset
  }

	PRIMITIVE(brnz) {				// ( bool -- ) -1branch if true
    if(T != 0) {
      PC += I.parm *2;                      // Parameter is the number of words +- 0 = next instruction
    }
    T = pop();															// pop the bool
    I.op2 = opc_nop;                        // skip the branch offset
	}

	PRIMITIVE(brn) {				// ( bool -- )	branch if negative
    if((int16_t)T < 0) {
      PC += I.parm *2;                      // Parameter is the number of words +- 0 = next instruction
    }
    T = pop();															// pop the bool
    I.op2 = opc_nop;                        // skip the branch offset
	}

	PRIMITIVE(brp) {				// ( bool -- )	branch if positive
    if((int16_t)T > 0) {
      PC += I.parm *2;                      // Parameter is the number of words +- 0 = next instruction
    }
    T = pop();															// pop the bool
    I.op2 = opc_nop;                        // skip the branch offset
	}


	//
  //  Stack
  //

  PRIMITIVE(dup) {            // ( n1 -- n1 n1 )
    push(T);
  }

  PRIMITIVE(drop) {           // ( n1 -- )
    T = pop();
  }

  PRIMITIVE(swap) {           // ( n1 n2 -- n2 n1 )
    uint16_t w = fetch(SP);
    store(SP, T);
    T = w;
  }

  PRIMITIVE(pick) {           // ( n1 -- n2 )
    T = fetch(SP + (T *2));
  }


	//
  //  Math
  //

  PRIMITIVE(add) {            						// ( n1 n2 -- n1+n2 )
    T += pop();
  }

  PRIMITIVE(sub) {            						// ( n1 n2 -- n1-n2) NOS - TOS
    T = pop() - T;
  }

  PRIMITIVE(mult) {           						// ( n1 n2 -- n1*n2 )
    T *= pop();
  }

  PRIMITIVE(div) {           							// ( n1 n2 -- n1/n2 ) NOS / TOS
  	if (T == 0 ) {												// Check for divide by 0
  		pop();
  		return;															// -->
  	}
    T = pop() / T;
  }

  PRIMITIVE(divmod) {                     // ( n1 n2 -- n1/n2 r ) NOS / TOS
  	if (T == 0 ) {												// Check for divide by 0
  		pop();
  		store(SP, 0);
  		return;															// -->
  	}

    int16_t divisor = T;
    int16_t dividend = fetch(SP);

    int16_t q = dividend / divisor;
    int16_t r = dividend % divisor;

    store(SP, r);                	// NOS gets remainder
    T = q;                            // T gets quotient
  }

  PRIMITIVE(and) {
		T &= pop();
  }

	PRIMITIVE(or) {
		T |= pop();
	}

	PRIMITIVE(not) {
		T = ~T;
	}


	//
  //  Memory
  //

  PRIMITIVE(afetch) {           // ( -- n1 )				// value of A
    push(T);
    T = A;
  }

  PRIMITIVE(astore) {           // ( n1 -- n1 )			// update value of A
    A = T;
    T = pop();
  }

  PRIMITIVE(fetcha) {           // ( -- n1 )				// fetch value at address A
    push(T);
    T = fetch(A);
  }

  PRIMITIVE(fetchaadd) {        // ( -- n1 )				// fetch value at address A, inc A
    push(T);
    T = fetch(A);
    A++;
  }

  PRIMITIVE(storea) {           // ( n1 -- )				// store value at address A
    store(A, T);
    T = pop();
  }

	PRIMITIVE(move) {							// ( src dest bytes -- )			// Move memory
		size_t bytes	= T;
		uint16_t dest = pop();
		uint16_t src  = pop();
		T = pop();

		if (src + bytes > MEMORY_SIZE) {												// Clamp source if it would read past the end of memory
    	bytes = MEMORY_SIZE - src;
		}
		if (dest + bytes > MEMORY_SIZE) {												// Clamp destination if it would write past the end of memory
	    bytes = MEMORY_SIZE - dest;
	  }

    std::memmove(&memory[dest], &memory[src], bytes);
	}

	PRIMITIVE(scmp) {							// ( string string -- flag )	// Compare null terminated strings, 0 = same
		const char* str1 = (const char*)&memory[pop()];
		const char* str2 = (const char*)&memory[T];

    T = std::strcmp(str1, str2);

//    std::cout << "scmp " << str1 << " = " << str2 << " -> " << T << " \n";
	}

	//
  //	Compiling
  //

	//	Convert the null terminated string to a number
	//
  PRIMITIVE(number) {         												// ( string -- n1 f )
    uint16_t addr = (uint16_t)T;
    const char* p = (const char*)&memory[addr];

    if (*p == '\0') {																	// Empty string
        T = 0; 																		// Flag: Failure
        push(0);																	// Value: 0
        return;																				// -->
    }

		//	Detect base with string prefixes
    int base = 10;																		// Decimal default
    if (*p == '%' ) {																	// Binary %...
    	base = 2;
    	p++;
    }
    if (p[0] == '0' && p[1] == 'b') {						 			// Binary 0b...
      base = 2;
      p += 2;
    }
    if (*p == '$' ) {																	// Hex $...
    	base = 16;
    	p++;
    }
    if (p[0] == '0' && p[1] == 'x') {									// Hex 0x...
      base = 16;
      p += 2;
    }

    char* endptr;
    long val = std::strtol(p, &endptr, base);					// Interpret the number
    bool success = (endptr != p);											// parsed at least one char

    T = success ? -1 : 0;         								// Flag on TOS
    push(success ? (uint16_t)val : 0); 						// Value on NOS
  }


	//	ScanWhile - Move through the string while characters match the set
	//
	PRIMITIVE(scanw) {                                 		// ( string set -- next string len )
    uint16_t set_addr = T;                         	// top: set
    uint16_t str_addr = fetch(SP);             	// second: string

    char* delim_set = (char*)&memory[set_addr];
    uint16_t start = str_addr;

		// std::cout << "1scanW >" << str_addr << "<  >" << delim_set << "< \n";

    while (memory[str_addr] != '\0') {
        char c = memory[str_addr];
        if (std::strchr(delim_set, c) == nullptr) {
            break; 																			// -> Stop when character is NOT in the set
        }
        str_addr++;
    }

		// std::cout << "2scanW " << str_addr << ", " << start << ", " << str_addr - start << "\n";

    store(SP, str_addr);                   			// Next
    push(start);                                 		// String
    T = str_addr - start;                           // Length
	}


	//	ScanUntil - Move through the string until characters match the set
	//
	PRIMITIVE(scanu) {                                 		// ( string set -- next string len )
    uint16_t set_addr = T;                          // top: set
    uint16_t str_addr = fetch(SP);             	// second: string

    char* delim_set = (char*)&memory[set_addr];
    uint16_t start = str_addr;

		// std::cout << "1scanU >" << str_addr << "<  >" << delim_set << "< \n";

    while (memory[str_addr] != '\0') {
        char c = memory[str_addr];
        if (std::strchr(delim_set, c) != nullptr) {
            break; 																			// -> Stop when character IS in the set
        }
        str_addr++;
    }

		// std::cout << "2scanU " << str_addr << ", " << start << ", " << str_addr - start << "\n";

    store(SP, str_addr);                   			// Next
    push(start);                                 		// String
    T = str_addr - start;                           // Length
	}


	//
  //  Terminal I/O
  //

  PRIMITIVE(dot) {            // ( n1 -- ) Decimal

	#ifdef HOST_TEST
		std::printf("%d ", (int16_t)T);
    fflush(stdout); 					// Ensure output flushes immediately
		T = pop();
  #endif

	}

	PRIMITIVE(udot) {						// ( u1 -- ) Unsigned decimal

	#ifdef HOST_TEST
		std::printf("%d ", (uint16_t)T);
    fflush(stdout); 					// Ensure output flushes immediately
		T = pop();
  #endif

	}

	PRIMITIVE(hdot) {						// ( n1 -- ) +-7f

	#ifdef HOST_TEST
		std::printf("%02X ", (int16_t)T);
    fflush(stdout); 					// Ensure output flushes immediately
		T = pop();
  #endif

	}

	PRIMITIVE(uhdot) {					// ( u1 -- ) ffff

	#ifdef HOST_TEST
		std::printf("%04X ", (uint16_t)T);
    fflush(stdout); 					// Ensure output flushes immediately
		T = pop();
  #endif

	}


  PRIMITIVE(emit) {           // ( char -- )

	#ifdef HOST_TEST
			char c = (char)(T & 0xFF);
			putchar(c);
    	fflush(stdout); 				// Ensure output flushes immediately
    	T = pop();
  #endif

  }


/*
  PRIMITIVE(type) {           // ( addr -- )
	#ifdef HOST_TEST
			fputs((const char*)&memory[T], stdout);
    	fflush(stdout); 				// Ensure output flushes immediately

			T = pop();
	#endif
  }
*/

  PRIMITIVE(qkey) {           // ( -- char) Doesn't wait

  }

  PRIMITIVE(key) {            // ( -- char )

	#ifdef HOST_TEST
			push(T);
			T = getchar();
  #endif

  }

  PRIMITIVE(accept) {         // (addr n1 -- n2) Returns chars input

	#ifdef HOST_TEST
	    uint16_t max_len = T;
	    uint16_t buf_addr = pop();

	  	char* dest = (char*)&memory[buf_addr];			   		// Cast VM memory address directly to a C-string buffer
	    if (fgets(dest, max_len, stdin) == nullptr) {				  // fgets reads up to max_len - 1 characters from stdin
	    	dest[0] = '\0';																			// Handle EOF or read error
  	  	T = 0;
  	  	return;																							// -->
  	  }

      size_t len = std::strlen(dest);											// strPC any trailing newline
      if (len > 0 && dest[len - 1] == '\n') {
      	dest[len - 1] = '\0';
        len--;
      }
      if (len > 0 && dest[len - 1] == '\r') { 						// Handle Windows \r\n line endings
      	dest[len - 1] = '\0';
        len--;
      }

      T = (uint16_t)len;															// leave final string length in T
	#endif

  }

	//
	//	File I/O
	//

	PRIMITIVE(fop) {									// ( path flags -- id bool )	Open
	};

	PRIMITIVE(fcl) {									// ( id -- bool )							Close
	};

	PRIMITIVE(fr)	{										// ( id -- addr bool )				Read
	};

	PRIMITIVE(fw)	{										// ( id addr -- bool )				Write
	};

	PRIMITIVE(fcmd)	{									// ( cmd -- bool )						Command
	};


	//
  //  Bootstrap memory image
  //
  #include "ForthBootMemory.h"


	PRIMITIVE(dots) {																				// Dump the stack
    const uint16_t PS_TOP = 0x0100 + 32;

    if (SP == PS_TOP) {
        std::printf("0:\n");
        return;
    }

    // mem_slots naturally equals true depth because the ghost slot offsets it
    int depth = (PS_TOP - SP) / 2;
    std::printf("%d: ", depth);

    // Print memory items from PS_TOP - 4 down to SP, skipping the boot ghost at PS_TOP - 2
    for (uint16_t p = PS_TOP - 4; p >= SP; p -= 2) {
        std::printf("%d ", (int16_t)fetch(p));
    }

    std::printf("[%d]\n", (int16_t)T);
	}


} inline ForthVM;
