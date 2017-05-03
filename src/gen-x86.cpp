/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the license terms with this software. If
 *	not, please visit the project home page at:
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
#include <mpl/prime.hpp>
#include <mpl/routine.hpp>
#include <mpl/structure.hpp>
#include <mpl/array.hpp>

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
	int SP_offset;
public:
	Stack_man(void){ SP_offset = 0; }
	
	void push_temp(reg_t reg     ); ///< push as a temp
	void push_auto(Data * auto_var); ///< push an automatic variable
	void push_parm(obj_pt parameter); ///< push a parameter onto the stack
	void set_BP(void     ); ///< advance BP to the SP
	void unload(uint count); ///< unload parameters from the stack
	void pop(void); ///< pop the current activation record
};

/// Keeps track of what is in each register
class Reg_man{
	obj_pt reg_d[NUM_reg];
public:
	void flush(void);
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


static PPD        * program_data;
static FILE       * fd          ; ///< the output file descriptor
static x86_mode_t   mode        ; ///< the processor mode we are building for

/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 */
static obj_pt reg_d[NUM_reg];
static Stack_man stack_manager;


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// runs the various functions through their various outputs.
static void test_x86(void){}


/************************** STRING WRITING FUNCTIONS **************************/

/** Return the correct assembler string for the given x86 instruction. */
static inline const char * str_instruction(x86_inst instruction){
	return inst_array[instruction];
}

/** Return a string representation of a number. */
static inline const char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
	return array;
}

/** Return the appropriate string to use the given x86 register. */
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

/** Add a command string to the output file. */
static void __attribute__((format(printf, 1, 2)))
put_str(const char * format, ...){
	va_list ap;
	
	va_start(ap, format);
	vfprintf(fd, format, ap);
	va_end(ap);
}

/***************************** HELPER FUNCTIONS *******************************/

/// Get the size of an object in bytes
static size_t size_of(Data * data){
	size_t size = 0;
	
	switch(data->get_type()){
	case ot_array: break; //FIXME: do something
	case ot_prime: break;
	case ot_struct: break;
	
	case ot_base   :
	case ot_routine:
	default        :
		msg_print(NULL, V_ERROR, "size_of(): data is not Data*");
		throw;
	}
	
	return size;
}

/** Determine whether an operand is already present in a register.
 */
static reg_t check_reg(obj_pt operand){
	uint i;
	
	for(i=A; i!=NUM_reg; i++){
		reg_t j = static_cast<reg_t>(i);
		if(reg_d[j] == operand) break;
	}
	return (reg_t)i;
}

/// push an auto variable onto the stack
void Stack_man::push_auto(Data * auto_var){
	//TODO: calculate its size
	
	// issue push instruction
	put_str("%s\t%s\n",
		str_instruction(X_PUSH),
		str_num(0)
	);
	
	// increment SP_offset
	if(mode == xm_long) SP_offset += qword;
	else SP_offset += dword;
	
	// set the offset
	auto_var->set_offset(SP_offset);
}

/// push a temp variable onto the stack
void Stack_man::push_temp(reg_t reg){
	Data * tmp;
	
	if(reg_d[reg]->get_sclass() != sc_temp){
		msg_print(NULL, V_ERROR, "Internal push_temp(): Not a temp");
		throw;
	}
	
	tmp = dynamic_cast<Data*>(reg_d[reg]);
	
	// issue push instruction
//	put_str("%s\t%s\n",
//		str_instruction(X_PUSH),
//		str_reg(set_width(tmp), reg)
//		// or should this be the stack width?
//	);
	
	// increment SP_offset
	if(mode == xm_long) SP_offset += qword;
	else SP_offset += dword;
	
	tmp->set_offset(SP_offset);
}

/// advance BP to the next activation record
void Stack_man::set_BP(void){
	put_str("\t%s\t%s,\t%s\n",
		str_instruction(X_MOV),
		str_reg(mode == xm_long? qword: dword, BP),
		str_reg(mode == xm_long? qword: dword, SP)
	);
	SP_offset = 0;
}

void Stack_man::pop(void){
	put_str(
		"\t%s\t%s,\t%s\n",
		inst_array[X_MOV],
		str_reg(mode == xm_long? qword:dword, SP),
		str_reg(mode == xm_long? qword:dword, BP)
	);
	SP_offset=0;
}

/** Store data in register to its appropriate memory location.
 */
static void store(reg_t reg){
	Prime * prime;
	
	if(!reg_d[reg]->is_mem()) return;
	
	if(reg_d[reg] == NULL){
		msg_print(NULL, V_ERROR,
			"store(): there is no memory location associated with register %u",
			reg
		);
		throw;
	}
	
	prime = dynamic_cast<Prime*>(reg_d[reg]);
	
	put_str("\t%s\t[%s]\t%s",
		str_instruction(X_MOV),
		str_prime(prime),
		str_reg(set_width(prime), reg)
	);
}

/** Load data from memory to a register.
 */
static void load(reg_t reg, Prime * mem){
	reg_t test_reg;
	
	// check if it's already in a register
	test_reg = check_reg(mem);
	
	if(test_reg == reg) // if it's already where it needs to be
		;
	else if(test_reg != NUM_reg){ // if it's in another register
		put_str("%s\t%s,\t%s\n",
			str_instruction(X_MOV),
			str_reg(set_width(mem), reg),
			str_reg(set_width(mem), test_reg)
		);
		reg_d[test_reg] = NULL;
	}
	else // if it's in memory
		put_str("\t%s\t%s,\t[%s]\n",
			str_instruction(X_MOV),
			str_reg(set_width(mem), reg),
			str_prime(mem)
		);
	
	reg_d[reg] = mem;
}

static void Store(reg_t reg){}

static void Load(reg_t reg, Data * mem){}
static void Load_index(reg_t, Array * array, Prime * idx){}
static void Load_member(reg_t, Structure * array /*, FIXME */ ){}

/*************************** INSTRUCTION FUNCTIONS ****************************/
// Alphabetical

/// make an assignment
static inline void ass(obj_pt result, obj_pt arg){
//	load(A, arg);
//	reg_d[A] = result;
}

/// most binary operations
static inline void
binary(Prime * result, Prime * left, Prime * right, x86_inst op){
	load(A, left);
	load(C, right);
	
	put_str("\t%s\t%s,\t%s\n",
		str_instruction(op),
		str_reg(set_width(left), A),
		str_reg(set_width(right), C)
	);
	
	reg_d[A] = result;
}

/// call a procedure
static inline void call(obj_pt result, Routine * proc){
	// parameters are already loaded
	put_str("\t%s\t%s\n", inst_array[X_CALL], proc->get_label());
	
	// unload parameters
	stack_manager.unload(proc->formal_params.count());
	
	reg_d[A] = result;
}

/// signed and unsigned division
static void div(Prime * result, Prime * left, Prime * right){
	load(A, left);
	load(C, right);
	
	if(left->is_signed() || right->is_signed()){
		put_str("\t%s\t%s\n",
			str_instruction(X_IDIV),
			str_reg(set_width(right), C)
		);
	}
	else{
		put_str("\t%s\t%s\n",
			str_instruction(X_DIV),
			str_reg(set_width(right), C)
		);
	}
	
	reg_d[A] = result;
}

/// dereference a pointer
static inline void dref(obj_pt result, Prime * pointer){
	load(A, pointer);
//	put_str("%s\t%s,\t[%s]\n",
//		str_instruction(X_MOV),
//		str_reg(set_width(result), A),
//		str_reg(set_width(w_ptr),A)
//	);
	
	
	//FIXME
	reg_d[A] = result;
}

/// place a label in the assembler file
static inline void lbl(obj_pt op){
	put_str("%s:\n", op->get_label());
}

/// signed and unsigned modulus division
static void mod(Prime * result, Prime * left, Prime * right){
	load(A, left);
	load(C, right);
	
	if(left->is_signed() || right->is_signed()){
		put_str("\t%s\t%s\n",
			str_instruction(X_IDIV),
			str_reg(set_width(right), C)
		);
	}
	else{
		put_str("\t%s\t%s\n",
			str_instruction(X_DIV),
			str_reg(set_width(right), C)
		);
	}
	
	put_str("%s\t%s,\t%s\n",
		str_instruction(X_MOV),
		str_reg(set_width(result), A),
		str_reg(set_width(result), D)
	);
	
	reg_d[A] = result;
}

/// signed and unsigned multiplication
static void mul(Prime * result, Prime * left, Prime * right){
	load(A, left);
	load(C, right);
	
	if(left->is_signed() || right->is_signed()){
		put_str("\t%s\t%s\n",
			str_instruction(X_IMUL),
			str_reg(set_width(right), C)
		);
	}
	else{
		put_str("\t%s\t%s\n",
			str_instruction(X_MUL),
			str_reg(set_width(right), C)
		);
	}
	// sets the carry and overflow flags if D is non-zero
	
	reg_d[A] = result;
}

/// gets the address of an object
static void ref(Prime * result, obj_pt arg){
	
	switch(arg->get_sclass()){
	case sc_private:
	case sc_public:
	case sc_extern:
	case sc_stack :
	case sc_param: break;
	
	case sc_none :
	case sc_temp :
	case sc_const:
	case sc_NUM  :
	default      :
		msg_print(NULL, V_ERROR,
			"Internal dref(): arg is not a memory location");
		throw;
	}
	
	if(result->get_width() != w_ptr){
		msg_print(NULL, V_ERROR, "Internal dref(): result is not w_ptr");
		throw;
	}
	
	// FIXME
//	put_str("\t%s\t%s\n%s\n",
//		str_instruction(X_MOVZX),
//		str_reg(set_width(w_ptr), A),
//		str_obj(arg)
//	);
	
	reg_d[A] = result;
}

/// return from a function
static inline void ret(obj_pt value){
	// load the return value if present
	//if(value) load(A, value);
	// TODO: jump to the return statement
}

/// most unary operations
static inline void unary(Prime * result, Prime * arg, x86_inst op){
	// load the accumulator
	load(A, arg);
	
	put_str("\t%s\t%s\n",
		str_instruction(op),
		str_reg(set_width(arg->get_width()), A)
	);
	
	//FIXME: reg_d[A] = result;
}

/***************************** ITERATOR FUNCTIONS *****************************/

/** Generate code for a single intermediate instruction
 */
static void Gen_inst(inst_pt inst){
	switch (inst->op){
	case i_nop : return;
	case i_proc: return;
	
	case i_inc : unary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		X_INC
	); break;
	case i_dec : unary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		X_DEC
	); break;
	case i_neg : unary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		X_NEG
	); break;
	case i_not : unary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		X_NOT
	); break;
	
	case i_ass : ass (inst->result, inst->left); break;
	case i_sz  : break;
	case i_dref: dref(inst->result, dynamic_cast<Prime*>(inst->left)); break;
	
	case i_ref : ref(dynamic_cast<Prime*>(inst->result), inst->left); break;
	
	case i_lsh : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_SHL
	); break;
	case i_rsh : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_SHR
	); break;
	case i_rol : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_ROL
	); break;
	case i_ror : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_ROR
	); break;
	case i_add : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_ADD
	); break;
	case i_sub : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_SUB
	); break;
	case i_band: binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_AND
	); break;
	case i_bor : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_OR
	) ; break;
	case i_xor : binary(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right),
		X_XOR
	); break;
	case i_mul : mul(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right)
	); break;
	case i_div : div(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right)
	); break;
	case i_mod : mod(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left),
		dynamic_cast<Prime*>(inst->right)
	); break;
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
	
	case i_parm: stack_manager.push_parm(
		dynamic_cast<Prime*>(inst->left)
	); break;
	case i_call: call(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Routine*>(inst->left)
	); break;
	
	case i_rtrn: ret(inst->left); break;
	
	case i_NUM:
	default: msg_print(NULL, V_ERROR, "Gen_inst(): got a bad inst_code");
	}
	
	/* Since temporaries are single use they can be completely covered by the
	 * accumulator unless they are not immediately used. In which case we have
	 * to find a place to stash them.
	 */
	if(!inst->used_next){
		if(inst->result->get_sclass() == sc_temp){
			msg_print(NULL, V_INFO, "We're setting a stack temp");
			stack_manager.push_temp(A);
		}
		else store(A);
	}
}

/** Generate code for a single basic block
 */
static void Gen_blk(blk_pt blk){
	inst_pt inst;
	
	msg_print(NULL, V_TRACE, "Gen_blk(): start");
	
	if(!( inst=blk->first() )){
		msg_print(NULL, V_ERROR, "Gen_blk(): received empty block");
		throw;
	}
	
	do Gen_inst(inst);
	while(( inst=blk->next() ));
	
	// flush the registers at the end of the block
	for(uint i=0; i<R8; i++){
		if(reg_d[i] != NULL) store((reg_t)i);
	}
	
	msg_print(NULL, V_TRACE, "Gen_blk(): stop");
}


/** Creates and tears down the activation record of a procedure
 *
 *	parameters are added to the stack in reverse order with the i_parm
 *	instruction. Formal parameters are read off the stack simply by their
 *	position: BP+machine_state+parameter_number
 *
 *	parameters must be contiguously packed at the top of the stack when i_call
 *	is made. this means we can't add new auto variables once a parameter has
 *	been pushed.
*/
static void Gen_routine(Routine * routine){
	blk_pt blk;
	obj_pt auto_var;
	
	/************** SANITY CHECKS *************/
	
	if(!routine){
		msg_print(NULL, V_ERROR, "Gen_routine(): Got a NULL pointer");
		throw;
	}
	
	/***************** SETUP ******************/
	
	// Initialize the register descriptor
	memset(reg_d, 0, sizeof(obj_pt)*NUM_reg);
	
	// place the label
	lbl(routine);
	
	// set the base pointer
	stack_manager.set_BP();
	
	// make space for automatics
	if(( auto_var=routine->autos.first() ))
		do{
			stack_manager.push_auto(dynamic_cast<Data*>(auto_var));
		}while(( auto_var=routine->autos.next() ));
	
	/************ SETUP STACK TEMPS ************/
	
	//FIXME
	
	/**************** MAIN LOOP ****************/
	
	if(!( blk=routine->get_first_blk() )){
		msg_print(NULL, V_ERROR, "Gen_proc(): Empty Routine");
		throw;
	}
	
	do Gen_blk(blk);
	while(( blk=routine->get_next_blk() ));
	
	/***************** RETURN *****************/
	
	// pop the current activation record
	stack_manager.pop();
	// return
	put_str("\t%s\n", inst_array[X_RET]);
}

/*************************** DECLARATION FUNCTIONS ****************************/

static void static_array(Array * array){
	put_str("\tresb\t%s\n", str_num(size_of(array)));
}

static void static_prime(Prime * prime){
	switch (size_of(prime)){
	case byte : put_str("\tdb\t%s\n", str_num(prime->get_value())); break;
	case word : put_str("\tdw\t%s\n", str_num(prime->get_value())); break;
	case dword: put_str("\tdd\t%s\n", str_num(prime->get_value())); break;
	case qword: put_str("\tdq\t%s\n", str_num(prime->get_value())); break;
	default:
		msg_print(NULL, V_ERROR,
			"static_prime(): got a bad size from size_of()");
		throw;
	}
}

static void static_structure(Structure * structure){
	Data * field;
	
	if(( field = structure->members.first() )){
		do{
			switch(field->get_type()){
			case ot_array:
				static_array(dynamic_cast<Array*>(field)); break;
			case ot_prime:
				static_prime(dynamic_cast<Prime*>(field)); break;
			case ot_struct:
				static_structure(dynamic_cast<Structure*>(field)); break;
			case ot_base:
			case ot_routine:
			default:
				msg_print(NULL, V_ERROR,
					"static_structure(): got an invalid field");
				throw;
			}
		}while(( field = structure->members.next() ));
	}
}

/// Generate a static data object
static void static_data(obj_pt obj){
	
	// place the label
	lbl(obj);
	
	switch(obj->get_type()){
	// data
	case ot_prime : static_prime    (dynamic_cast<Prime*>    (obj)); break;
	case ot_struct: static_structure(dynamic_cast<Structure*>(obj)); break;
	case ot_array : static_array    (dynamic_cast<Array*>    (obj)); break;
	
	// not data
	case ot_base   :
	case ot_routine:
	default        :
		msg_print(NULL, V_ERROR, "static_data(): not data");
		throw;
	}
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


void x86 (FILE * out_fd, PPD * prog, x86_mode_t proccessor_mode){
	obj_pt obj;
	
	msg_print(NULL, V_INFO, "x86(): start");
	
	if(proccessor_mode != xm_long && proccessor_mode != xm_protected){
		msg_print(NULL, V_ERROR, "x86: Invalid mode");
		throw;
	}
	
	program_data = prog;
	fd           = out_fd;
	mode         = proccessor_mode;
	
	if(!( obj=prog->objects.first() )){
		msg_print(NULL, V_ERROR, "x86(): Program contains no objects");
		return;
	}
	
	/*********** DECLARE VISIBILITY ***********/
	
	do{
		switch(obj->get_sclass()){
		case sc_public: put_str("global %s\n", obj->get_label()); break; 
		case sc_extern: put_str("extern %s\n", obj->get_label()); break;
		
		// Do nothing
		case sc_private:
		case sc_stack  :
		case sc_param  :
		case sc_temp   :
		case sc_const  : break;
		
		// error
		case sc_none:
		case sc_NUM :
		default     :
			msg_print(NULL, V_ERROR,
				"x86(): got an object with an invalid storage class");
			throw;
		}
	}while(( obj=prog->objects.next() ));
	
	/************ STATIC VARIABLES ************/
	
	obj=prog->objects.first();
	
	put_str("\nsection .data\t; Static data\n");
	
	if (mode == xm_long) put_str("align 8\n");
	else put_str("align 4\n");
	
	do{
		if(obj->is_static_data()) static_data(obj);
	}while(( obj=prog->objects.next() ));
	
	/************** PROGRAM CODE **************/
	
	obj=prog->objects.first();
	
	put_str("\nsection .code\t; Program code\n");
	
	do{
		if(obj->get_type() == ot_routine)
			Gen_routine(dynamic_cast<Routine*>(obj));
	}while(( obj=prog->objects.next() ));
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


