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
 *	values assigned to SI and DI contain references to the object
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
 *	## Addressing
 *	### Effective Address
 *	Effective Address = reg + (Scale x reg) + immediate
 *	* Scale: A positive value of 1, 2, 4, or 8.
 *	* Displacement: An 8-bit, 16-bit, or 32-bit two’s-complement value encoded
 *	as part of the instruction.
 *
 *	### Logical Addresses
 *	Logical Address = Segment Selector : Effective Address
 *	A logical address  is a reference into a segmented-address space. It is
 *	comprised of the segment selector and the effective address.
 *
 *	### Linear Address
 *	Linear Address = Segment Base Address + Effective Address
 *	The segment selector is a pointer to an entry in a descriptor table. That
 *	entry contains the segment base address. In the flat memory model the
 *	segment base address is always 0 and linear addresses are equivalent to
 *	effective addresses
 *
 *	### Physical Address
 *	physical addresses are translated from linear addresses through paging.
 */


#include "x86.hpp"



static size_t param_sz;
static size_t frame_sz;


/******************************************************************************/
//                          REGISTER MANAGEMENT
/******************************************************************************/


/// These are all the x86 "general purpose" registers
typedef enum{
	A,   ///< Accumulator
	B,   ///< General Purpose
	C,   ///< Counter
	D,   ///< Data
	SI,  ///< Source Index
	DI,  ///< Destination Index
	BP,  ///< Base Pointer
	SP,  ///< Stack Pointer
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

class Reg_man{
	lbl_pt reg[NUM_reg];
	bool   ref[NUM_reg];
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Reg_man();
	
	/******************************* ACCESSOR *********************************/
	
//	reg_t find_ref(obj_pt);
//	reg_t find_val(obj_pt obj){
//		uint i;
//		
//		for(i=A; i!=NUM_reg; i++){
//			reg_t j = static_cast<reg_t>(i);
//			if(reg[j] == obj) break;
//		}
//		return (reg_t)i;
//	}
//	bool   is_ref(reg_t);
//	bool   is_clear(reg_t);
//	reg_t  check(void);
//	Data * get_obj(reg_t);
//	
//	/******************************* MUTATORS *********************************/
//	
//	void set_ref(reg_t r, sym_pt o){ reg[r] = o; ref[r] = true ; }
//	void set_val(reg_t r, sym_pt o){ reg[r] = o; ref[r] = false; }
	void clear(void){
		memset(reg, 0, sizeof(lbl_pt)*NUM_reg);
		memset(ref, 0, sizeof(bool)*NUM_reg);
	}
//	void clear(reg_t r){ reg[r] = NULL; }
//	void xchg(reg_t a, reg_t b);
	
};

/** Return the appropriate string to use the given x86 register. */
static const char * str_reg(size_t width, reg_t reg){
	static char array[4] = "   ";

	if(mode != xm_long && width == QWORD){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): qword only available in long mode");
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



/******************************************************************************/
//                            ?????????????????
/******************************************************************************/


//static inline void xchg(reg_t a, reg_t b){
//	size_t ex_sz;
//	
//	// what size
//	if(reg_d.get_obj(a)->get_size() > (ex_sz=reg_d.get_obj(b)->get_size()))
//		ex_sz=reg_d.get_obj(a)->get_size();
//	
//	// perform the exchange
//	put_str(FORM_4,
//		"xchg",
//		str_reg(ex_sz, a),
//		str_reg(ex_sz, b),
//		""
//	);
//	
//	// update the descriptor
//	reg_d.xchg(a,b);
//}


///******************************************************************************/
////                            RESOLVE DATA OBJECTS
///******************************************************************************/


//// Stack variables are accessed as [BP-frame_size+offset]
//static inline void ref_auto(std::string& s, obj_pt var){
//	s = "BP-";
//	s += str_num(frame_sz);
//	s += "+";
//	s += var->get_label();
//}

//// Formal parameters are accessed as [BP+2*stack_width+index*stack_width]
//static inline void ref_param(std::string& s, obj_pt var){
//	if(mode == xm_long){
//		s = "BP+16+";         // 2*stack_width
//		s +=var->get_label();
//		s += "*8";            // *stack_width
//	}
//	else{
//		s = "BP+8+";
//		s +=var->get_label();
//		s += "*4";
//	}
//}

//static inline void ref_static(std::string &s, obj_pt var){
//	s=var->get_label();
//}

//static inline void reftoval(std::string &s){
//	s.insert(0, "[");
//	s += "]";
//}

//static inline void op_size(std::string &s, Data * var){
//	switch(var->get_size()){
//	case BYTE : s.insert(0, "byte "); break;
//	case WORD : s.insert(0, "word "); break;
//	case DWORD: s.insert(0, "dword "); break;
//	case QWORD: s.insert(0, "qword "); break;
//	default:
//		msg_print(NULL, V_ERROR, "op_size(): bad");
//		throw;
//	}
//}

//#define BUFFER_CNT 3

////static void resolve_prime(Primative * obj){
////	static std::string buffers[BUFFER_CNT];
////	static uint        next;
////	reg_t reg;
////	
////	// pick a buffer
////	next++;
////	next%=BUFFER_CNT;
////	
////	// first check if it's already in a register
////	if(( reg=reg_d.find_val(obj) )){
////		buffers[next]= str_reg(obj->get_size(), reg);
////		return;
////	}
////	
////	// then check if there is a reference in a register
////	if(( reg=reg_d.find_ref(obj) )){
////		buffers[next]= str_reg(PTR, reg);
////		reftoval(buffers[next]);
////		return;
////	}
////	
////	// then generate its memory location
////	switch(obj->get_sclass()){
////	case sc_private:
////	case sc_public :
////	case sc_extern : ref_static(buffers[next], obj); break;
////	case sc_stack  : ref_auto  (buffers[next], obj); break;
////	case sc_param  : ref_param (buffers[next], obj); break;
////	
////	case sc_member:
////		msg_print(NULL, V_ERROR,
////			"Internal resolve_prime(): got a member object with no ref in any reg");
////		throw;
////	
////	//error
////	case sc_temp :
////	case sc_const:
////	case sc_none :
////	case sc_NUM  :
////	default      :
////		msg_print(NULL, V_ERROR,
////			"Internal resolve_prime(): got a non-memory object");
////		throw;
////	}
////	
////	reftoval(buffers[next]);
////	op_size (buffers[next], obj);
////	return;
////}

////static void resolve_addr(obj_pt obj){
////	static std::string buffers[BUFFER_CNT];
////	static uint        next;
////	
////	// pick a buffer
////	next++;
////	next%=BUFFER_CNT;
////	
////	// generate its memory location
////	switch(obj->get_sclass()){
////	case sc_member : // member labels are their offset within a struct
////	case sc_private:
////	case sc_public :
////	case sc_extern : ref_static(buffers[next], obj); break;
////	case sc_stack  : ref_auto  (buffers[next], obj); break;
////	case sc_param  : ref_param (buffers[next], obj); break;
////	
////	//error
////	case sc_temp :
////	case sc_const:
////	case sc_none :
////	case sc_NUM  :
////	default      :
////		msg_print(NULL, V_ERROR,
////			"Internal resolve_addr(): got a non-memory object");
////		throw;
////	}
////}

//static void find_ref(loc& location, Data * data){}

//static void find(loc& location, Data * data){}
////const char * str_r(loc l){}


///******************************************************************************/
////                       LOAD R-VALUES AND STORE TEMPS
///******************************************************************************/



//static void Stash(reg_t reg){
//	// already empty
//	if(reg_d.is_clear(reg)) return;
//	
//	// already in memory
//	if(reg_d.get_obj(reg)->get_sclass() != sc_temp) /* store */ ;
//	
	#define TREG_P 1 // B
	#define TREG_L 9 // B, R8-R15
//	
//	
//	if(reg == A);
//	
//	/*	DI -> SI
//		A,SI -> B -> R8-R15 -> C,D -> spill
//	
//	*/
//}

///**	load the r-value of an object into a register making that register
// *	available if necessary
// */
//static void Load_prime(reg_t reg, Primative * source){
//	
//	// check if it's already loaded
//	if(reg_d.get_obj(reg) == source && !reg_d.is_ref(reg)) return;
//	
//	// determine if source is already in a register and XCHG reg
////	if((ex_reg=reg_d.find_val(source)) != NUM_reg){
////		if(reg_d.get_obj(reg)->get_size() > source->get_size())
////			ex_sz = reg_d.get_obj(reg)->get_size();
////		else ex_sz = source->get_size();
////		
////		put_str(FORM_4,
////			"xchg",
////			str_reg(ex_sz, reg),
////			str_reg(ex_sz, ex_reg),
////			source->get_label()
////		);
////		return;
////	}
////	
////	Stash(reg);
////	
////	// Determine if a ref of source is already in a register dref
////	if((ex_reg=reg_d.find_ref(source)) != NUM_reg){
////		put_str(FORM_4,
////			"mov",
////			str_reg(source->get_size(), reg),
////			
////		);
////	}
//	
//	
//	// ELSE MOV resolve etc.
//}


///******************************************************************************/
////                       HELPERS FOR GETTING REFERENCES
///******************************************************************************/


//static inline void idx_array(Primative * si, Array * array, Primative * index){
//	size_t step_size;
//	reg_t  reg;
//	
//	// sanity check
//	if(!si || !array || !index){
//		msg_print(NULL, V_ERROR, "idx(): got a NULL");
//		throw;
//	}
//	
//	//if( (reg=check_reg(array)) == NUM_reg ) throw;
//	if( (reg=reg_d.find_ref(array)) == NUM_reg ) throw;
//	
//	step_size= array->get_child()->get_size();
//	Load_prime(A, index);
//	
//	if(step_size > QWORD){
//		put_str(FORM_3, "mul", str_num(step_size), "idx()");
//		// FIXME: mul cannot be used this way
//	
//		// A now contains the offset
//		put_str("\t%s\t%s, [%s+%s]\n",
//			"lea",
//			str_reg(si->get_size(), reg),
//			str_reg(si->get_size(), reg),
//			str_reg(PTR, A)
//		);
//	}
//	else{
//		put_str("\t%s\t%s, [%s+%s*%s]\n",
//			"lea",
//			str_reg(si->get_size(), reg), // target
//			str_reg(si->get_size(), reg), // base
//			str_reg(PTR, A), // index
//			str_num(step_size)
//		);
//	}
//	
//	// this is necessary if we want to later index the child
//	//reg_d[SI] = array->get_child();
//	reg_d.set_ref(reg, array->get_child());
//}

//static inline void idx_struct(Primative * si, Struct_inst * s, Data * member){
//	reg_t reg;
//	
//	// sanity check
//	if(!si || !s || !member){
//		msg_print(NULL, V_ERROR, "idx(): got a NULL");
//		throw;
//	}
//	
//	//if( (reg=check_reg(s)) == NUM_reg ) throw;
//	if( (reg=reg_d.find_ref(s)) == NUM_reg ) throw;
//	
//	put_str("\t%s\t%s, [%s+%s]\n",
//		"lea",
//		str_reg(si->get_size(), reg),
//		str_reg(si->get_size(), reg),
//		member->get_label()
//	);
//	
//	reg_d.set_ref(reg, member);
//}


///******************************************************************************/
////                             HELPERS ASSIGNMENTS
///******************************************************************************/


//static inline void mem_cpy(Data * dest, Data * source){
//	
//}

//static inline void prime_ass(Primative * dest, Primative * source){
//	if(dest->is_signed() != source->is_signed())
//		msg_print(NULL, V_WARN, "Mismatched signedness in assignment");
//}


static const char * str_sym(sym_pt sym){}
static void stash(reg_t reg){}






/******************************************************************************/
//                            INSTRUCTION MACROS
/******************************************************************************/


/* Because many x86 instructions require the use of specific registers each instruction macro must have a way to clear the registers it needs and have the previous value stored if it is still live.
*/



static inline void ass(sym_pt dest, sym_pt source){
	if(dest->get_size() < source->get_size())
		msg_print(NULL, V_WARN, "Assignment may cause overflow");
	if(dest->get_type() != source->get_type())
		msg_print(NULL, V_WARN, "Mismatched object type");
	
	if(dest->get_type() == ot_prime && source->get_type() == ot_prime)
		prime_ass(static_cast<Primative*>(dest), static_cast<Primative*>(source));
	else mem_cpy(dest, source);
}

static inline void binary_l(inst_code op, sym_pt dest, sym_pt source){
	resolve_prime(dest);
	resolve_prime(source);
	
	switch(op){
	case r_add: put_str(FORM_4,
		"add",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_sub: put_str(FORM_4,
		"sub",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_and: put_str(FORM_4,
		"and",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_or : put_str(FORM_4,
		"or",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_xor: put_str(FORM_4,
		"xor",
		"FIXME",
		"FIXME",
		""
	); break;
	}
}

static inline void
binary_r(inst_code op, sym_pt result, sym_pt dest, sym_pt source){
	Load_prime(A, dest);
	resolve_prime(source);
	
	switch(op){
	case r_add: put_str(FORM_4,
		"add",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_sub: put_str(FORM_4,
		"sub",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_and: put_str(FORM_4,
		"and",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_or : put_str(FORM_4,
		"or",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_xor: put_str(FORM_4,
		"xor",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	}
	
	reg_d.set_val(A, result);
}

static inline void call(sym_pt result, sym_pt proc){
	put_str(FORM_3, "call", proc->get_label(), "call()");
	reg_d.set_val(A, result);
}

static inline void dec(sym_pt arg){
	// TODO: these should probably Load_prime() for speed
	resolve_prime(arg);
	put_str(FORM_3, "dec", "FIXME", "");
}

static inline void
div(inst_code op, sym_pt result, sym_pt dest, sym_pt source){
	Load_prime(A, dest);
	Load_prime(D, source);
	
	if(dest->is_signed() || source->is_signed())
		put_str(FORM_3, "idiv", str_reg(source->get_size(), D), "");
	else put_str(FORM_3, "div", str_reg(source->get_size(), D), "");
	
	if(op == r_div) reg_d.set_val(A, result);
	else reg_d.set_val(D, result);
}

static inline void dref(sym_pt result, sym_pt pointer){}

static inline void inc(sym_pt arg){
	resolve_prime(arg);
	put_str(FORM_3, "inc", "FIXME", "");
}

static inline void inv_l(sym_pt arg){
	resolve_prime(arg);
	put_str(FORM_3, "not", "FIXME", "");
}

static inline void inv_r(sym_pt result, sym_pt arg){
	Load_prime(A, arg);
	put_str(FORM_3, "not", "FIXME", "");
	reg_d.set_val(A, result);
}



static inline void mul(sym_pt result, sym_pt dest, sym_pt source){
	Load_prime(A, dest);
	Load_prime(D, source);
	
	if(dest->is_signed() || source->is_signed())
		put_str(FORM_3, "imul", str_reg(source->get_size(), D), "");
	else put_str(FORM_3, "mul", str_reg(source->get_size(), D), "");
	
	// TODO: check for overflow
	
	reg_d.set_val(D, result);
}

static inline void neg_l(sym_pt arg){
	Load_prime(A, arg);
	put_str(FORM_3, "neg", "FIXME", "");
}

static inline void neg_r(sym_pt result, sym_pt arg){
	Load_prime(A, arg);
	put_str(FORM_3, "neg", "FIXME", "");
	reg_d.set_val(A, result);
}

static inline void ref(sym_pt si, sym_pt data, sym_pt index){
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
			"lea",
			str_reg(PTR, SI),
			"FIXME",
			"idx()"
		);
		
		reg_d.set_ref(SI, data);
	}
	
	if     (index && data->get_type() == ot_array)
		idx_array(si, static_cast<Array*>(data), dynamic_cast<Prime*>(index));
	else if(index && data->get_type() == ot_struct_inst)
		idx_struct(si, static_cast<Struct_inst*>(data), index);
}

static inline void ret(sym_pt val){
	Load_prime(A, val);
	put_str(FORM_2, "leave", "");
	put_str(FORM_3, "ret", str_num(param_sz), "");
}

static inline void shift_l(inst_code op, sym_pt dest, sym_pt count){
	
	if(count->get_sclass() != sc_const) Load_prime(C, count);
	resolve_prime(count);
	resolve_prime(dest);
	
	switch(op){
	case r_shl: put_str(FORM_4,
		"shl",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_shr: put_str(FORM_4,
		"shr",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_rol: put_str(FORM_4,
		"rol",
		"FIXME",
		"FIXME",
		""
	); break;
	case r_ror: put_str(FORM_4,
		"ror",
		"FIXME",
		"FIXME",
		""
	); break;
	}
}

static inline void
shift_r(inst_code op, sym_pt result, sym_pt dest, sym_pt count){
	
	if(result->get_size() != dest->get_size()) throw;
	
	if(count->get_sclass() != sc_const) Load_prime(C, count);
	resolve_prime(count);
	
	Load_prime(A, dest);
	
	switch(op){
	case r_shl: put_str(FORM_4,
		"shl",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_shr: put_str(FORM_4,
		"shr",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_rol: put_str(FORM_4,
		"rol",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	case r_ror: put_str(FORM_4,
		"ror",
		str_reg(dest->get_size(), A),
		"FIXME",
		""
	); break;
	}
	
	reg_d.set_val(A, result);
}

static inline void sz(sym_pt size, sym_pt object){
	// nothing to load so we just stash
	Stash(A);
	put_str(FORM_4,
		"movzx",
		str_reg(size->get_size(), A),
		str_num(object->get_size()),
		"sz()"
	);
}


/******************************************************************************/
//                        GENERATE ROUTINE INSTRUCTIONS
/******************************************************************************/


static void Gen_inst(inst_pt inst){
	switch (inst->op){
	
	//inc and dec act on a single symbol and have no other result
	case l_inc: inc(inst->dest); break;
	case l_dec: dec(inst->dest); break;
	
	case l_ass: ass(inst->dest, inst->source); break;
	
//	case r_ref: ref( // this function uses A and leaves a result in SI
//		inst->result, inst->dest, inst->source); break;
	
	// regular unary ops
	case r_neg: neg_r(
		inst->result,
		inst->dest
	); break;
	case r_not: inv_r(
		inst->result,
		inst->dest
	); break;
	case l_neg: neg_l(
		inst->dest
	); break;
	case l_not: inv_l(
		inst->dest
	); break;
	
	// shift ops
	case r_shl:
	case r_shr:
	case r_rol:
	case r_ror: shift_r(inst->op,
		inst->result,
		inst->dest,
		inst->source
	); break;
	case l_shl:
	case l_shr:
	case l_rol:
	case l_ror: shift_l(inst->op,
		inst->dest,
		inst->source
	); break;
	
	// regular binary ops
	case r_add:
	case r_sub:
	case r_and:
	case r_or :
	case r_xor: binary_r(inst->op,
		inst->result,
		inst->dest,
		inst->source
	); break;
	case l_add:
	case l_sub:
	case l_and:
	case l_or :
	case l_xor: binary_l(inst->op,
		inst->dest,
		inst->source
	); break;
	
	// multiplication
	case r_mul : mul(
		inst->result,
		inst->dest,
		inst->source
	); break;
	case r_div:
	case r_mod: div(inst->op,
		inst->result,
		inst->dest,
		inst->source
	); break;
	
	// flow control
	//case i_parm: break;
	case i_call: call(
		inst->result,
		inst->source
	); break;
	case i_ret: ret(inst->source); break;
	
	case i_lbl : put_lbl(inst->source); break;
	case i_jmp : break;
	case i_jz  : break;
	
	// compile-time constant
	case i_sz: sz(
		inst->result,
		inst->source
	); break;
	
	// errors
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
static void Gen_routine(lbl_pt lbl, Routine * routine){
	blk_pt blk;
	int    stack_temps;
	
	/************** SANITY CHECKS *************/
	
	if(!lbl || !routine){
		msg_print(NULL, V_ERROR, "Gen_routine(): Got a NULL pointer");
		throw;
	}
	
	/************ SETUP STACK TEMPS ************/
	
	stack_temps = routine->concurrent_temps;
	
	if(mode == xm_protected) stack_temps -= TREG_P;
	else stack_temps -= TREG_L;
	
	while(stack_temps > 0){
		//FIXME add stack temps
	}
	
	/******** TODO SORT AUTOS AND PARAMETERS ********/
	
	/************ CALCULATE SIZES ***************/
	
	set_struct_size(&routine->auto_storage);
	set_struct_size(&routine->formal_params);
	
	frame_sz=routine->auto_storage .get_size();
	param_sz=routine->formal_params.get_size();
	
	// pad them up to the stack width
	if(mode == xm_long){
		param_sz += QWORD-(param_sz & 0xf);
		frame_sz += QWORD-(frame_sz & 0xf);
	}
	else{
		param_sz += DWORD-(param_sz & 0x7);
		frame_sz += DWORD-(frame_sz & 0x7);
	}
	
	/**************** PROLOGUE ******************/
	
	/*	A static link, or access link, is a pointer to the stack frame of the most recent activation of the lexically enclosing function.
		A dynamic link is a pointer to the stack frame of the caller.
		The ENTER function automatically creates dynamic links but could easily become overwhelmed by large recursive functions.
		
		A `display` is an array of static links. the size of the display is a constant based on the lexical nesting of its definition.
	*/
	
	// Initialize the register descriptor
	reg_d.clear();
	
	// place the label
	put_str("\n\n");
	put_lbl(lbl);
	
	put_str(FORM_3, "enter", str_num(frame_sz), "");
	
	/**************** MAIN LOOP ****************/
	
	if(!( blk=routine->get_first_blk() )){
		msg_print(NULL, V_ERROR, "Gen_proc(): Empty Routine");
		throw;
	}
	
	do Gen_blk(blk); while(( blk=routine->get_next_blk() ));
	
	/***************** RETURN *****************/
	
	// this adds an extra return just in case. it will often be unreachable
	put_str(FORM_2, "leave", "");
	put_str(FORM_3, "ret", str_num(param_sz), "");
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


/**	This function creates assembler code by making multiple passes through the
 *	PPD object list.
 */
void x86 (FILE * out_fd, PPD * prog, x86_mode_t proccessor_mode){
	lbl_pt lbl;
	
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
	
	
	
	x86_declarations();
	
	/************** PROGRAM CODE **************/
	
	lbl=dynamic_cast<lbl_pt>(prog->symbols.first());
	
	put_str("\nsection .code\t; Program code\n");
	
	if (mode == xm_long) put_str("align 8\n");
	else put_str("align 4\n");
	
	do{
		if(lbl && lbl->get_def()->get_type() == st_routine)
			Gen_routine(
				lbl,
				dynamic_cast<Routine*>(lbl->get_def())
			);
	}while(( lbl=dynamic_cast<lbl_pt>(prog->symbols.next()) ));
	
	put_str("\n; End of MPL generated file\n\n");
	
	// TODO: if this is not stand-alone generate an Object Interface Description
	
	msg_print(NULL, V_INFO, "x86(): stop");
}


