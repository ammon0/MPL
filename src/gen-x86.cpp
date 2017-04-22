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
 *	## Code Generation
 *	x86 instructions typically replace the left operand with the result. Since
 *	results will always be temporaries, and temporaries are only ever used once.
 *	In this generator will will try to make the left operand and the result
 *	always be the accumulator. This creates an opportunity for an optimizer that
 *	algebraically rearranges things.
 *
 *	## Register Allocation
 *	There is a problem if a temp variable is not immediately used. All temps are
 *	produced in the accumulator. If at any point in code generation we find that
 *	the contents of the accumulator is temp and not being used in the current
 *	instruction then it will have to be stored somewhere. A register would be
 *	ideal. However, all the registers are potentially filled with parameters. We
 *	have no way of knowing whether it would be better to switch out a parameter.
 *	We do know that the temp will only ever be used once so I feel less bad
 *	about putting it on the stack if there are no registers available.
 *
 *	## Activation Record
 *
 *	<PRE>
 *	   Activation Record    Instructions
 *	|_____________________|
 *	|     parameters      | PUSH params
 *	|_____________________|
 *	|       R8-R15        | PUSH R / POP R
 *	|_____________________|
 *	|        B-DI         | PUSH A / POP A
 *	|_____________________|
 *	|         BP          | PUSH BP / POP BP
 *	|_____________________|
 *	|         IP          | CALL / RET
 *	|_____________________| ______________________________<- BP
 *	| automatic variables | PUSH 0x0 / POP
 *	|_____________________|
 *	|     temporaries     | PUSH temp
 *	|_____________________|
 *	
 *	</PRE>
 *
 *	SP is always pointing to the top item on the stack, so
 *	PUSH=DEC,MOV POP=MOV,INC
 *
 *	To make life easier on myself I've decided to use SP and BP for their
 *	intendend purposes. In long mode the first 8 parameters will be passed in
 *	R8-15 with any others in the stack. In protected mode they are all passed on
 *	the stack.
 *
 *	## Parameters
 *	Parameters and return values are passed in registers reducing the stack
 *	overhead. This also means that many of the registers need not be stored in
 *	the stack before the call.
 *
 *	Since some of the registers may be needed during the execution of a routine,
 *	the routine should allocate automatic variable space for each. If the routine
 *	makes a CALL it will need to store them all in their stack locations.
 *
 *	Parameters are stored in R8-15, BP, B, SI, DI, C, D, A in that order. If
 *	there are more parameters than this they will be placed on the stack.
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
	BP, ///< Base Pointer
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
	bad_width=0,
	byte     =1,
	word     =2,
	dword    =4,
	qword    =8
} reg_width;


/// These are the x86 instructions that we are working with
typedef enum {
	X_MOV,      ///< copy data to a new location
	X_MOVSX,   ///< copy 8, or 16-bit data to a larger location and sign extend
	X_MOVSXD, ///< copy 32-bit data to a 64-bit register and sign extend
	X_MOVZX, ///< copy 8, or 16-bit data to a larger location and zero extend
	X_MOVBE, ///< copy and swap byte order
	X_LEA,  ///< load effective address
	
	X_CLD, ///< clear DF
	X_STD, ///< set DF
	X_STOS, ///< store string. uses A, DI, ES, and DF flag
	X_LODS, ///< load string. uses A, SI, DS, and DF flag
	X_MOVS, ///< copy from memory to memory. uses DI, SI, DS, and DF flag
	X_OUTS, ///< write a string to IO. uses DS, SI, D, and DF flag
	X_INS,  ///< read a string from IO.
	X_CMPS, ///< compare arrays
	X_SCAS, ///< scan an array in DI
	
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
	X_CMP,  ///< sets the status flags as if subtraction had occurred
	// X_SETcc sets a register based on flag conditions
	
	X_MUL,  ///< unsigned multiplication
	X_IMUL, ///< signed multiplication
	X_DIV,  ///< unsigned division
	X_IDIV, ///< Signed division
	
	X_AND,
	X_ANDN, ///< AND NOT
	X_OR,
	X_XOR,
	X_NOT,
	X_TEST, ///< sets the status flags as if AND had occurred
	
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

/// Keeps track of the stack details
class Stack_man{
	umax  SP_offset;
public:
	Stack_man(void){ SP_offset = 0; }
	
	RETURN push_temp(reg_t reg     ); ///< push as a temp
	void   push_auto(op_pt auto_var); ///< push an automatic variable
	void   push_parm(op_pt parameter); ///< push a parameter onto the stack
	void   set_BP(void     ); ///< advance BP to the SP
	void   unload(uint count); ///< unload parameters from the stack
	void   pop(void); ///< pop the current activation record
};


/******************************************************************************/
//                  GLOBAL CONSTANTS IN THE GEN-X86 MODULE
/******************************************************************************/


/// the assembler strings for each x86 instruction
static const char * inst_array[NUM_inst] = {
	"mov", "movsx", "movsxd", "movzx", "movbe", "lea",
	"cld", "std", "stos", "lods", "movs", "outs", "ins", "cmps", "scas",
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


static Instruction_Queue * instructions; ///< the instructions for the PPD
static Operands          * operands; ///< the operand index for the PPD
static String_Array      * strings ; ///< the string space for the PPD
static FILE              * fd  ; ///< the output file descriptor
static x86_mode_t          mode; ///< the processor mode we are building for

/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 */
static op_pt reg_d[NUM_reg];
static Stack_man stack_manager;

#define STATE_SZ 0 ///< how many bytes are needed to save the processor state


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// runs the various functions through their various outputs.
void test_x86(void);


/************************** STRING WRITING FUNCTIONS **************************/

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
	
	if(mode != xm_long && width == qword){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): qword only availible in long mode");
		return "!!Bad width!!";
	}
	
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
		return "!!bad!!";
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
	case st_auto:
		i = (i+1) %3; // increment i
		sprintf(arr[i], "BP-%s", str_num(op->BP_offset));
		return arr[i];
	
	case st_param:
		i = (i+1) %3; // increment i
		sprintf(arr[i], "BP+%s", str_num(op->BP_offset+STATE_SZ));
		msg_print(NULL, V_WARN, "Internal str_oprand(): bad parameter offset");
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

/***************************** HELPER FUNCTIONS *******************************/

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

/// push an auto variable onto the stack
void Stack_man::push_auto(op_pt auto_var){
	// issue push instruction
	put_cmd("%s\t%s\n",
		str_instruction(X_PUSH),
		str_num(0)
	);
	
	// increment SP_offset
	if(mode == xm_long) SP_offset += qword;
	else SP_offset += dword;
	
	// set the offset
	auto_var->BP_offset = SP_offset;
}

/// push a temp variable onto the stack
RETURN Stack_man::push_temp(reg_t reg){
	
	if(reg_d[reg]->type != st_temp){
		msg_print(NULL, V_ERROR, "Internal push_temp(): Not a temp");
		return failure;
	}
	
	// issue push instruction
	put_cmd("%s\t%s\n",
		str_instruction(X_PUSH),
		str_reg(set_width(reg_d[reg]->width), reg)
		// or should this be the stack width?
	);
	
	// increment SP_offset
	if(mode == xm_long) SP_offset += qword;
	else SP_offset += dword;
	
	reg_d[reg]->BP_offset = SP_offset;
	
	return success;
}

/// advance BP to the next activation record
void Stack_man::set_BP(void){
	put_cmd("\t%s\t%s,\t%s\n",
		str_instruction(X_MOV),
		str_reg(mode == xm_long? qword: dword, BP),
		str_reg(mode == xm_long? qword: dword, SP)
	);
	SP_offset = 0;
}

void Stack_man::pop(void){
	put_cmd(
		"\t%s\t%s,\t%s\n",
		inst_array[X_MOV],
		str_reg(mode == xm_long? qword:dword, SP),
		str_reg(mode == xm_long? qword:dword, BP)
	);
	SP_offset=0;
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

/** Load data from memory to a register.
 */
static void load(reg_t reg, op_pt mem){
	reg_t test_reg;
	
	// check if it's already in a register
	test_reg = check_reg(mem);
	
	if(test_reg == reg) // if it's already where it needs to be
		;
	else if(test_reg != NUM_reg){ // if it's in another register
		put_cmd("%s\t%s,\t%s\n",
			str_instruction(X_MOV),
			str_reg(set_width(mem->width), reg),
			str_reg(set_width(mem->width), test_reg)
		);
		reg_d[test_reg] = NULL;
	}
	else // if it's in memory
		put_cmd("\t%s\t%s,\t[%s]\n",
			str_instruction(X_MOV),
			str_reg(set_width(mem->width), reg),
			str_oprand(mem)
		);
	
	reg_d[reg] = mem;
}

/*************************** INSTRUCTION FUNCTIONS ****************************/
// Alphabetical

/// make an assignment
static inline void ass(op_pt result, op_pt arg){
	load(A, arg);
	reg_d[A] = result;
}

/// most binary operations
static inline void binary(op_pt result, op_pt left, op_pt right, x86_inst op){
	load(A, left);
	load(C, right);
	
	put_cmd("\t%s\t%s,\t%s\n",
		str_instruction(op),
		str_reg(set_width(left->width), A),
		str_reg(set_width(right->width), C)
	);
	
	reg_d[A] = result;
}

/// call a procedure
static inline void call(op_pt result, op_pt proc){
	// parameters are already loaded
	
	// store the processor state
	if(mode == xm_long){
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, B));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, C));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, D));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, SI));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, DI));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, BP));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R8));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R9));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R10));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R11));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R12));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R13));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R14));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(qword, R15));
	}
	else{
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(dword, B));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(dword, C));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(dword, D));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(dword, SI));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(dword, DI));
		put_cmd("%s\t%s\n", str_instruction(X_PUSH), str_reg(dword, BP));
	}
	// do we need to store flags? X_POPF
	
	put_cmd("\t%s\t%s\n", inst_array[X_CALL], strings->get(proc->label));
	
	// restore the processor state
	if(mode == xm_long){
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R15));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R14));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R13));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R12));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R11));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R10));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R9) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, R8) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, BP) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, DI) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, SI) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, D)  );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, C)  );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(qword, B)  );
	}
	else{
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(dword, BP));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(dword, DI));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(dword, SI));
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(dword, D) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(dword, C) );
		put_cmd("%s\t%s\n", str_instruction(X_POP), str_reg(dword, B) );
	}
	
	// unload parameters
	stack_manager.unload(proc->param_cnt());
	
	reg_d[A] = result;
}

/// signed and unsigned division
static void div(op_pt result, op_pt left, op_pt right){
	load(A, left);
	load(C, right);
	
	if(left->sign || right->sign){
		put_cmd("\t%s\t%s\n",
			str_instruction(X_IDIV),
			str_reg(set_width(right->width), C)
		);
	}
	else{
		put_cmd("\t%s\t%s\n",
			str_instruction(X_DIV),
			str_reg(set_width(right->width), C)
		);
	}
	
	reg_d[A] = result;
}

/// dereference a pointer
static inline void dref(op_pt result, op_pt pointer){
	load(A, pointer);
	put_cmd("%s\t%s,\t[%s]\n",
		str_instruction(X_MOV),
		str_reg(set_width(result->width), A),
		str_reg(set_width(w_ptr),A)
	);
	
	reg_d[A] = result;
}

/// place a label in the assembler file
static inline void lbl(op_pt op){
	put_cmd("%s:\n", strings->get(op->label));
}

/// signed and unsigned modulus division
static void mod(op_pt result, op_pt left, op_pt right){
	load(A, left);
	load(C, right);
	
	if(left->sign || right->sign){
		put_cmd("\t%s\t%s\n",
			str_instruction(X_IDIV),
			str_reg(set_width(right->width), C)
		);
	}
	else{
		put_cmd("\t%s\t%s\n",
			str_instruction(X_DIV),
			str_reg(set_width(right->width), C)
		);
	}
	
	put_cmd("%s\t%s,\t%s\n",
		str_instruction(X_MOV),
		str_reg(set_width(result->width), A),
		str_reg(set_width(result->width), D)
	);
	
	reg_d[A] = result;
}

/// signed and unsigned multiplication
static void mul(op_pt result, op_pt left, op_pt right){
	load(A, left);
	load(C, right);
	
	if(left->sign || right->sign){
		put_cmd("\t%s\t%s\n",
			str_instruction(X_IMUL),
			str_reg(set_width(right->width), C)
		);
	}
	else{
		put_cmd("\t%s\t%s\n",
			str_instruction(X_MUL),
			str_reg(set_width(right->width), C)
		);
	}
	// sets the carry and overflow flags if D is non-zero
	
	reg_d[A] = result;
}

/// gets the address of a variable
static RETURN ref(op_pt result, op_pt arg){
	if(arg->type != st_static || arg->type != st_auto){
		msg_print(NULL, V_ERROR,
			"Internal dref(): arg is not a memory location");
		return failure;
	}
	
	if(result->width != w_ptr){
		msg_print(NULL, V_ERROR, "Internal dref(): result is not w_ptr");
		return failure;
	}
	
	put_cmd("\t%s\t%s\n%s\n",
		str_instruction(X_MOVZX),
		str_reg(set_width(w_ptr), A),
		str_oprand(arg)
	);
	
	reg_d[A] = result;
	
	return success;
}

/// return from a function
static inline void ret(op_pt value){
	// load the return value if present
	if(value) load(A, value);
	// pop the current activation record
	stack_manager.pop();
	// return
	put_cmd("\t%s\n", inst_array[X_RET]);
}

/// return the size, in bytes, of an operand
static inline void sz(op_pt result, op_pt arg){
	switch(set_width(arg->width)){
	case byte:
		put_cmd(
			"%s\t%s,\t%s\n",
			str_instruction(X_MOVZX),
			str_reg(set_width(w_word), A),
			str_num(1)
		);
		break;
	case word:
		put_cmd(
			"%s\t%s,\t%s\n",
			str_instruction(X_MOVZX),
			str_reg(set_width(w_word), A),
			str_num(2)
		);
		break;
	case dword:
		put_cmd(
			"%s\t%s,\t%s\n",
			str_instruction(X_MOVZX),
			str_reg(set_width(w_word), A),
			str_num(4)
		);
		break;
	case qword:
		put_cmd(
			"%s\t%s,\t%s\n",
			str_instruction(X_MOVZX),
			str_reg(set_width(w_word), A),
			str_num(8)
		);
		break;
	case bad_width:
	default:
		msg_print(NULL, V_ERROR, "sz(): broken");
	}
	
	reg_d[A] = result;
}

/// most unary operations
static inline void unary(op_pt result, op_pt arg, x86_inst op){
	// load the accumulator
	load(A, arg);
	
	put_cmd("\t%s\t%s\n",
		str_instruction(op),
		str_reg(set_width(arg->width), A)
	);
	
	reg_d[A] = result;
}

/***************************** ITERATOR FUNCTIONS *****************************/

/** Generate code for a single intermediate instruction
 */
static RETURN Gen_inst(inst_pt inst){
	return_t ret_val=success;
	
	switch (inst->op){
	case i_nop : return success;
	case i_proc: return success;
	
	case i_inc : unary(inst->result, inst->left, X_INC); break;
	case i_dec : unary(inst->result, inst->left, X_DEC); break;
	case i_neg : unary(inst->result, inst->left, X_NEG); break;
	case i_not : unary(inst->result, inst->left, X_NOT); break;
	
	case i_ass : ass (inst->result, inst->left); break;
	case i_sz  : sz  (inst->result, inst->left); break;
	case i_dref: dref(inst->result, inst->left); break;
	
	case i_ref : ret_val = ref(inst->result, inst->left); break;
	
	case i_lsh : binary(inst->result, inst->left, inst->right, X_SHL);
	case i_rsh : binary(inst->result, inst->left, inst->right, X_SHR);
	case i_rol : binary(inst->result, inst->left, inst->right, X_ROL);
	case i_ror : binary(inst->result, inst->left, inst->right, X_ROR);
	case i_add : binary(inst->result, inst->left, inst->right, X_ADD);
	case i_sub : binary(inst->result, inst->left, inst->right, X_SUB);
	case i_band: binary(inst->result, inst->left, inst->right, X_AND);
	case i_bor : binary(inst->result, inst->left, inst->right, X_OR);
	case i_xor : binary(inst->result, inst->left, inst->right, X_XOR);
	
	case i_mul : mul(inst->result, inst->left, inst->right); break;
	case i_div : div(inst->result, inst->left, inst->right); break;
	case i_mod : mod(inst->result, inst->left, inst->right); break;
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
	case i_cpy : break;
	
	case i_parm: stack_manager.push_parm(inst->left); break;
	case i_call: call(inst->result, inst->left); break;
	
	case i_rtrn: ret(inst->left); break;
	
	case i_NUM:
	default: msg_print(NULL, V_ERROR, "Gen_inst(): got a bad inst_code");
	}
	
	if(ret_val == failure) return failure;
	
	/* Since temporaries are single use they can be completely covered by the
	 * accumulator unless they are not immediately used. In which case we have
	 * to find a place to stash them.
	 */
	if(!inst->used_next){
		if(inst->result->type == st_temp){
			msg_print(NULL, V_INFO, "We're setting a stack temp");
			if(stack_manager.push_temp(A) == failure){
				msg_print(NULL, V_ERROR,
					"Gen_inst(): could not set a stack temp");
				return failure;
			}
		}
		else if(store(A) == failure){
			msg_print(NULL, V_ERROR, "Gen_inst(): could not store");
			return failure;
		}
	}
	
	return success;
}

/** Generate code for a single basic block
 */
static RETURN Gen_blk(blk_pt blk){
	inst_pt inst;
	
	msg_print(NULL, V_TRACE, "Gen_blk(): start");
	
	
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
	for(uint i=0; i<R8; i++){
		if(reg_d[i] != NULL)
			if( store((reg_t)i) == failure){
				msg_print(NULL, V_ERROR, "Internal Gen_blk(): store() failed");
				return failure;
			}
	}
	
	msg_print(NULL, V_TRACE, "Gen_blk(): stop");
	return success;
}

/** Creates and tears down the activation record of a procedure
 *
 *	parameters are added to the stack with the i_parm instruction. Formal
 *	parameters are read off the stack simply by their order:
 *	BP+machine_state+(total_parameters-parameter_number)
 *
 *	parameters must be contiguously packed at the top of the stack when i_call
 *	is made. this means we can't add new auto variables once a parameter has
 *	been pushed.
*/
static RETURN Gen_proc(proc_pt proc){
	blk_pt blk;
	op_pt  op;
	
	msg_print(NULL, V_TRACE, "Gen_proc(): start");
	
	// Initialize the register descriptor
	memset(reg_d, 0, sizeof(op_pt)*NUM_reg);
	
	// place the label
	lbl(proc->info);
	
	// set the base pointer
	stack_manager.set_BP();
	
	// make space for automatics
	if(( op=proc->info->first_auto() ))
		do{
			stack_manager.push_auto(op);
		}while(( op=proc->info->next_auto() ));
	
	if(!( blk=proc->first() )){
		msg_print(NULL, V_ERROR, "Gen_proc(): Empty Procedure");
		return failure;
	}
	do{
		if(Gen_blk(blk) == failure){
			msg_print(NULL, V_ERROR,
				"Internal Gen_proc(): Gen_blk() returned failure"
			);
			return failure;
		}
	}while(( blk=proc->next() ));
	
	// in case there was no explicit return. this will be dead code otherwise
	ret(NULL);
	
	msg_print(NULL, V_TRACE, "Gen_proc(): stop");
	return success;
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


void x86 (FILE * out_fd, PPD * prog, x86_mode_t proccessor_mode){
	proc_pt proc = NULL;
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
	
	instructions = &(prog->instructions);
	operands = &(prog->operands);
	strings  = &(prog->strings);
	fd   = out_fd;
	mode = proccessor_mode;
	
	if(!( proc=prog->instructions.first() )){
		msg_print(NULL, V_ERROR, "x86(): Empty Instruction queue");
		return;
	}
	
	fprintf(out_fd,"\nsection .text\t; Program code\n");
	
	do{
		if(Gen_proc(proc) == failure){
			msg_print(NULL, V_ERROR,
				"Internal x86(): Gen_blk() returned failure"
			);
			return;
		}
	} while(( proc=prog->instructions.next() ));
	
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
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


