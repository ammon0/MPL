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
 *
 *	## Parameters
 *	Parameters and return values are passed in registers reducing the stack
 *	overhead. This also means that many of the registers need not be stored in
 *	the stack before the call.
 *
 *	Since some of the registers may be needed during the execution of a routine,
 *	the routine should allocate automaic variable space for each. If the routine
 *	makes a CALL it will need to store them all in their stack locations.
 *
 *	Parameters are stored in B, BP, SI, DI, C, D, A in that order
 *
 *	## Activation Record
 *	   Activation Record    Instructions
 *	|_____________________|
 *	|          SP         | PUSH / POP
 *	|_____________________|               caller's responsability
 *	|          IP         | CALL / RET
 *	|_____________________| ____________________________
 *	|  parameter storage  | <- SP        callee's responsability
 *	|_____________________|
 *	| automatic variables |
 *	|_____________________|
 *
 *	When each routine is called it must setup its own automaic variables as
 *	offsets of SP. But SP does not change. This way all the auto variables do
 *	not need to be recalculated each time another is added, and their size can
 *	be determined at runtime. The caller must push SP and then set SP past its
 *	own auto variables. On return it must pop SP. The callee need only call RET
 *	which works on the current SP automaically.
 *
 *	If we don't acutually use push and pop to setup the automaic variables then
 *	SP stays where it is, all of our offsets are calculated according to SP and
 *	BP becomes a general purpose register.
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


/// These are all the x86 "general purpose" registers
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

/// These are the x86 register sizes
typedef enum {
	bad_width,
	byte,
	word,
	dword,
	qword
} reg_width;


/// These are the x86 instructions that we are working with
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


/// the assembler strings for each x86 instruction
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

static Operands     * ops ; ///< the operand index for the PPD
static String_Array * lbls; ///< the string space for the PPD
static FILE         * fd  ; ///< the output file descriptor
static x86_mode_t     mode; ///< the processor mode we are building for


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// runs the various functions through their various outputs.
void test_x86(void);


/****************************** HELPER FUNCTIONS ******************************/

/** Return the correct assembler string for the given x86 instruction.
 */
static const char * str_instruction(x86_inst instruction){
	return inst_array[instruction];
}

/** Return the label string for the given operand.
 */
static const char * str_lbl(Operand * operand){
	return lbls->get(operand->label);
}

/** Return a string representation of a number.
 */
static const char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
	return array;
}

/** Return the appropriate string to use the given x86 register.
 */
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

/** Add a command string to the output file.
 */
static void __attribute__((format(printf, 1, 2)))
put_cmd(const char * format, ...){
	va_list ap;
	
	va_start(ap, format);
	vfprintf(fd, format, ap);
	va_end(ap);
}

/** Return the appropriate x86 register width for the given data size.
 */
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

/** Determine whether an operand is already present in a register.
 */
static reg_t check_reg(op_pt operand){
	uint i;
	
	for(i=A; i!=NUM_X86_REG; i++){
		reg_t j = static_cast<reg_t>(i);
		if(reg_d[j] == operand) break;
	}
	return (reg_t)i;
}

/** Load data from memory to a register.
 */
static RETURN load(reg_t reg, op_pt mem){
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

/** Store data in register to its appropriate memory location.
 */
static RETURN store(reg_t reg){
	if(reg_d[reg] == NULL){
		msg_print(
			NULL,
			V_ERROR,
			"store(): there is no memory location associated with register %u",
			reg
		);
		return failure;
	}
	
	if(reg_d[reg]->type != st_data) return success;
	
	put_cmd("\t%s\t%s\t%s",
		str_instruction(X_MOV),
		str_lbl(reg_d[reg]),
		str_reg(set_width(reg_d[reg]->width), reg)
	);
	return success;
}

/** Return an appropriate register to store a result in.
 */
static reg_t get_reg(op_pt arg){
	reg_t reg;
	
	if( arg && (reg=check_reg(arg)) != NUM_X86_REG ) return reg;
	
	// find empty space
	if(!reg_d[A]) return A; // general purpose
	if(!reg_d[B]) return B;
	if(!reg_d[BP]) return BP;
	if(mode == xm_long){
		if(!reg_d[R8]) return R8;
		if(!reg_d[R9]) return R9;
		if(!reg_d[R10]) return R10;
		if(!reg_d[R11]) return R11;
		if(!reg_d[R12]) return R12;
		if(!reg_d[R13]) return R13;
		if(!reg_d[R14]) return R14;
		if(!reg_d[R15]) return R15;
	}
	if(!reg_d[C]) return C; // special purpose, last priority
	if(!reg_d[D]) return D;
	if(!reg_d[SI]) return SI;
	if(!reg_d[DI]) return DI;
	
	// make space
	if(store(A) == failure){
		msg_print(NULL, V_ERROR, "Internal get_reg(): store() failed");
		return NUM_X86_REG;
	}
	reg_d[A] = NULL;
	return A;
}

/*************************** INSTRUCTION FUNCTIONS ****************************/
// Alfabetical

static RETURN ass(op_pt result, op_pt arg){
	
	check_reg(result);
	check_reg(arg);
	
	return success;
}

static RETURN binary(op_pt result, op_pt left, op_pt right, x86_inst op){
	reg_t result_reg, left_reg, right_reg;
	
	result_reg = get_reg(left);
	left_reg = check_reg(left);
	right_reg = check_reg(right);
	
	reg_d[result_reg] = result;
	return success;
}

static RETURN dref(op_pt result, op_pt pointer){
	reg_t result_reg;
	
	
	
	return success;
}

static RETURN ref(op_pt result, op_pt arg){
	reg_t result_reg;
	
	if(arg->type != st_data){
		msg_print(NULL, V_ERROR,
			"Internal dref(): arg is not a memory location");
		return failure;
	}
	
	if(result->width != w_ptr){
		msg_print(NULL, V_ERROR, "Internal dref(): result is not w_ptr");
		return failure;
	}
	
	result_reg = get_reg(NULL);
	
	put_cmd("\t%s\t%s\n%s\n",
		str_instruction(X_MOVZX),
		str_reg(set_width(w_ptr), result_reg),
		str_lbl(arg)
	);
	
	return success;
}

static RETURN unary(op_pt result, op_pt arg, x86_inst op){
	reg_t result_reg, arg_reg;
	
	result_reg = get_reg(arg);
	arg_reg = check_reg(arg);
	
	if(arg_reg == NUM_X86_REG){
		if(load(result_reg, arg) == failure){
			msg_print(NULL, V_ERROR, "Internal neg(): load() failed");
			return failure;
		}
	}else if(arg_reg != result_reg){
		msg_print(NULL, V_ERROR, "Internal neg(): regs do not match");
		return failure;
	}
	
	put_cmd("\t%s\t%s\n",
		str_instruction(op),
		str_reg(set_width(arg->width), result_reg)
	);
	
	reg_d[result_reg] = result;
	return success;
}

/***************************** ITERATOR FUNCTIONS *****************************/

/** Generate code for a single intermediate instruction
 */
static RETURN Gen_inst(inst_pt inst){
	switch (inst->op){
	case i_nop : return success;
	
	// easy unaries
	case i_neg : return unary(inst->result, inst->left, X_NEG);
	case i_not : return unary(inst->result, inst->left, X_NOT);
	case i_inc : return unary(inst->result, inst->left, X_INC);
	case i_dec : return unary(inst->result, inst->left, X_DEC);
	
	case i_ass : return ass(inst->result, inst->left);
	case i_ref : return ref(inst->result, inst->left);
	case i_dref: return dref(inst->result, inst->left);
	case i_sz  : break;
	
	// easy binaries
	case i_lsh : return binary(inst->result, inst->left, inst->right, X_SHL);
	case i_rsh : return binary(inst->result, inst->left, inst->right, X_SHR);
	case i_rol : return binary(inst->result, inst->left, inst->right, X_ROL);
	case i_ror : return binary(inst->result, inst->left, inst->right, X_ROR);
	
	case i_add : return binary(inst->result, inst->left, inst->right, X_ADD);
	case i_sub : return binary(inst->result, inst->left, inst->right, X_SUB);
	case i_band: return binary(inst->result, inst->left, inst->right, X_AND);
	case i_bor : return binary(inst->result, inst->left, inst->right, X_OR);
	case i_xor : return binary(inst->result, inst->left, inst->right, X_XOR);
	
	case i_mul : break;
	case i_div : break;
	case i_mod : break;
	case i_exp : break;
	
	case i_eq  : break;
	case i_neq : break;
	case i_lt  : break;
	case i_gt  : break;
	case i_lte : break;
	case i_gte : break;
	
	case i_inv : break;
	case i_and : break;
	case i_or  : break;
	
	case i_lbl : break;
	case i_jmp : break;
	case i_jz  : break;
	case i_loop: break;
	case i_parm: break;
	case i_call: break;
	case i_rtrn: break;
	
	case i_NUM:
	default: msg_print(NULL, V_ERROR, "Gen_inst(): got a bad inst_code");
	}
	
	return success;
}

/** Generate code for a single basic block
 */
static RETURN Gen_blk(Instructions * blk){
	inst_pt inst;
	
	// Initialize the register descriptor
	memset(reg_d, 0, sizeof(op_pt)*NUM_X86_REG);
	
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
		if(reg_d[i] != NULL)
			if( store((reg_t)i) == failure){
				msg_print(NULL, V_ERROR, "Internal Gen_blk(): store() failed");
				return failure;
			}
	}
	
	return success;
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


void x86 (FILE * out_fd, PPD * prog, x86_mode_t proccessor_mode){
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
	
	ops  = &(prog->operands);
	lbls = &(prog->labels);
	fd   = out_fd;
	mode = proccessor_mode;
	
	if(!( blk=prog->bq.first() )){
		msg_print(NULL, V_ERROR, "x86(): Empty block queue");
		return;
	}
	
	fprintf(out_fd,"\nsection .text\t; Program code\n");
	
	do{
		if(Gen_blk(blk) == failure){
			msg_print(NULL, V_ERROR,
				"Internal x86(): Gen_blk() returned failure"
			);
			return;
		}
	} while(( blk=prog->bq.next() ));
	
	
	fprintf(out_fd,"\nsection .data\t; Data Section contains constants\n");
	
	//TODO: constant strings
	
	fprintf(out_fd,"\nsection .bss\t; Declare static variables\n");
	if (mode == xm_long) fprintf(out_fd,"align 8\n");
	else fprintf(out_fd,"align 4\n");
	
	//TODO: static variables
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


