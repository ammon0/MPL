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

#include <mpl/ppd.hpp>
#include <mpl/gen.hpp>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>


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
	bad_width,
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


static const char * inst_array[NUM_X86_INST] = {
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
static op_pt          reg_d[NUM_X86_REG];
static Operands     * ops               ;
static String_Array * lbls              ;
static FILE         * fd                ;
static x86_mode_t     mode              ;


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// runs the various functions through their various outputs.
void test_x86(void);


/****************************** HELPER FUNCTIONS ******************************/

static const char * str_instruction(x86_inst instruction){
	return inst_array[instruction];
}
static const char * str_lbl(Operand * operand){
	return lbls->get(operand->label);
}

static const char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
	return array;
}

// assumes width is possible in current mode
static const char * str_reg(reg_width width, reg_t reg){
	static char array[4] = "   ";
	
	//TODO: check mode
	
	switch (width){
	case byte : array[0] = ' '; array[2] = ' '; break;
	case word : array[0] = ' '; array[2] = 'x'; break;
	case dword: array[0] = 'e'; array[2] = 'x'; break;
	case qword: array[0] = 'r'; array[2] = 'x'; break;
	
	case bad_width:
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad reg_width");
		return NULL;
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
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad reg_t");
		return NULL;
	}
	
	return array;
}

static void __attribute__((format(printf, 1, 2)))
put_cmd(const char * format, ...){
	va_list ap;
	
	va_start(ap, format);
	vfprintf(fd, format, ap);
	va_end(ap);
}

static reg_width set_width(width_t in){
	switch(in){
	case w_byte : return byte;
	case w_byte2: return word;
	case w_byte4: return dword;
	case w_byte8: return qword;
	case w_word :
	case w_max  :
	case w_ptr  :
		if(mode == xm_long) return qword;
		else return dword;
	
	case w_none :
	case w_NUM  :
	default     :
		msg_print(NULL, V_ERROR, "set_width(): got a bad width");
		return bad_width;
	}
}

static reg_t check_reg(op_pt operand){
	uint i;
	
	for(i=A; i!=NUM_X86_REG; i++){
		reg_t j = static_cast<reg_t>(i);
		if(reg_d[j] == operand) break;
	}
	return (reg_t)i;
}

static return_t
load(reg_t reg, op_pt mem){
	if(reg_d[reg] != NULL){
		msg_print(NULL, V_ERROR, "load(): that register is already in use");
		return failure;
	}
	
	reg_d[reg] = mem;
	put_cmd("\t%s\t%s\t[%s]\n",
		str_instruction(X_MOV),
		str_reg(set_width(mem->width), reg),
		str_lbl(mem)
	);
	
	return success;
}

static return_t
store(reg_t reg){
	if(reg_d[reg] == NULL){
		msg_print(
			NULL,
			V_ERROR,
			"store(): there is no memory location associated with register %u",
			reg
		);
		return failure;
	}
	
	put_cmd("\t%s\t%s\t%s",
		str_instruction(X_MOV),
		str_lbl(reg_d[reg]),
		str_reg(set_width(reg_d[reg]->width), reg)
	);
	
	reg_d[reg] = NULL;
	return success;
}

/*************************** INSTRUCTION FUNCTIONS ****************************/
// Alfabetical

static return_t
ass(op_pt result, op_pt arg){
	
	check_reg(result);
	check_reg(arg);
	
	return success;
}

static return_t
neg(op_pt result, op_pt arg){
	
	check_reg(result);
	check_reg(arg);
	
	return success;
}

/***************************** ITERATOR FUNCTIONS *****************************/

static return_t Gen_inst(inst_pt inst){
	switch (inst->op){
	case i_nop : return success;
	
	case i_ass : return ass(inst->result, inst->left);
	case i_ref : break;
	case i_dref: break;
	case i_neg : return neg(inst->result, inst->left);
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
	case i_rol : break;
	case i_ror : break;
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
	
	case i_lbl : break;
	case i_jmp : break;
	case i_jz  : break;
	case i_loop: break;
	case i_call: break;
	case i_rtrn: break;
	
	case i_NUM:
	default: msg_print(NULL, V_ERROR, "Gen_inst(): got a bad inst_code");
	}
	
	return success;
}


static return_t Gen_blk(Instructions * blk){
	inst_pt inst;
	
	if(!( inst=blk->first() )){
		msg_print(NULL, V_ERROR, "Gen_blk(): received empty block");
		return failure;
	}
	
	do{
		if( Gen_inst(inst) == failure ){
			msg_print(NULL, V_ERROR, "Gen_blk(): Gen_inst() failed");
			return failure;
		}
	}while(( inst=blk->next() ));
	
	// flush the registers at the end of the block
	for(uint i=0; i<NUM_X86_REG; i++){
		if(reg_d[i] != NULL) store((reg_t)i);
	}
	
	return success;
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


/**	Produces a NASM file from the Portable Program Data
*/
void x86 (FILE * out_fd, PPD * prog, x86_mode_t set_mode){
	Instructions * blk = NULL;
	
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
	ops  = &(prog->operands);
	lbls = &(prog->labels);
	fd   = out_fd;
	mode = set_mode;
	
	if(!( blk=prog->bq.first() )){
		msg_print(NULL, V_ERROR, "x86(): Empty block queue");
		return;
	}
	
	fprintf(out_fd,"\nsection .text\t; Program code\n");
	
	do{
		Gen_blk(blk);
	} while(( blk=prog->bq.next() ));
	
	
	fprintf(out_fd,"\nsection .data\t; Data Section contains constants\n");
	fprintf(out_fd,"\nsection .bss\t; Declare static variables\n");
	if (mode == xm_long) fprintf(out_fd,"align 8\n");
	else fprintf(out_fd,"align 4\n");
	
	//TODO: static variables
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


