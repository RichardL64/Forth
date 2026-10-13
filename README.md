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
                    
