/*
	Main.cpp
	Test harness for the forthVM

	Requires:
		ForthVM.h					The VM itself
		ForthBootMemory.h	Boot memory builder


	To run on Mac OSX:
		clang++ -std=c++17 -O3 -g main.cpp -o forth_vm

		clang++ -std=c++17 -g -Wl,-map,forth_vm.map main.cpp -o forth_vm -O3

		./forth_vm


*/

#define HOST_TEST								// When defined diverts IO primitives to the terminal

#include <iostream>
#include <vector>
#include "ForthVM.h" 						// Pure C++ VM engine

#include <iostream>
#include <iomanip>
#include <string>



// Static table mapping odd opcodes (1, 3, 5, 7...) directly by index: (op - 1) / 2
//
std::string get_opcode_name(uint8_t op) {
    static const char* const opcode_names[] = {
        "nop",		// 0x01
        "next",		// 0x03
        "entr",		// 0x05
        "exit",		// 0x07
        "exec",		// 0x09

        "sp#",		// 0x0b sp literal
        "sp!",
        "rp!",
        "sp@",
        "rp@",

        "lit8",		// 0x15	Literal 8 bit
        "lit ",		//			Literal 16 bit
        "lits",		//			Literal string

        "br  ",		// 			branch relative
        "brz ",		// 				if zero (false)
        "brnz",		// 				if not zero (true)
        "brn ",		//				if negative
        "brp ",		//				if positive

        "dup",
        "drop",
        "swap",
        "pick",

        "add ",
        "sub ",
        "mul ",
        "div ",
        "dvmd",
        "and ",
        "or  ",
        "not ",

        "a@  ",
        "a!  ",
        "@a  ",		//		@a c@    a+ a-
        "!a  ",		//		!ca
        "a+  ",
        "move",
        "scmp",		//			String compare

        "num ",
        "scnw",		//			Scan while
        "scnu",		//			Scan until

        ".   ",		//			Signed
        "u.  ",		//			Unsigned
        "h.  ",		//			Hex						ff
        "uh. ",		//			Unsigned hex	ffff

        "?key",
        "acpt",		//			Accept

				"in  ",		//			I/O
				"out "
    };

    size_t index = (op - 1) / 2;
    if (op % 2 == 0 || index >= sizeof(opcode_names) / sizeof(opcode_names[0])) {
				char buf[16];
        std::snprintf(buf, sizeof(buf), "#%02X", op);
        return std::string(buf);    }

    return opcode_names[index];
}


//
//	Disassembler
//
void disassemble(const uint8_t* memory, uint16_t start_addr, int word_count) {
    uint16_t addr = (start_addr +1) & ~1;
    int words_processed = 0;

		std::cout << "\n--- Disassembly ---\n";

    while (words_processed < word_count) {
        uint16_t low = memory[addr];
        uint16_t high = memory[addr + 1];
        uint16_t word = low | (high << 8);

        // Print address and raw 16-bit word hex
        //
        std::cout << std::hex << std::setw(4) << std::setfill('0') << addr << ": "
                  << std::setw(4) << std::setfill('0') << word << "  ";

				// Opcode specific formatting
				//
        if ((word & 1) == 0) {					            // Bit 0 = 0: Call
            std::cout << "call  " << std::hex << std::setw(4) << std::setfill('0') << word << "\n";
            addr += 2;
            words_processed++;

        } else {																	  // Bit 0 = 1: Packed dual opcodes
            uint8_t op1 = low;
            uint8_t op2 = high;

            std::cout << get_opcode_name(op1) << " ";																					// Op1

            if (op1 == ForthVMClass::opc_lit8
            	|| op1 == ForthVMClass::opc_in || op1 == ForthVMClass::opc_out) {								// Lit 8 bit
            	std::cout << "   " << std::hex << std::setw(2) << std::setfill('0') << high;

						} else if (op1 == ForthVMClass::opc_lits ) {																			// Lit string
							std::cout << " \"" << &memory[addr] << "\"";
							addr += (int8_t)op2 *2;

            } else if (op1 == ForthVMClass::opc_br
            					|| op1 == ForthVMClass::opc_brz || op1 == ForthVMClass::opc_brnz
            					||	op1 == ForthVMClass::opc_brn || op1 == ForthVMClass::opc_brp) {			// Branches
            	std::cout << " " << std::hex << std::setw(4) << std::setfill('0') << addr +2 + ((int8_t)op2 *2);

            } else if (op2 != ForthVMClass::opc_nop) {																				// Suppress nop
            	std::cout << "\n            " << get_opcode_name(op2);

						}
						std::cout << "\n";

            addr += 2;
            words_processed++;


            if (op1 == ForthVMClass::opc_lit16 || op2 == ForthVMClass::opc_lit16
            || op1 == ForthVMClass::opc_split16 || op2 == ForthVMClass::opc_split16) {				// lit and split 16 bit
            		uint16_t lit_low = memory[addr];
                uint16_t lit_high = memory[addr + 1];
                uint16_t lit_val = lit_low | (lit_high << 8);

                std::cout << std::hex << std::setw(4) << std::setfill('0') << addr << ": "
                          << std::setw(4) << std::setfill('0') << lit_val << "        "
                          << std::setw(4) << std::setfill('0') << lit_val << "\n";

                addr += 2;
                words_processed++;
            }
        }
    }
    std::cout << std::dec; // Reset stream formatting
}


//
//	Dump memory
//
void dump_memory(const uint8_t* memory, uint16_t start_addr, uint16_t length) {
    // Round down start_addr to the nearest 16-byte boundary
    uint16_t addr = start_addr; // & 0xFFF0;
    uint16_t end_addr = start_addr + length;

		std::cout << "\n--- Memory dump ---\n";

    while (addr < end_addr) {
        // Print address header
        std::cout << std::hex << std::setw(4) << std::setfill('0') << addr << ": ";

        char ascii_buf[17];
        int ascii_idx = 0;

        // Print 16 hex bytes
        for (int i = 0; i < 16; ++i) {
            uint16_t current_addr = addr + i;
            if (current_addr >= start_addr && current_addr < end_addr) {
                uint8_t val = memory[current_addr];
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)val << " ";

                // Collect printable ASCII
                ascii_buf[ascii_idx++] = (val >= 32 && val <= 126) ? val : '.';
            } else {
                std::cout << "   "; // Padding for out-of-range bytes
                ascii_buf[ascii_idx++] = ' ';
            }
            if (i == 7) std::cout << " "; // Extra space in the middle for readability
        }

        ascii_buf[ascii_idx] = '\0';
        std::cout << " | " << ascii_buf << " |\n";

        addr += 16;
    }
    std::cout << std::dec; // Reset stream formatting back to decimal
}


//
//	Main testing loop
//
int main() {
    std::cout << "Starting Forth VM Desktop Harness...\n";

    ForthVM.init();

    dump_memory(ForthVM.memory, 0x0000, 0x0014);
    dump_memory(ForthVM.memory, 0x0800, 0x0500);

		disassemble(ForthVM.memory, 0x09fe, 50);

		int i = 1;
		while(i<10) {
//        std::cout << "\n--- Tick " << i << " ---\n";
//				i++;
//				registers();
//					std::cout << ForthVM.SP << ": ";
//					dump_memory(ForthVM.memory, ForthVM.SP, 4);
        ForthVM.tick();
    }


    return 0;
}
