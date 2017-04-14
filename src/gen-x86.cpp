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
 *	Parameters are stored in R8-15, BP, B, SI, DI, C, D, A in that order. If
 *	there are more parameters than this they will be placed on the stack.
 *
 *	## Activation Record
 *
 *	<PRE>
 *	   Activation Record    Instructions
 *	|_____________________| <- SP before
 *	|additional parameters|        caller's responsability
 *	|_____________________|
 *	|          IP         | CALL / RET
 *	|_____________________| ____________________________
 *	|  parameter storage  |
 *	|_____________________|
 *	| automatic variables |        callee's responsability
 *	|_____________________|
 *	|     temporaries     |
 *	|_____________________| <- SP after
 *	</PRE>
 *
 *	### Calling convention
 *
 *	* MOV [SP-offset] caller loads stack parameters as offsets of SP
 *	* MOV caller loads parameters into registers
 *	* SUB SP, param_offset; caller advances SP to next open location this offset
 *	is calculated at compile time.
 *	* CALL caller calls callee IP is pushed onto stack
 *	* SUB SP, auto_offset; callee advances SP for all its automatic storage.
 *	this offset is known at compile time.
 *
 *	auto variables are accessed as [SP+offset]. additional parameters are
 *	accessed as [SP+auto_offset+8+offset]
 *
 *	It may be possible to add dynamically sized variables after SP with pointers
 *	before it.
 *
 *	If we do ever implement closures this scheme will become obsolete since we'll
 *	have to manage activation records in the heap. We will then have no use for
 *	the built-in stack functions and SP will become general purpose.
 *
 *	## Code Generation
 *	x86 instructions typically replace the left operand with the result. Since
 *	results will always be temporaries, and temporaries are only ever used once.
 *	In this generator will will try to make the left operand and the result
 *	always be the accumulator. This creates an opportunity for an optimizer that
 *	algebraically rearranges things.
 *
 *	## Register Allocation
 *	There is a problem if a temp variable is not immediatly used. All temps are
 *	produced in the accumulator. If at any point in code generation we find that
 *	the contents of the accumulator is temp and not being used in the current
 *	instruction then it will have to be stored somewhere. A register would be
 *	ideal. However, all the registers are potentially filled with parameters. We
 *	have no way of knowing whether it would be better to switch out a parameter.
 *	We do know that the temp will only ever be used once so I feel less bad
 *	about putting it on the stack if there are no registers availible.
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
	R8,  ///< General Purpose
	R9,  ///< General Purpose
	R10, ///< General Purpose
	R11, ///< General Purpose
	R12, ///< General Purpose
	R13, ///< General Purpose
	R14, ///< General Purpose
	R15, ///< General Purpose
	NUM_reg
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
	X_MOV,      ///< copy data to a new location
	X_MOVSX,   ///< copy 8, or 16-bit data to a larger location and sign extend
	X_MOVSXD, ///< copy 32-bit data to a 64-bit register and sign extend
	X_MOVZX, ///< copy 8, or 16-bit data to a larger location and zero extend
	X_MOVBE, ///< copy and swap byte order
	
	X_CLD, ///< clear DF
	X_STD, ///< set DF
	X_STOS, ///< store string. uses A, DI, ES, and DF flag
	X_LODS, ///< load string. uses A, SI, DS, and DF flag
	X_MOVS, ///< copy from memory to memory. uses DI, SI, DS, and DF flag
	X_OUTS, ///< write a string to IO. uses DS, SI, D, and DF flag
	X_INS,  ///< read a string from IO. 
	
	X_IN,  ///< read from an IO port
	X_OUT, ///< write to an IO port
	
	X_PUSH,   ///< push data onto the stack
	X_POP,    ///< pop data from the stack
	
	X_INC, ///< Increment
	X_DEC, ///< decrement
	X_ADD,
	X_XADD, ///< exchange then add
	X_SUB,
	X_NEG,
	X_CMP,  ///< sets the status flags as if subtraction had occured
	
	X_MUL,  ///< unsigned multiplication
	X_IMUL, ///< signed multiplication
	X_DIV,  ///< unsigned division
	X_IDIV, ///< Signed division
	
	X_AND,
	X_ANDN, ///< AND NOT
	X_OR,
	X_XOR,
	X_NOT,
	X_TEST, ///< sets the status flags as if AND had occured
	
	X_SHR, ///< bit shift right unsigned
	X_SAR, ///< bit shift right signed (behavior is not the same as idiv)
	X_SHL, ///< bit shift left
	X_ROR, ///< bit rotate right
	X_ROL, ///< bit rotate left
	
	X_JMP,   ///< jump
	X_JZ,    ///< jump if zero
	X_JNZ,   ///< jump if non-zero
	X_LOOP,  ///< loop with RCX counter
	X_CALL,  ///< call a procedure
	X_RET,   ///< return from a procedure
	X_INT,   ///< software interrupt
	X_BOUND, ///< check an index is within array bounds
	X_ENTER,
	X_LEAVE,
	
	NUM_inst
} x86_inst;


/******************************************************************************/
//                  GLOBAL CONSTANTS IN THE GEN-X86 MODULE
/******************************************************************************/


/// the assembler strings for each x86 instruction
static const char * inst_array[NUM_inst] = {
	"mov", "movsx", "movsxd", "movzx", "movbe",
	"cld", "std", "stos", "lods", "movs", "outs", "ins",
	"in", "out",
	"push", "pop",
	"inc", "dec", "add", "xadd", "sub", "neg", "cmp",
	"mul", "imul", "div", "idiv",
	"and", "andn", "or", "xor", "not", "test",
	"shr", "sar", "shl", "ror", "rol",
	"jmp", "jz", "jnz", "loop", "call", "ret", "int", "bound", "enter", "leave"
};


/******************************************************************************/
//                              MODULE VARIABLES
/******************************************************************************/


/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 */
static op_pt          reg_d[NUM_reg];

static Operands     * operands; ///< the operand index for the PPD
static String_Array * strings ; ///< the string space for the PPD
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
static inline const char * str_instruction(x86_inst instruction){
	return inst_array[instruction];
}

/** Return a string representation of a number.
 */
static inline const char * str_num(umax num){
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
	
	case NUM_reg:
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad reg_t");
		return NULL;
	}
	
	return array;
}

/** Return a string to access an operand
*/
static inline const char * str_oprand(op_pt op){
	static char arr[3][24];
	static uint i;

	switch(op->type){
	// we include these as immediate values
	case st_const: return str_num(op->const_value);
	
	// these are read from memory
	case st_static:
	case st_string:
	case st_code  : return strings->get(op->label);
	
	// these are read from the stack
	case st_auto: // SP-offset
		i = i+1 %3; // increment i
		sprintf(arr[i], "SP-%s", str_num(op->SP_offset));
		return arr[i];
	
	// these are in registers
	case st_temp:
		msg_print(NULL, V_ERROR, "Internal str_oprand(): got a temp");
		return "!!bad!!";
	
	case st_none:
	case st_NUM:
	default:
		msg_print(NULL, V_ERROR, "Internal str_oprand(): bad type");
		return "!!bad!!";
	}
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
	
	for(i=A; i!=NUM_reg; i++){
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
		str_oprand(mem)
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
	
	if(reg_d[reg]->type != st_static || reg_d[reg]->type != st_auto)
		return success;
	
	put_cmd("\t%s\t[%s]\t%s",
		str_instruction(X_MOV),
		str_oprand(reg_d[reg]),
		str_reg(set_width(reg_d[reg]->width), reg)
	);
	return success;
}

/** Return an appropriate register to store a result in.
 */
static reg_t get_reg(op_pt arg){
	reg_t reg;
	
	
	// find empty space
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
	
	// make space
	if(store(A) == failure){
		msg_print(NULL, V_ERROR, "Internal get_reg(): store() failed");
		return NUM_reg;
	}
	reg_d[A] = NULL;
	return A;
}

/** Ensures that the accumulator contains the given operand.
*/
static void prep_accum(op_pt){
	
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

static void lbl(op_pt op){
	put_cmd("%s:\n", strings->get(op->label));
}

/// gets the address of a variable
static RETURN ref(op_pt result, op_pt arg){
	reg_t result_reg;
	
	if(arg->type != st_static || arg->type != st_auto){
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
		str_oprand(arg)
	);
	
	return success;
}

static RETURN unary(op_pt result, op_pt arg, x86_inst op){
	reg_t result_reg, arg_reg;
	
	result_reg = get_reg(arg);
	arg_reg = check_reg(arg);
	
	if(arg_reg == NUM_reg){
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
	/* How to Process an Instruction
	 *
	 *
	 * Since temporaries are single use they can be completely covered by the
	 * accumulator. 
	 */
	
	
	switch (inst->op){
	case i_nop : return success;
	
	
	case i_inc : return unary(inst->result, inst->left, X_INC);
	case i_dec : return unary(inst->result, inst->left, X_DEC);
	case i_ass : return ass(inst->result, inst->left);
	
	case i_ref : return ref(inst->result, inst->left);
	case i_dref: return dref(inst->result, inst->left);
	case i_sz  : break;
	
	/* Each of these instructions is destructive to the left operand. In other words the result is stored in the same register as the left operand.
	*/
	case i_neg : return unary(inst->result, inst->left, X_NEG);
	case i_not : return unary(inst->result, inst->left, X_NOT);
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
	
	// these are probably all implemented with X_CMP
	case i_eq  : break;
	case i_neq : break;
	case i_lt  : break;
	case i_gt  : break;
	case i_lte : break;
	case i_gte : break;
	
	case i_inv : break;
	case i_and : break;
	case i_or  : break;
	
	case i_lbl : lbl(inst->left); break;
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
static RETURN Gen_blk(blk_pt blk){
	inst_pt inst;
	
	// Initialize the register descriptor
	memset(reg_d, 0, sizeof(op_pt)*NUM_reg);
	
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
	for(uint i=0; i<NUM_reg; i++){
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
	blk_pt blk = NULL;
	op_pt op = NULL;
	
	msg_print(NULL, V_INFO, "x86(): start");
	
	if(mode == xm_real){
		msg_print(NULL, V_ERROR, "Real Mode not supported");
		return;
	}
	if(mode == xm_smm){
		msg_print(NULL, V_ERROR, "System Management Mode not supported");
		return;
	}
	
	operands = &(prog->operands);
	strings  = &(prog->strings);
	fd   = out_fd;
	mode = proccessor_mode;
	
	if(!( blk=prog->instructions.first() )){
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
	} while(( blk=prog->instructions.next() ));
	
	// string constants
	fprintf(out_fd,"\nsection .data\t; Data Section contains constants\n");
	op = operands->first();
	do{
		if(op->type == st_string)
			fprintf(out_fd, "%s:\t\"%s\"\n",
				strings->get(op->label),
				strings->get(op->string_content)
			);
	}while(( op = operands->next() ));
	
	// static variables
	fprintf(out_fd,"\nsection .bss\t; Declare static variables\n");
	if (mode == xm_long) fprintf(out_fd,"align 8\n");
	else fprintf(out_fd,"align 4\n");
	op = operands->first();
	do{
		if(op->type == st_static)
			switch(set_width(op->width)){
			case byte:
				fprintf(out_fd, "%s:\tdb 0x0\n", strings->get(op->label));
				break;
			case word:
				fprintf(out_fd, "%s:\tdw 0x0\n", strings->get(op->label));
				break;
			case dword:
				fprintf(out_fd, "%s:\tdd 0x0\n", strings->get(op->label));
				break;
			case qword:
				fprintf(out_fd, "%s:\tdq 0x0\n", strings->get(op->label));
				break;
			case bad_width:
			default:
				msg_print(NULL, V_ERROR,
					"Internal x86(): set_width() returned invalid");
				fprintf(out_fd, "%s:\t!!bad!! 0x0\n", strings->get(op->label));
			}
	}while(( op = operands->next() ));
	
	//TODO: static variables
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


