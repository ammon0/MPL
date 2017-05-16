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
 *	In this generator we will try to make the left operand and the result
 *	always be the accumulator. This creates an opportunity for an optimizer that
 *	algebraically rearranges things.
 *
 *	## Register Allocation
 *	There is a problem if a temp variable is not immediately used. All temps are
 *	produced in the accumulator. If at any point in code generation we find that
 *	the contents of the accumulator is temp and not being used in the current
 *	instruction then it will have to be stored somewhere. Another register
 *	would be ideal.
 *
 *	values assigned to SI and DI are implicitly the reference to the object
 *	even if the object is prime. This is because the results of the ref
 *	operation stores in these locations and it will often be necessary to store
 *	values at these locations.
 *
 *	## Activation Record
 *
 *	<PRE>
 *	  Activation Record   Instructions
 *	|___________________|
 *	|    parameters     | PUSH / POP
 *	|___________________|
 *	|         IP        | CALL / RET
 *	|___________________|
 *	|         BP        | ENTER / LEAVE
 *	|___________________| <-BP
 *	|       Autos       |
 *	|_03_|_02_|_01_|_00_| <-SP
 *
 *	</PRE>
 *
 *	SP is always pointing to the top item on the stack, so
 *	PUSH=DEC,MOV POP=MOV,INC
 *
 *	BP is always pointing to the stored BP of the caller
 *
 *	Formal parameters are accessed as [BP+2*stack_width+index*stack_width]
 *
 *	Stack variables are accessed as [BP-frame_size+offset]
 *
 *	The ENTER instruction has a feature that allows up to 32 access links to be
 *	added the the current stack frame
 */


#include <util/msg.h>

#include <mpl/ppd.hpp>
#include <mpl/gen.hpp>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>

#include "x86_reg.hpp"


/******************************************************************************/
//                               DEFINITIONS
/******************************************************************************/


/// These are the x86 register sizes
#define BYTE  ((size_t) 1)
#define WORD  ((size_t) 2)
#define DWORD ((size_t) 4)
#define QWORD ((size_t) 8)

#define PTR (mode == xm_long? QWORD:DWORD)

/// These are the x86 instructions that we are working with
typedef enum x86_inst {
	X_MOV,      ///< copy data to a new location
	X_MOVSX,   ///< copy 8, or 16-bit data to a larger location and sign extend
	X_MOVSXD, ///< copy 32-bit data to a 64-bit register and sign extend
	X_MOVZX, ///< copy 8, or 16-bit data to a larger location and zero extend
	X_MOVBE, ///< copy and swap byte order
	X_LEA,  ///< load effective address multipliers are 2, 4, 8, 3, 5, and 9

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

typedef imax offset_t;
typedef umax index_t;


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
static size_t       frame_sz    ; ///< the size of the current stack frame

/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 *
 */
//static obj_pt reg_d[NUM_reg];

static Reg_man reg_d;


/******************************************************************************/
//                          STRING WRITING FUNCTIONS
/******************************************************************************/


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
static const char * str_reg(size_t width, reg_t reg){
	static char array[4] = "   ";

	if(mode != xm_long && width == QWORD){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): qword only availible in long mode");
		return "!!Bad width!!";
	}
	
	if(mode != xm_long && reg > SP){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): R8-R15 only available in long mode");
		return "!!Bad register!!";
	}
	
	switch (width){
	case BYTE : array[0] = ' '; array[2] = ' '; break;
	case WORD : array[0] = ' '; array[2] = 'x'; break;
	case DWORD: array[0] = 'e'; array[2] = 'x'; break;
	case QWORD: array[0] = 'r'; array[2] = 'x'; break;
	
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad width");
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

#define FORM_2   "\t%6s                                           ;%s\n"
#define FORM_3   "\t%6s %20s                      ;%s\n"
#define FORM_4   "\t%6s %20s, %20s;%s\n"
#define FORM_LBL "%s:\n"


/******************************************************************************/
//                        RESOLVE DATA OBJECTS
/******************************************************************************/


// Stack variables are accessed as [BP-frame_size+offset]
static inline void ref_auto(std::string& s, obj_pt var){
	s = "BP-";
	s += str_num(frame_sz);
	s += "+";
	s += var->get_label();
}

// Formal parameters are accessed as [BP+2*stack_width+index*stack_width]
static inline void ref_param(std::string& s, obj_pt var){
	if(mode == xm_long){
		s = "BP+16+";
		s +=var->get_label();
		s += "*8";
	}
	else{
		s = "BP+8+";
		s +=var->get_label();
		s += "*4";
	}
}

static inline void ref_static(std::string &s, obj_pt var){
	s=var->get_label();
}

static inline void reftoval(std::string &s){
	s.insert(0, "[");
	s += "]";
}

static inline void op_size(std::string &s, Data * var){
	switch(var->get_size()){
	case BYTE : s.insert(0, "byte "); break;
	case WORD : s.insert(0, "word "); break;
	case DWORD: s.insert(0, "dword "); break;
	case QWORD: s.insert(0, "qword "); break;
	default:
		msg_print(NULL, V_ERROR, "op_size(): bad");
		throw;
	}
}

#define BUFFER_CNT 3

static void resolve_prime(Prime * obj){
	static std::string buffers[BUFFER_CNT];
	static uint        next;
	reg_t reg;
	
	// pick a buffer
	next++;
	next%=BUFFER_CNT;
	
	// first check if it's already in a register
	if(( reg=reg_d.find_val(obj) )){
		buffers[next]= str_reg(obj->get_size(), reg);
		return;
	}
	
	// then check if there is a reference in a register
	if(( reg=reg_d.find_ref(obj) )){
		buffers[next]= str_reg(PTR, reg);
		reftoval(buffers[next]);
		return;
	}
	
	// then generate its memory location
	switch(obj->get_sclass()){
	case sc_private:
	case sc_public :
	case sc_extern : ref_static(buffers[next], obj); break;
	case sc_stack  : ref_auto  (buffers[next], obj); break;
	case sc_param  : ref_param (buffers[next], obj); break;
	
	case sc_member:
		msg_print(NULL, V_ERROR,
			"Internal resolve_prime(): got a member object with no ref in any reg");
		throw;
	
	//error
	case sc_temp :
	case sc_const:
	case sc_none :
	case sc_NUM  :
	default      :
		msg_print(NULL, V_ERROR,
			"Internal resolve_prime(): got a non-memory object");
		throw;
	}
	
	reftoval(buffers[next]);
	op_size (buffers[next], obj);
	return;
}

static void resolve_addr(obj_pt obj){
	static std::string buffers[BUFFER_CNT];
	static uint        next;
	
	// pick a buffer
	next++;
	next%=BUFFER_CNT;
	
	// generate its memory location
	switch(obj->get_sclass()){
	case sc_member : // member labels are their offset within a struct
	case sc_private:
	case sc_public :
	case sc_extern : ref_static(buffers[next], obj); break;
	case sc_stack  : ref_auto  (buffers[next], obj); break;
	case sc_param  : ref_param (buffers[next], obj); break;
	
	//error
	case sc_temp :
	case sc_const:
	case sc_none :
	case sc_NUM  :
	default      :
		msg_print(NULL, V_ERROR,
			"Internal resolve_addr(): got a non-memory object");
		throw;
	}
}


/******************************************************************************/
//                       LOAD R-VALUES AND STORE TEMPS
/******************************************************************************/


// assume D is used internal to an instruction no value is ever left there
// these functions c
static void Clear_A(void){
	// cascades to B, C, R8-R15, the to storage temps
}
static void Clear_DI(void){
	// cascades to SI then to the GPs

}

static void Stash(reg_t reg){
	// already empty
	if(reg_d.is_clear(reg)) return;
	
	// already in memory
	if(reg_d.get_obj(reg)->get_sclass() != sc_temp) return;
	
	
}

/**	load the r-value of an object into a register making that register
 *	available if necessary
 */
static void Load_prime(reg_t reg, Prime * source){
	
	// determine if source is already in a register and XCHG reg
	
	// ELSE Stash(reg)
	
	// Determine if a ref of source is already in a register dref
	
	// ELSE MOV resolve etc.
}


/******************************************************************************/
//                       HELPERS FOR GETTING REFERENCES
/******************************************************************************/


static inline void idx_array(Prime * si, Array * array, Prime * index){
	size_t step_size;
	reg_t  reg;
	
	// sanity check
	if(!si || !array || !index){
		msg_print(NULL, V_ERROR, "idx(): got a NULL");
		throw;
	}
	
	//if( (reg=check_reg(array)) == NUM_reg ) throw;
	if( (reg=reg_d.find_ref(array)) == NUM_reg ) throw;
	
	step_size= array->get_child()->get_size();
	Load_prime(A, index);
	
	if(step_size > QWORD){
		put_str(FORM_3, inst_array[X_MUL], str_num(step_size), "idx()");
	
		// A now contains the offset
		put_str("\t%s\t%s, [%s+%s]\n",
			inst_array[X_LEA],
			str_reg(si->get_size(), reg),
			str_reg(si->get_size(), reg),
			str_reg(PTR, A)
		);
	}
	else{
		put_str("\t%s\t%s, [%s+%s*%s]\n",
			inst_array[X_LEA],
			str_reg(si->get_size(), reg), // target
			str_reg(si->get_size(), reg), // base
			str_reg(PTR, A), // index
			str_num(step_size)
		);
	}
	
	// this is necessary if we want to later index the child
	//reg_d[SI] = array->get_child();
	reg_d.set_ref(reg, array->get_child());
}

static inline void idx_struct(Prime * si, Struct_inst * s, Data * member){
	reg_t reg;
	
	// sanity check
	if(!si || !s || !member){
		msg_print(NULL, V_ERROR, "idx(): got a NULL");
		throw;
	}
	
	//if( (reg=check_reg(s)) == NUM_reg ) throw;
	if( (reg=reg_d.find_ref(s)) == NUM_reg ) throw;
	
	put_str("\t%s\t%s, [%s+%s]\n",
		inst_array[X_LEA],
		str_reg(si->get_size(), reg),
		str_reg(si->get_size(), reg),
		member->get_label()
	);
	
	reg_d.set_ref(reg, member);
}


/******************************************************************************/
//                       ALPHABETICAL INSTRUCTION MACROS
/******************************************************************************/


// functions that write to memory

static inline void ass(Prime * dest, Prime * source){
	resolve_prime(dest);
	resolve_prime(source);
	
	if(dest->get_size() < source->get_size())
		msg_print(NULL, V_WARN, "Assignment may cause overflow");
	
	if(dest->is_signed() != source->is_signed())
		msg_print(NULL, V_WARN, "Mismatched signedness in assignment");
	
	// FIXME both operands cannot be in memory
	
	// TODO handle objects larger than prime
	
	// check if operands are refs to the object
	
	put_str(FORM_4,
		inst_array[X_MOV],
		"FIXME",
		"FIXME",
		"An assignment"
	);
}

static inline void dec(Prime* arg){
	resolve_prime(arg);
	
	put_str(FORM_3, inst_array[X_DEC], "FIXME", "");
}


static inline void inc(Prime * arg){
	resolve_prime(arg);
	
	put_str(FORM_3, inst_array[X_INC], "FIXME", "");
}






/// call a procedure
static inline void call(Prime * result, Routine * proc){
	// parameters are already loaded
	put_str(FORM_3, inst_array[X_CALL], proc->get_label(), "call()");

	// TODO: unload parameters

	//reg_d[A] = result;
	reg_d.set_val(A, result);
}



static inline void ref(Prime * si, Data * data, Data * index){
	// sanity check
	if(!si || !data){
		msg_print(NULL, V_ERROR, "ref(): got a NULL");
		throw;
	}
	
	// load the reference if it is not already there
	if( reg_d.find_ref(data) == NUM_reg){
		// get the address of the compound
		resolve_addr(data);
		put_str("\t%s\t%s, [%s] ;%s\n",
			inst_array[X_LEA],
			str_reg(PTR, SI),
			"FIXME",
			"idx()"
		);
		
		reg_d.set_ref(SI, data);
		//reg_d[SI] = data;
	}
	
	if     (index && data->get_type() == ot_array)
		idx_array(si, static_cast<Array*>(data), dynamic_cast<Prime*>(index));
	else if(index && data->get_type() == ot_struct_inst)
		idx_struct(si, static_cast<Struct_inst*>(data), index);
}




/// place a label in the assembler file
static inline void lbl(obj_pt op){
	put_str(FORM_LBL, op->get_label());
}


/******************************************************************************/
//                        GENERATE ROUTINE INSTRUCTIONS
/******************************************************************************/


/*	if(right){
 *		Load_prime(A, left);
 *		resolve_prime(right);
 *	}
 *	else resolve_prime(left);
 *	generate the instruction.
 *	change the reg_d of A
 */

/** Generate code for a single intermediate instruction
 *	The vast majority of x86 instructions have only a source and destination
 */
static void Gen_inst(inst_pt inst){
	
	
	
	// all instructions with two arguments will produce a temporary result, and some unaries.
	
	switch (inst->op){
	case i_nop : 
	case i_proc: return;
	
	//inc and dec act on a single object and have no other result
	case i_inc: inc(dynamic_cast<Prime*>(inst->left)); break;
	case i_dec: dec(dynamic_cast<Prime*>(inst->left)); break;
	
	case i_ass: ass(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Prime*>(inst->left)
	); break;
	
	// this function uses A and leaves a result in SI
	case i_ref: ref(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Data*> (inst->left),
		dynamic_cast<Data*> (inst->right)
	); break;
	
	//case i_dref: dref(inst->result, dynamic_cast<Prime*>(inst->left)); break;
	
	
	case i_call: call(
		dynamic_cast<Prime*>(inst->result),
		dynamic_cast<Routine*>(inst->left)
	); break;
	
	//	case i_sz  : break;
	
//	case i_ret: ret(inst->left); break;
	
//	case i_neg : unary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		X_NEG
//	); break;
//		case i_inv : break;
//	

//	
	
//	case i_lsh : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_SHL
//	); break;
//	case i_rsh : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_SHR
//	); break;
//	case i_rol : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_ROL
//	); break;
//	case i_ror : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_ROR
//	); break;
//	case i_add : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_ADD
//	); break;
//	case i_sub : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_SUB
//	); break;
//	case i_band: binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_AND
//	); break;
//	case i_bor : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_OR
//	) ; break;
//	case i_xor : binary(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right),
//		X_XOR
//	); break;
//	case i_mul : mul(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right)
//	); break;
//	case i_div : div(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right)
//	); break;
//	case i_mod : mod(
//		dynamic_cast<Prime*>(inst->result),
//		dynamic_cast<Prime*>(inst->left),
//		dynamic_cast<Prime*>(inst->right)
//	); break;
//	case i_exp : break;
	
//	// these are probably all implemented with X_CMP
//	case i_eq  : break;
//	case i_neq : break;
//	case i_lt  : break;
//	case i_gt  : break;
//	case i_lte : break;
//	case i_gte : break;
	
	
//	case i_and : break;
//	case i_or  : break;
//	case i_not : break;
	
//	case i_lbl : lbl(inst->left); break;
//	case i_jmp : break;
//	case i_jz  : break;
//	case i_loop: break;
//	case i_cpy : break;
	
//	case i_parm: stack_manager.push_parm(
//		dynamic_cast<Prime*>(inst->left)
//	); break;
	

	case i_NUM:
	default: msg_print(NULL, V_ERROR, "Gen_inst(): got a bad inst_code");
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

	msg_print(NULL, V_TRACE, "Gen_blk(): stop");
}

static void set_struct_size(Struct_def * structure);

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
	size_t auto_size, formal_size;
	
	/************** SANITY CHECKS *************/
	
	if(!routine){
		msg_print(NULL, V_ERROR, "Gen_routine(): Got a NULL pointer");
		throw;
	}
	
	/************ SETUP STACK TEMPS ************/
	
	//FIXME determine how many temps are needed and add them to the autos
	
	/******** TODO SORT AUTOS AND PARAMETERS ********/
	
	/************ CALCULATE SIZES ***************/
	
	set_struct_size(&routine->auto_storage);
	set_struct_size(&routine->formal_params);
	
	auto_size  =routine->auto_storage .get_size();
	formal_size=routine->formal_params.get_size();
	
	// pad them up to the stack width
	if(mode == xm_long){
		formal_size += QWORD-(formal_size & 0xf);
		auto_size   += QWORD-(auto_size   & 0xf);
	}
	else{
		formal_size += DWORD-(formal_size & 0x7);
		auto_size   += DWORD-(auto_size   & 0x7);
	}
	
	/**************** PROLOGUE ******************/
	
	/*	A static link, or access link, is a pointer to the stack frame of the most recent activation of the lexically enclosing function.
		A dynamic ling is a pointer to the stack frame of the caller.
		The ENTER function automatically creates dynamic links but could easily become overwhelmed by large recursive functions.
		
		A `display` is an array of static links. the size of the display is a constant based on the lexical nesting of its definition.
	*/
	
	// Initialize the register descriptor
	//memset(reg_d, 0, sizeof(obj_pt)*NUM_reg);
	reg_d.clear();
	
	// place the label
	lbl(routine);
	
	put_str("\t%s %s\n", inst_array[X_ENTER], str_num(auto_size));
	
	/**************** MAIN LOOP ****************/
	
	if(!( blk=routine->get_first_blk() )){
		msg_print(NULL, V_ERROR, "Gen_proc(): Empty Routine");
		throw;
	}
	
	do Gen_blk(blk); while(( blk=routine->get_next_blk() ));
	
	/***************** RETURN *****************/
	
	// pop the current activation record
	put_str("\t%s\n", inst_array[X_LEAVE]);
	// return
	put_str("\t%s\n", inst_array[X_RET]);
}


/******************************************************************************/
//                            DECLARE STATIC DATA
/******************************************************************************/


static void static_array(Array * array){
	
	// if the array is not initialized just do this
	if(array->value.empty()){
		put_str("\tresb\t%s\n", str_num(array->get_size()) );
		return;
	}
	
	// so it is initialized. sanity check
	if(array->value.size() > array->get_size()){
		msg_print(NULL, V_ERROR,
			"static_array(): initialization too large");
		throw;
	}
	
	put_str("\tdb\t");
	
	// declare an iterator
	std::vector<uint8_t>::iterator byte;
	
	byte=array->value.begin();
	if(*byte<0x80 && *byte>=0x20) put_str("'%c'", *byte);
	else put_str("%s", str_num(*byte));
	
	while(++byte < array->value.end()){
		if(*byte<0x80 && *byte>=0x20) put_str(",'%c'", *byte);
		else put_str(",%s", str_num(*byte));
	}
	
}

static void static_prime(Prime * prime){
	switch (prime->get_size()){
	case BYTE : put_str("\tdb\t%s\n", str_num(prime->get_value())); break;
	case WORD : put_str("\tdw\t%s\n", str_num(prime->get_value())); break;
	case DWORD: put_str("\tdd\t%s\n", str_num(prime->get_value())); break;
	case QWORD: put_str("\tdq\t%s\n", str_num(prime->get_value())); break;
	default:
		msg_print(NULL, V_ERROR,
			"static_prime(): got a bad size from size_of()");
		throw;
	}
}

static void static_structure(Struct_inst * structure){
	put_str("\tresb\t%s\n", str_num(structure->get_size()) );
}

/// Generate a static data object
static void static_data(Data * data){
	// dynamic_cast will pass a null if not data
	if(!data) return;
	
	// set padding if needed
	if(mode == xm_long && data->get_size() > QWORD){
		put_str("\talign %s\n", str_num(QWORD));
	}
	else if(mode == xm_protected && data->get_size() > DWORD){
		put_str("\talign %s\n", str_num(DWORD));
	}
	// if we are here the field cannot be bigger than the mode alignment
	else put_str("\talign %s\n", str_num(data->get_size()));
	
	// place the label
	lbl(data);

	switch(data->get_type()){
	// data
	case ot_prime : static_prime(dynamic_cast<Prime*>(data)); break;
	case ot_array : static_array(dynamic_cast<Array*>(data)); break;
	case ot_struct_inst:
		static_structure(dynamic_cast<Struct_inst*>(data));
		break;

	// not data
	default        :
		msg_print(NULL, V_ERROR, "Internal static_data(): not data");
		throw;
	}
}


/******************************************************************************/
//                          CALCULATE SIZES & OFFSETS
/******************************************************************************/


// forward declaration here
static void set_size(obj_pt);

/// calculate the size of a prime
static void set_prime_size(Prime * prime){
	switch(prime->get_width()){
	case w_byte : prime->set_size(BYTE); break;
	case w_byte2: prime->set_size(WORD); break;
	case w_byte4: prime->set_size(DWORD); break;
	
	case w_byte8:
		if(mode == xm_long) prime->set_size(QWORD);
		else{
			msg_print(NULL, V_ERROR,
				"set_prime_size(): cannot use qwords in protected mode");
			throw;
		}
		break;
	
	case w_word :
	case w_ptr  :
	case w_max  :
		if(mode == xm_long) prime->set_size(QWORD);
		else prime->set_size(DWORD);
		break;
	
	case w_none:
	case w_NUM :
	default    :
		msg_print(NULL, V_ERROR, "set_prime_size(): received invalid width");
		throw;
	}
}

/// calculate the size of an array
static void set_array_size(Array * array){
	if(!array->get_child()){
		msg_print(NULL, V_ERROR,
			"set_array_size(): found array %s with no child",
			array->get_label()
		);
		throw;
	}
	
	// recursively set member sizes
	if(!array->get_size()) set_size(array->get_child());
}

/** calculate the size of structure and its member offsets
 *	This function also prints a structure declaration to the out_fd
 */
static void set_struct_size(Struct_def * structure){
	size_t bytes=0, pad;
	Data * field;
	
	// make sure all field sizes are available first
	field = structure->members.first();
	do{
		if(!field->get_size()){
			if(field->get_type() == ot_struct_inst)
				set_struct_size(
					dynamic_cast<Struct_inst*>(field)->get_layout()
				);
			else set_size(field);
		}
	}while(( field = structure->members.next() ));
	
	
	// calculate offsets and padding
	put_str("\nstruc %s\n", structure->get_label());
	
	// get the first field
	field = structure->members.first();
	
	// print the first field
	put_str("\t%s\t: resb %s\n",
		field->get_label(),
		str_num(field->get_size())
	);
	
	// store any necessary alignment info
	bytes += field->get_size();
	
	// get the next field
	while(( field = structure->members.next() )){
		// decide whether any padding is needed
		if(mode == xm_long && field->get_size() > QWORD){
			msg_print(NULL, V_WARN, "Padding before field %s in structure %s",
				field->get_label(),
				structure->get_label()
			);
			put_str("\talign %s\n", str_num(QWORD));
		}
		else if(mode == xm_protected && field->get_size() > DWORD){
			msg_print(NULL, V_WARN, "Padding before field %s in structure %s",
				field->get_label(),
				structure->get_label()
			);
			put_str("\talign %s\n", str_num(DWORD));
		}
		// if we are here the field cannot be bigger than the mode alignment
		else if(( pad=bytes%field->get_size() )){
			msg_print(NULL, V_WARN, "Padding before field %s in structure %s",
				field->get_label(),
				structure->get_label()
			);
			put_str("\talign %s\n", str_num(field->get_size()));
		}
		
		// print the field
		put_str("\t%s\t: resb %s\n",
			field->get_label(),
			str_num(field->get_size())
		);
		// store any necessary alignment info
		bytes += field->get_size();
	}
	
	put_str("endstrc\n");
	
	structure->set_size(bytes);
	
	put_str(
		"%%if (%s != %s_size)\n\
		\t %%error \"Internal struct size mismatch\"\n\
		%%endif\n\n",
		str_num(structure->get_size()),
		structure->get_label()
	
	);
}

static void set_size(obj_pt obj){
	switch( obj->get_type() ){
	case ot_prime: set_prime_size( dynamic_cast<Prime*>(obj) ); break;
	case ot_array: set_array_size( dynamic_cast<Array*>(obj) ); break;
	case ot_struct_def:
		set_struct_size( dynamic_cast<Struct_def*>(obj) );
		break;
	
	// ignore
	case ot_routine    :
	case ot_struct_inst: break;
	
	// error
	default     :
		msg_print(NULL, V_ERROR, "set_size(): found an invalid obj_t");
		throw;
	}
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


/**	This function creates assembler code by making multiple passes through the
 *	PPD object list.
 */
void x86 (FILE * out_fd, PPD * prog, x86_mode_t proccessor_mode){
	obj_pt obj;
	
	msg_print(NULL, V_INFO, "x86(): start");
	
	if(proccessor_mode != xm_long && proccessor_mode != xm_protected){
		msg_print(NULL, V_ERROR, "x86: Invalid processor mode");
		throw;
	}
	
	if(!out_fd){
		msg_print(NULL, V_ERROR, "x86(): out_fd is NULL");
		throw;
	}
	
	program_data = prog;
	fd           = out_fd;
	mode         = proccessor_mode;
	
	put_str("\n; A NASM assembler file generated from MPL\n");
	// this should validate out_fd
	
	if(!( obj=prog->objects.first() )){
		msg_print(NULL, V_ERROR, "x86(): Program contains no objects");
		return;
	}
	
	/***** SET SIZES AND STRUCTURE OFFSETS *****/
	
	put_str("\n; Declaring record offsets\n");
	
	do{
		set_size(obj);
	}while(( obj=prog->objects.next() ));
	
	/*********** DECLARE VISIBILITY ***********/
	
	put_str("\n; Declaring Visibility\n");
	
	obj=prog->objects.first();
	
	do{
		switch(obj->get_sclass()){
		case sc_public: put_str("global %s\n", obj->get_label()); break;
		case sc_extern: put_str("extern %s\n", obj->get_label()); break;
		
		// Do nothing
		case sc_private:
		case sc_stack  :
		case sc_param  :
		case sc_member :
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
	
	put_str("\nsection .data\t; Declaring static data\n");
	
	if (mode == xm_long) put_str("align 8");
	else put_str("align 4");
	put_str(" ; this probably does nothing\n");
	
	do{
		switch(obj->get_sclass()){
		case sc_private:
		case sc_public : static_data(dynamic_cast<Data*>(obj));
		
		// ignore
		case sc_stack :
		case sc_param :
		case sc_member:
		case sc_temp  :
		case sc_extern:
		case sc_const : break;
		
		//error
		case sc_none:
		case sc_NUM:
		default: throw;
		}
	}while(( obj=prog->objects.next() ));
	
	/************** PROGRAM CODE **************/
	
	obj=prog->objects.first();
	
	put_str("\nsection .code\t; Program code\n");
	
	if (mode == xm_long) put_str("align 8\n");
	else put_str("align 4\n");
	
	do{
		if(obj->get_type() == ot_routine)
			Gen_routine(dynamic_cast<Routine*>(obj));
	}while(( obj=prog->objects.next() ));
	
	put_str("\n; End of MPL generated file\n")
	
	// TODO: if this is not stand-alone generate an Object Interface Description
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


