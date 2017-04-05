/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the licence terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/MPL
 *
 ******************************************************************************/

/**	@file gen-x86.cpp
 *	
 *	generates x86 assembler code from the portable program data.
 */




#include <util/types.h>
#include <util/msg.h>

#include <stdio.h>
#include <string.h>

#include <mpl/ppd.hpp>
#include <mpl/gen.hpp>


/******************************************************************************/
//                               DEFINITIONS
/******************************************************************************/


typedef enum{
	A,  ///< Accumulator
	B,  ///< General Purpose
	C,  ///< Counter
	D,  ///< Data
	SI, ///< Source Index
	DI, ///< Destination Index
	BP, ///< Base Pointer (General Purpose)
	SP, ///< Stack Pointer
	R8, R9, R10, R11, R12, R13, R14, R15, ///< General Purpose
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
	X_MOV,   ///< copy data to a new location
	X_MOVSX, ///< copy data to a larger location and sign extend
	X_MOVZX, ///< copy data to a larger location and zero extend
	
	X_PUSH,   ///< push data onto the stack
	X_POP,    ///< pop data from the stack
	X_PUSHAD, ///< push all registers onto the stack (invalid in 64)
	X_POPAD,  ///< pop all registers from the stack (invalid in 64)
	
	X_INC, ///< Increment
	X_DEC, ///< decrement
	X_ADD,
	X_SUB,
	X_MUL,  ///< unsigned multiplication
	X_IMUL, ///< signed multiplication
	X_DIV,  ///< unsigned division
	X_IDIV, ///< Signed division
	X_CMP,  ///< sets the status flags as if subtraction had occured
	X_NEG,
	
	X_AND,
	X_OR,
	X_XOR,
	X_NOT,
	X_TEST, ///< sets the status flags as if AND had occured
	
	X_SHR, ///< bit shift right
	X_SHL, ///< bit shift left
	X_ROR, ///< bit rotate right
	X_ROL, ///< bit rotate left
	
	X_JMP,   ///< jump
	X_JZ,    ///< jump if zero
	X_JNZ,   ///< jump if non-zero
	X_LOOP,  ///< loop with RCX counter
	X_CALL,  ///< call a procedure
	X_RET,   ///< return from a procedure
	X_IRET,  ///< return from interrupt
	X_INT,   ///< software interrupt
	X_BOUND, ///< check an index is within array bounds
	X_ENTER,
	X_LEAVE,
	
	NUM_X86_INST
} x86_inst;


/******************************************************************************/
//                  GLOBAL CONSTANTS IN THE GEN-X86 MODULE
/******************************************************************************/


const char * inst_array[NUM_X86_INST] = {
	"mov", "movsx", "movzx",
	"push", "pop", "pushad", "popad",
	"inc", "dec", "add", "sub", "mul", "imul", "div", "idiv", "cmp", "neg",
	"and", "or", "xor", "not", "test",
	"shr", "shl", "ror", "rol",
	"jump", "jz", "jnz", "loop", "call", "ret", "iret", "int", "bound", "enter",
	"leave"
};


/******************************************************************************/
//                              MODULE VARIABLES
/******************************************************************************/


/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 */
static op_pt reg_d[NUM_X86_REG];


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// runs the various functions through their various outputs.
void test_x86(void);


static const char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
	return array;
}

// assumes width is possible in current mode
static const char * str_reg(reg_width width, reg_t reg){
	static char array[4] = "   ";
	
	switch (width){
	case byte : array[0] = ' '; array[2] = ' '; break;
	case word : array[0] = ' '; array[2] = 'x'; break;
	case dword: array[0] = 'e'; array[2] = 'x'; break;
	case qword: array[0] = 'r'; array[2] = 'x'; break;
	default   : msg_print(NULL, V_ERROR, "something done broke in put_reg()");
	}
	
	switch (reg){
	case A  : array[1] = 'a'; break;
	case B  : array[1] = 'b'; break;
	case C  : array[1] = 'c'; break;
	case D  : array[1] = 'd'; break;
	case SI : array[1] = 's'; array[2] = 'i'; break;
	case DI : array[1] = 'd'; array[2] = 'i'; break;
	case BP : array[1] = 'b'; array[2] = 'p'; break;
	case SP : array[1] = 's'; array[2] = 'p'; break;
	case R8 : array[1] = '8'; array[2] = ' '; break;
	case R9 : array[1] = '9'; array[2] = ' '; break;
	case R10: array[1] = '1'; array[2] = '0'; break;
	case R11: array[1] = '1'; array[2] = '1'; break;
	case R12: array[1] = '1'; array[2] = '2'; break;
	case R13: array[1] = '1'; array[2] = '3'; break;
	case R14: array[1] = '1'; array[2] = '4'; break;
	case R15: array[1] = '1'; array[2] = '5'; break;
	case NUM_X86_REG:
	default: msg_print(NULL, V_ERROR, "something done broke in put_reg()");
	}
	
	return array;
}

static const char * str_instruction(x86_inst instruction){
	return inst_array[instruction];
}

static void put_op(inst_pt inst){
	switch (inst->op){
	
	case i_nop : break;
	case i_ass : break;
	case i_ref : break;
	case i_dref: break;
	case i_neg : break;
	case i_not : break;
	case i_inv : break;
	case i_inc : break;
	case i_dec : break;
	case i_sz  : break;
	case i_mul : break;
	case i_div : break;
	case i_mod : break;
	case i_exp : break;
	case i_lsh : break;
	case i_rsh : break;
	case i_add : break;
	case i_sub : break;
	case i_band: break;
	case i_bor : break;
	case i_xor : break;
	case i_eq  : break;
	case i_neq : break;
	case i_lt  : break;
	case i_gt  : break;
	case i_lte : break;
	case i_gte : break;
	case i_and : break;
	case i_or  : break;
	case i_jmp : break;
	case i_jz  : break;
	
	case i_NUM:
	default: msg_print(NULL, V_ERROR, "something done broke in put_operation()");
	}
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


/**	Produces a NASM file from the Portable Program Data
*/
void x86 (FILE * out_fd, Blocked_PD prog, x86_mode_t mode){
	msg_print(NULL, V_INFO, "x86(): start");
	
	if(mode == xm_real){
		msg_print(NULL, V_ERROR, "Real Mode not supported");
		return;
	}
	if(mode == xm_smm){
		msg_print(NULL, V_ERROR, "System Management Mode not supported");
		return;
	}
	
	// Initialize the register descriptor
	memset(reg_d, 0, sizeof(op_pt)*NUM_X86_REG);
	
	
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
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


