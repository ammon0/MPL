/*******************************************************************************
 *
 *	occ : The Omega Code Compiler
 *
 *	Copyright (c) 2016 Ammon Dodson
 *
 ******************************************************************************/

#include "proto.h"
#include "errors.h"
#include <stdio.h>
#include <string.h>


/******************************************************************************/
//                               DEFINITIONS
/******************************************************************************/


typedef enum {
	A,  // Accumulator
	B,  // General Purpose
	C,  // Counter
	D,  // Data
	SI, // Source Index
	DI, // Destination Index
	BP, // General Purpose
	SP, // Stack Pointer
	R8, R9, R10, R11, R12, R13, R14, R15,
	NUM_X86_REG
} reg_t;

typedef enum {
	byte,
	word,
	dword,
	qword
} reg_width;

/* If within a function the stack is only used for storing automaic variables the the value of SP does not change until the function returns. in which case if parameters are passed in the registers then the BP can be general purpose. */

typedef enum {
	X_MOV,
	X_ADD,
	X_SUB,
	NUM_X86_INST
} x86_inst;


/******************************************************************************/
//                  GLOBAL CONSTANTS IN THE GEN-X86 MODULE
/******************************************************************************/


const char * inst_array[NUM_X86_INST] = {
	"mov", "add", "sub"
};


/******************************************************************************/
//                  GLOBAL VARIABLES IN THE GEN-X86 MODULE
/******************************************************************************/


/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 */
sym_pt reg_d[NUM_X86_REG];



/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// runs the various functions through their various outputs.
void test_x86(void);


static char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
	return array;
}

static char * str_name(str_dx dx){
	const char * in_name;
	static char * out_name;
	size_t length;
	
	in_name = Program_data::get_string(dx);
	length = strlen(in_name) + 2; // $ and \0
	
	out_name = (char*) realloc(out_name, length);
	
	sprintf(out_name, "$%s", in_name);
	
	return out_name;
}

// assumes width is possible in current mode
static char * str_reg(reg_width width, reg_t reg, bool B64){
	static char array[4];
	
	if (!B64 && width == qword)
		crit_error("operand too large for current proccessor mode");
	if (!B64 && reg >SP)
		crit_error("registers R8-R15 are only availible in Long mode");
	
	array[4] = '\0';
	
	switch (width){
	case byte : array[1] = ' '; array[3] = ' '; break;
	case word : array[1] = ' '; array[3] = 'x'; break;
	case dword: array[1] = 'e'; array[3] = 'x'; break;
	case qword: array[1] = 'r'; array[3] = 'x'; break;
	default   : crit_error("something done broke in put_reg()");
	}
	
	switch (reg){
	case A: array[2] = 'a'; break;
	case B: array[2] = 'b'; break;
	case C: array[2] = 'c'; break;
	case D: array[2] = 'd'; break;
	case SI: array[2] = 's'; array[3] = 'i'; break;
	case DI: array[2] = 'd'; array[3] = 'i'; break;
	case BP: array[2] = 'b'; array[3] = 'p'; break;
	case SP: array[2] = 's'; array[3] = 'p'; break;
	case R8: array[2] = '8'; array[3] = ' '; break;
	case R9: array[2] = '9'; array[3] = ' '; break;
	case R10: array[2] = '1'; array[3] = '0'; break;
	case R11: array[2] = '1'; array[3] = '1'; break;
	case R12: array[2] = '1'; array[3] = '2'; break;
	case R13: array[2] = '1'; array[3] = '3'; break;
	case R14: array[2] = '1'; array[3] = '4'; break;
	case R15: array[2] = '1'; array[3] = '5'; break;
	case NUM_X86_REG:
	default: crit_error("something done broke in put_reg()");
	}
	
	return array;
}


static void put_op(iop_pt op){
	switch (op -> op){
	case I_NOP:
		break;

	case I_ASS :
		break;

	case I_REF :
		break;

	case I_DREF:
		break;

	case I_NEG :
		break;

	case I_NOT :
		break;

	case I_INV :
		break;

	case I_INC :
		break;

	case I_DEC :
		break;

	case I_MUL:
		break;

	case I_DIV:
		break;

	case I_MOD:
		break;

	case I_EXP:
		break;

	case I_LSH:
		break;

	case I_RSH:
		break;

	case I_ADD :
		break;

	case I_SUB :
		break;

	case I_BAND:
		break;

	case I_BOR :
		break;

	case I_XOR :
		break;

	case I_EQ :
		break;

	case I_NEQ:
		break;

	case I_LT :
		break;

	case I_GT :
		break;
	
	case I_LTE:
		break;
	
	case I_GTE:
		break;
	
	case I_AND:
		break;
	
	case I_OR :
		break;
	
	case I_JMP :
		break;
	
	case I_JZ  :
		break;
	
	case I_CALL:
		break;
	
	case I_PUSH:
		break;
	
	case I_RTRN:
		break;
	
	case NUM_I_CODES:
	default: crit_error("something done broke in put_operation()");
	}
}

static void Gen_blk(FILE * out_fd, Instruction_Queue * blk){
	iop_pt iop;
	
//	iop = (icmd*) DS_first(blk);
//	do{
//		// Getreg
//		
//		// find a copy of arg1 and put it in result
//		
//		// generate op res arg2
//		
//		// if arg1 or arg2 are not live remove them from the reg_d
//		
//	} while (( iop = (icmd*) DS_next(blk) ));
//	
	// commit live symbols in regs to memory
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/

/**	Produces a NASM file from the block queue
*/
void x86 (char * filename, bool B64){
	FILE * out_fd;
	
	info_msg("\tx86(): start");
	
	out_fd = fopen(filename, "w");
	
	// Initialize the register descriptor
	memset(reg_d, 0, sizeof(sym_pt)*NUM_X86_REG);
	
	
//	if(!blk_pt) crit_error("x86(): Empty block queue");
//	
//	fprintf(out_fd,"\nsection .text\t; Program code\n");
//	
//	do{
//		Gen_blk(out_fd, *blk_pt);
//	} while(( blk_pt = (DS_pt) DS_next(prog->block_q) ));
//	
//	
//	fprintf(out_fd,"\nsection .data\t; Data Section contains constants\n");
//	fprintf(out_fd,"\nsection .bss\t; Declare static variables\n");
//	if (B64) fprintf(out_fd,"align 8\n");
//	else fprintf(out_fd,"align 4\n");
	
	//TODO: static variables
	
	fclose(out_fd);
	out_fd = NULL;
	
	info_msg("\tx86(): stop");
}


