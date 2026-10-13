/*
    ForthBootMemory.h
    R.A.Lincoln   September 2026

    Bootstrap RAM image with base words
    Copied over RAM by the VM at startup
*/

#pragma once


//  Instruction helpers
//
#define lit8(value)    	_align(); opc(lit8); _byte(value);
#define lit16(value)   	opc(lit16); _align(); _word(value);
#define lits(str)				_string(str);


//	Allocate memory assembler style
//
#define org(addr) \
	cursor = addr;

#define ds_b(label, bytes) \
    const uint16_t label = cursor; \
		cursor += bytes;

#define ds_w(label, words) \
    const uint16_t label = cursor; \
    cursor += words *2;

#define dc_b(label, ...) \
    const uint16_t label = cursor; \
    for (uint8_t b : { __VA_ARGS__ }) { \
        rom[cursor++] = b; \
    }

#define dc_w(label, value) \
    const uint16_t label = cursor; \
    rom[cursor++] = (uint8_t)((value) & 0xFF); \
    rom[cursor++] = (uint8_t)(((value) >> 8) & 0xFF);

#define set_w(label, value) \
    rom[label] = (uint8_t)((value) & 0xFF); \
    rom[label+1] = (uint8_t)(((value) >> 8) & 0xFF);

#define dc_s(label, str) \
    const uint16_t label = cursor; \
    for (char c : str) { \
        rom[cursor++] = c; \
    }

#define mark(label, offset) \
	const uint16_t label = cursor + offset;


//  Create word_...name... headers
//
#define create(name) \
    uint16_t word_##name = _header(#name);													// leaf word

#define createx(label, name) \
    uint16_t word_##label = _header(name);													// leaf word with special chars name

#define createc(name) \
    uint16_t word_##name = _header(#name); opc(enter);							// colon word

#define createcx(label, name) \
    uint16_t word_##label = _header(name); opc(enter);							// colon word

#define immediate rom[latest +2] = 128;

#define endc	opc(exit); _align();																	// end colon word


//	Opcode lists
//
#define _OP_1(a)          		_byte(opc_##a);
#define _OP_2(a, b)       		_OP_1(a); _OP_1(b);
#define _OP_3(a, b, c)    		_OP_2(a, b); _OP_1(c);
#define _OP_4(a, b, c, d) 		_OP_2(a, b); _OP_2(c, d);
#define _OP_5(a, b, c, d, e)	_OP_2(a, b); _OP_2(c, d); _OP_1(e);

#define _GET_OP_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME
#define opc(...) _GET_OP_MACRO(__VA_ARGS__, _OP_5, _OP_4, _OP_3, _OP_2, _OP_1)(__VA_ARGS__)


//	Call lists
//
#define _CALL_1(a)          					_align(); _word(word_##a);
#define _CALL_2(a, b)       					_CALL_1(a); _word(word_##b);
#define _CALL_3(a, b, c)    					_CALL_2(a, b); _word(word_##c);
#define _CALL_4(a, b, c, d) 					_CALL_3(a, b, c); _word(word_##d);
#define _CALL_5(a, b, c, d, e)				_CALL_4(a, b, c, d); _word(word_##e);;
#define _CALL_6(a, b, c, d, e, f)			_CALL_5(a, b, c, d, e); _word(word_##f);;
#define _CALL_7(a, b, c, d, e, f, g)	_CALL_6(a, b, c, d, e, f); _word(word_##g);;

#define _GET_WORD_MACRO(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define call(...) _GET_WORD_MACRO(__VA_ARGS__, _CALL_7, _CALL_6, _CALL_5, _CALL_4, _CALL_3, _CALL_2, _CALL_1)(__VA_ARGS__)


//	Flow control
//
#define BEGIN(label) \
		_align(); \
		size_t label = cursor;

#define AGAIN(label) \
		_align(); \
    opc(br); \
    _byte((label - cursor) /2);


#define WHILE(patch) \
		_align(); \
    opc(brz); \
    size_t patch = cursor; \
    _byte(0xff);

#define REPEAT(begin, while) \
		_align(); \
    opc(br); \
    _byte((begin - cursor) /2); \
    rom[while] = (cursor - while) /2;


#define IF(patch) \
		_align(); \
    opc(brz); \
    size_t patch = cursor; \
    _byte(0xff);

#define ZIF(patch) \
		_align(); \
    opc(brnz); \
    size_t patch = cursor; \
    _byte(0xff);

#define NIF(patch) \
		_align(); \
    opc(brn); \
    size_t patch = cursor; \
    _byte(0xff);

#define PIF(patch) \
		_align(); \
    opc(brp); \
    size_t patch = cursor; \
    _byte(0xff);

#define ELSE(patch, jump_patch) \
		_align(); \
    opc(br); \
    size_t jump_patch = cursor; \
    _byte(0xff); \
    rom[patch] = (cursor - patch) /2;

#define ENDIF(patch) \
    rom[patch] = (cursor - patch) /2;





//
// Compile-time memory image generator
//

constexpr std::array<uint8_t, MEMORY_SIZE> generate_core_image() {
    std::array<uint8_t, MEMORY_SIZE> rom = {};
    size_t cursor = 0;
    uint16_t latest = 0;

		auto _align = [&](void) {											// Word align with nops
			if (cursor & 1) {
				rom[cursor++] = opc_nop;
			}
		};
    auto _byte = [&](uint8_t b) { rom[cursor++] = b; };
    auto _word = [&](uint16_t w) { _align(); rom[cursor++] = w & 0xFF; rom[cursor++] = w >> 8; };

    auto _header = [&](const char* name) {
			_align();
			uint16_t h = cursor;
      _word(latest);      											// Link field
      latest = h;

      _byte(0);           											// Flags

      for (int i = 0; name[i] != '\0'; ++i) {		// Name field
        _byte(name[i]);
      }
      _byte('\0');        											// Null terminator

      _align(); 																// Word align the paramater field
			return cursor;
    };

		auto _string = [&](const char* str) {
    	size_t bytes = strlen(str);
    	bytes++;																	// Include null
    	uint8_t words = (bytes + 1) /2;

			_align();
    	_byte(opc_lits);                       		// String literal opcode
    	_byte(words);    													// Word count to jump over it
    	for (size_t i = 0; i < bytes; ++i) {
        _byte(str[i]);                      		// Characters + null terminator
    	}
      _align(); 																// Word align
		};


		//
    //  Bootstrap memory image
    //  16bit aligned words
    //

    //	Page 0
    //	$02-$7f available
    //	$80-$ff assume c64 version kernal use
		//
		org(0x0004);								// Start at $04 for Amiga old times sake
		dc_w(COLD, 		0x0000);			// Cold start word, must equal ForthVM COLD_START_VECTOR
		dc_w(MEMBOT, 	0x0800);			// Memory limits
		dc_w(MEMTOP, 	0xCFFF);
		dc_w(DP, 			0x0000);			// Next compilation location
		dc_w(CONTEXT, 0x0000);			// Wordlist to find words
		dc_w(CURRENT, 0x0000);			// Wordlist to define words
		dc_w(FENCE,		0x0000);			// Dictionary forget limit
		dc_w(IN, 			0x0000);
		dc_w(STATE, 	0x0000);			// True when compiling


		//	Page 1
		//
		org(0x0100)

		ds_w(PS_BOT, 16);						// Parameter stack
		dc_w(PS_TOP, 0);

		ds_w(RS_BOT, 100);					// Return stack
		dc_w(RS_TOP, 0);


		//	Page 3
		//
		org(0x0300);

		ds_b(TIB, 80);							// Input buffer


    //  Word definitions
    //
    org(0x0800);


		//	Leaf words - don't call other words - end with next opcode
		//
		createx(szero, "s0");			lit16(PS_TOP); opc(next);
		createx(spstore, "sp!");	opc(spstore, next);
		createx(spfetch, "sp@");	opc(spfetch, next);

		createx(rzero, "r0");		lit16(RS_TOP); opc(next);
		createx(rpstore, "rp!");	opc(rpstore, next);
		createx(rpfetch, "rp@");	opc(rpfetch, next);

    createx(add, "+");				opc(add, next);
    createx(sub, "-"); 				opc(sub, next);

    createx(mult, "*"); 			opc(mult, next);
    createx(div, "/"); 				opc(div, next);
    createx(divmod, "/mod");		opc(divmod, next);

    createx(oneadd, "1+");		lit8(1); opc(add, next);
    createx(twoadd, "2+");		lit8(2); opc(add, next);
    createx(twodiv, "2/"); 		lit8(2); opc(div, next);
    createx(twomult, "2*");		lit8(2); opc(mult, next);

		create(and);							opc(and, next);
		create(or);								opc(or, next);
		create(not);							opc(not, next);

		createx(less, "<");				opc(sub); NIF(less1)															// : < - -if 0 exit then -1 ;
																lit8(0); opc(next);
															ENDIF(less1);
															lit8(-1); opc(next);

		createx(more, ">");				opc(sub); PIF(more1)															// : < - +if 0 exit then -1 ;
																lit8(0); opc(next);
															ENDIF(more1);
															lit8(-1); opc(next);

		createx(lesseq, "<=");		opc(sub); PIF(lesse1)															// : <= - -if -1 exit then 0 ;
																lit8(-1); opc(next);
															ENDIF(lesse1);
															lit8(0); opc(next);

		createx(moreeq, ">=");		opc(sub); NIF(moree1)															// : >= - +if -1 exit then 0 ;
																lit8(-1); opc(next);
															ENDIF(moree1);
															lit8(0); opc(next);

    create(dup);  						opc(dup, next);
    create(drop);							opc(drop, next);
    create(swap);							opc(swap, next);
    create(pick);							opc(pick, next);
    create(over);							lit8(1); opc(pick, next);                         // : over 1 pick ;
    create(nip);							opc(swap, drop, next);														// : nip swap drop ;
    createcx(twodup, "2dup");																										// : 2dup over over ;
    	call(over, over);
    endc;

    createx(fetch, "@");			opc(astore, fetcha, next);												// : @ a! @a ; ( addr -- n1 )
    createx(store, "!");			opc(astore, storea, next);												// : ! a! !a ; ( addr n1 -- )
    createx(plusstore, "+!");	opc(astore, fetcha, add, storea, next);						// : +! a! @a + !a ;
    createx(cfetch, "c@");		opc(astore, fetcha); lit8(0xff); opc(and, next);	// : c@ a! @a 0xff and ;
    createx(cstore, "c!");		opc(astore, fetcha, and, storea, next);     			// : c! a! @a and !a ;
		create(move);							opc(move, next);
		create(scmp);							opc(scmp, next);

    createx(dot, ".");    		opc(dot, next);
    createx(udot, "u.");    	opc(udot, next);
    createx(hdot, "h.");    	opc(hdot, next);
    createx(uhdot, "uh.");    opc(uhdot, next);

    create(emit);         		opc(emit, next);

		create(type);																																// : type ( addr -- )
			opc(astore);																															//		a!									( -- )
			BEGIN(type1)																															//		begin
				opc(fetchaadd); lit16(0xff); opc(and, dup);															//			@a+ 0xff and dup	( -- char char )
			WHILE(type2)																															//		while								( -- char )
				opc(emit);																															//			emit 							( -- )
			REPEAT(type1, type2)																											//		repeat
			opc(drop, next);																													//		drop, next					( -- )

    createx(qkey, "?key");		opc(qkey, next);
    create(key);           		opc(key, next);
    create(accept);       		opc(accept, next);

		createx(dots, ".s");			opc(dots, next);

    create(dp); 							opc(lit16, next); _word(DP);											// Constant runtime for sys vars
    create(limit); 						opc(lit16, next); _word(MEMTOP);
    create(current); 					opc(lit16, next); _word(CURRENT);
    create(context); 					opc(lit16, next); _word(CONTEXT);
    create(tib); 							opc(lit16, next); _word(TIB);
		create(fence);						opc(lit16, next); _word(FENCE);
    createx(in, ">in");				opc(lit16, next); _word(IN);
		create(state);						opc(lit16, next); _word(STATE);

    create(number);						opc(number, next);															// : number ( addr - n1 flag )


		createx(exit0, "\0"); 		opc(exit);	immediate;													// End of TIB, shouldn't need next
		create(execute); 					opc(execute);																		// shouldn't need next

		create(exit);							opc(exit);

		createx(qexit, "?exit");																									// Exit if true != 0
			IF(qexit1);
				opc(exit);
			ENDIF(qexit1);
			opc(next);

		createx(zqexit, "0?exit");																								// Exit if false = 0
			ZIF(zqexit1);
				opc(exit);
			ENDIF(zqexit1);
			opc(next);

		create(cr);								lit8('\n'); opc(emit, next);										// : cr '\n' emit ;
		create(space);						lit8(32); opc(emit, next);											//: space 32 emit ;



		//	Colon code words - end with endc macro
		//

		createc(depth);																														// : depth s0 sp@ - 2/ 1 - ;
			call(szero, spfetch, sub, twodiv); lit8(1); call(sub);
		endc;

    createc(definitions); 																										// : definitions context @ current ! ;
    	call(context, fetch, current, store);
    endc;

    createc(here);																														// : here dp @ ;
    	call(dp, fetch);
		endc;

		createc(align);																														// : align 1+ 0xfe and ;
			call(oneadd); lit8(0xfe); call(and);
		endc;

    createc(allot);																											 			// : allot align dp +! ;
			call(align, dp, plusstore);
		endc;

    createcx(comma, ",");     																								// : , here ! 2 allot ;
    	call(here, store); lit8(2); call(allot);
    endc;

		createcx(name, "name>"); 																								// link field -> name field
			lit8(3); call(add);
		endc;

		createcx(body, "body>");																								// link field -> parameter field
			call(name);
			lits("\0"); opc(scanu, add);																					// ( -- next string-end )
			call(oneadd, align, nip);																							// link field -> xt
		endc;


		//	Find
		//
		createc(find);																													// : find ( string -- xt | 0 )
			call(context, fetch);																									//		context @							( -- str link )
			BEGIN(find1);																													//		begin
				opc(dup);																														//			dup									( -- str link flag )
			WHILE(find2);																													//		while
				call(twodup, name);																									//			2dup name>					( -- str link str str2 )
				opc(scmp) ZIF(find3);																								//			scmp 0if						( -- str link )
					call(nip, body); opc(exit);																				//				nip body> exit		( -- xt )
				ENDIF(find3)																												//			endif
				call(fetch);																												//  		@										( -- str link )
			REPEAT(find1, find2);																									//		repeat
			call(nip);																														//		nip
		endc;																																		//	;


		//	Word
		//
		dc_s(WHITE_SPACE,	" \t\r\n\0");																					// To ignore

		createc(word);																													// : word ( -- )
			call(tib, in, fetch, add);																						// 		( -- next )
			lit16(WHITE_SPACE); opc(scanw, drop, drop); 													//		( -- next ) 					Leading white space
			lit16(WHITE_SPACE); opc(scanu, dup);																	// 		( -- next str len len )
			call(here, add); lit8(0); call(swap, cstore);													// 		( -- next str len ) 	Trailing \0 at the dest
			call(here); opc(swap, move);																					// 		( -- here) 						Copy the word to here
			call(tib, sub, in, store);																						// 		( -- ) 								Update in
		endc;																																		//	;


		//	Interpret error routine - display word? & reset the stacks
		//
		createc(abort);																													// : abort
			call(here, type);																											//		here type
			lits("?\n"); call(type);
			call(szero, spstore); 																								//		reset the stack pointers
			call(rzero, rpstore);																									//

			call(abort);																													//		Patched to call(quit) later
			mark(ABORT, -2);																											// 		Address to patch in quit
		endc;																																		// ;

		createcx(qstack, "?stack");																							//	: ?stack
			call(szero, spfetch, sub);																						//		s0 sp@ -
			lit8(2); call(less); IF(qstack1);																			//		2 < if
				lits("Stack empty "); call(type, abort);														// 			-->
			ENDIF(qstack1);																												//		endif

		endc;

		//	Main interpret loop
		//	Intrpret each word in the buffer until the null end of string executes exit
		//
    createc(interpret);																											// : interpret
    	BEGIN(interpret1);																										//		begin
				call(word, here, find, dup);																				// 			word here find			( -- xt xt )

    		IF(interpret2);																											//			if
//    			call(state), call(fetch), brz(FM4),																//				state @ < (comparing with words header) if
//    				call(comma),																										//					,
//    			br(FM5), FM4,																											//				else

    				call(execute, qstack);																					//					execute ?stack

				ELSE(interpret2, interpret3);																				//			else
					call(drop, here, number);																					//				drop here number	( -- n1 flag)
					ZIF(interpret4);																									// 				0if
			  		call(abort);																										//					abort
			  	ENDIF(interpret4)																									//				endif

			  ENDIF(interpret3);																									//			endif
			AGAIN(interpret1);																										// 		again
    endc;																																		//	;


		//	Primary terminal input
		//
    createc(query); 																	// : query
    	lit8(0); call(in, store);												//		0 >in !
    	call(tib); lit8(80); call(accept, drop);      	// 		tib 80 accept drop ;
		endc;																							// ;

		//	Main quit loop
		//	Input/interpret repeat
		//
    createc(quit);											  						// : quit begin ok query interpret again ;
    	BEGIN(quit1);
				lits("Ok.\n"); call(type, query, interpret);
    	AGAIN(quit1);
		endc;
		set_w(ABORT, word_quit);													// Patch abort forward call to quit



		//
		//	Test harnesses
		//

		createc(x);
			lit8('\n');
			lit8('X');
			opc(emit, emit);
		endc;

		createc(t);																		// True
			lit8('t'); opc(emit);
			lit8(-1);
			lit8('T'); opc(emit);
 		endc;

		createc(f);																		// False
			lit8('f'); opc(emit)
			lit8(0);
			lit8('F'); opc(emit);
		endc;


    createc(t1);
			lits("t1\n"); call(type);
			lits("testa"); call(dup); opc(scmp); call(dot, cr);
			lits("testa"); lits("testc"); opc(scmp); call(dot, cr);
			lits(":"); lits(":"); opc(scmp); call(dot, cr);
		endc;

		createc(t2);
			lits("t2\n"); call(type);

			call(query, tib, find, cr, dots);
			lit8(0); call(tib, store);
		endc;

		createc(t3);
			lits("t3\n"); call(type);

    	call(query);
			call(word, here, type, cr);
		  call(word, here, type, cr);
		  call(word, here, type, cr);
		  call(word, here, type, cr);
		  call(word, here, type, cr);
		  call(word, here, type, cr);
		endc;


		//
    //	Cold start
    //	Initialise registers
  	//
    create(cold);																	// : cold
    	opc(split16); _word(PS_TOP);								//		sp# PS_TOP			Must NOT use the stack before it is initialised
    	lit16(RS_TOP); opc(rpstore);								//		RS_TOP rp!

    	lits("\n     **** Lincoln Forth 2026 ****\n64K RAM System  ");
    	call(type);

			lit8(MEMTOP);																// 	(MEMTOP)
			call(fetch, here, sub, udot);								// 	@ here - u.
			lits("Forth bytes free\n");
			call(type);

      call(quit);																	//   quit								// --> never returns
      opc(next, nop);															// ;


		//
    //  Update sys var values dependent on defined words
    //
    set_w(COLD_START_VECTOR, word_cold);
    set_w(DP, cursor);                  // Next free byte
    set_w(CURRENT, latest);             // Last link field address
    set_w(CONTEXT, latest);             // Last link field address



    return rom;
}
