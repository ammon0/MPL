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
 *	always be the accumulator. This creates a need for an optimizer that
 *	algebraically rearranges things.
 *
 *	## Register Allocation
 *	There is a problem if a temp variable is not immediately used. All temps are
 *	produced in the accumulator. If at any point in code generation we find that
 *	the contents of the accumulator is temp and not being used in the current
 *	instruction then it will have to be stored somewhere. Another register
 *	would be ideal.
 *
 *	## Parameters
 *	When the actual parameter structure is allocated its offset from BP is
 *	noted. Registers can still spill beyond it as long as they are removed
 *	before the call.
 *
 *	## Activation Record
 *
 *	<PRE>
 *	  Activation Record   Instructions
 *	|___________________|
 *	| Formal parameters |
 *	|___________________|
 *	|         IP        | CALL / RET
 *	|___________________|
 *	|         BP        | ENTER / LEAVE
 *	|___________________| <-BP
 *	|       Autos       |
 *	|___________________|
 *	| Spilled Registers |
 *	|___________________|
 *	| Actual Parameters |
 *	|_03_|_02_|_01_|_00_| <-SP
 *	</PRE>
 *
 *	SP is always pointing to the top item on the stack, so
 *	PUSH=DEC,MOV POP=MOV,INC
 *
 *	BP is always pointing to the stored BP of the caller
 *
 *	Formal parameters are accessed as [BP+2*stack_width+label]
 *
 *	Stack variables are accessed as [BP-frame_size+label]
 *
 *	Spilled registers are accessed as [BP-frame_size-stack_width*offset]
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
 *	In the flat memory model the segment base address is always 0 and linear
 *	addresses are equivalent to effective addresses.
 *
 *	### Physical Address
 *	physical addresses are translated from linear addresses through paging.
 */


#include "x86.hpp"




/******************************************************************************/
//                          REGISTER MANAGEMENT
/******************************************************************************/




static Reg_man reg_d;




/******************************************************************************/
//                            RESOLVE DATA OBJECTS
/******************************************************************************/


static inline void reftoval(std::string &s){
	s.insert(0, "[");
	s += "]";
}

static inline void op_size(std::string &s, lbl_pt var){
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

static void Resolve(std::string &s, lbl_pt lbl){
	switch(lbl->get_mode()){
	case am_static_priv  :
	case am_static_pub   :
	case am_static_extern:
	case am_constant     : // constants are declared using EQU
		s = lbl->get_name();
		break;
	
	case am_stack_aparam:
		break;
	
	case am_stack_fparam:
		break;
	
	case am_stack_auto:
		s = "BP-";
		s += str_num(frame_sz);
		s += "+";
		s += lbl->get_name();
		break;
	
	case am_temp:
		reg_d.find(s,lbl);
		break;
	
	
	
	//error
	case am_none:
	case am_NUM:
	default: throw;
	}
}


/******************************************************************************/
//                       LOAD R-VALUES AND STORE TEMPS
/******************************************************************************/



static void Stash(reg_t reg){
	std::string open_s;
	
	// already empty
	if(!reg_d.get(reg)) return;
	
	reg_d.get_open(open_s);
	
	put_str(FORM_4,
		"mov",
		open_s.c_str(),
		str_reg(mode==xm_long?QWORD:DWORD, reg),
		"stashing"
	);
}

/** Load(reg_t reg, lbl_pt lbl)
 *	loads a label value into a register first clearing the register.
*/
static void Load(reg_t target, lbl_pt source){
	size_t ex_sz;
	std::string source_s;
	
	// check if it's already loaded
	if(reg_d.get(target) == source) return;
	
	
	// if lbl is already in a register we can use it
	reg_d.find(source_s,source);
	
	if(!source_s.empty()){
		
		// if the destination is occupied then we can XCHG
		if(reg_d.get(target)){
			if(reg_d.get(target)->get_size() > source->get_size())
				ex_sz = reg_d.get(target)->get_size();
			else ex_sz = source->get_size();
			
			put_str(FORM_4,
				"xchg",
				str_reg(ex_sz, target),
				source_s.c_str(),
				source->get_name()
			);
		}
		
		// else we just move it
		else{
			put_str(FORM_4,
				"mov",
				str_reg(source->get_size(), target),
				source_s.c_str(),
				source->get_name()
			);
		}
		
		reg_d.set(target, source);
		return;
	}
	
	// If we're still here then we have to get the symbol from memory.
	Stash(target);
	
	Resolve(source_s, source);
	
	put_str(FORM_4,
		"mov",
		str_reg(source->get_size(), target),
		source_s.c_str(),
		"Loading"
	);
	
	reg_d.set(target, source);
}


/******************************************************************************/
//                            INSTRUCTION MACROS
/******************************************************************************/


/* Because many x86 instructions require the use of specific registers each instruction macro must have a way to clear the registers it needs and have the previous value stored if it is still live.
*/



static inline void ass(lbl_pt dest, lbl_pt source){
	std::string dest_s, source_s;
	
	if(dest->get_size() < source->get_size())
		msg_print(NULL, V_WARN, "Assignment may cause overflow");
	
	Resolve(dest_s, dest);
	Resolve(source_s, source);
	
	reftoval(dest_s);
	
	op_size(dest_s, dest);
	op_size(source_s, source);
	
	put_str(FORM_4,
		"mov",   // FIXME: zero or sign extend?
		dest_s.c_str(),
		source_s.c_str(),
		"ass()"
	);
}

static inline void binary_l(inst_code op, lbl_pt dest, lbl_pt source){
	std::string dest_s, source_s;
	
	Resolve(dest_s, dest);
	Resolve(source_s, source);
	
	//FIXME: both cannot be memory locations
	
	switch(op){
	case i_add: put_str(FORM_4,
		"add",
		dest_s.c_str(),
		source_s.c_str(),
		""
	); break;
	case i_sub: put_str(FORM_4,
		"sub",
		dest_s.c_str(),
		source_s.c_str(),
		""
	); break;
	case i_and: put_str(FORM_4,
		"and",
		dest_s.c_str(),
		source_s.c_str(),
		""
	); break;
	case i_or : put_str(FORM_4,
		"or",
		dest_s.c_str(),
		source_s.c_str(),
		""
	); break;
	case i_xor: put_str(FORM_4,
		"xor",
		dest_s.c_str(),
		source_s.c_str(),
		""
	); break;
	}
}

static inline void
binary_r(inst_code op, lbl_pt dest, lbl_pt left, lbl_pt right){
	std::string right_s;
	
	Resolve(right_s, right);
	
	Load(A, left);
	
	switch(op){
	case i_add: put_str(FORM_4,
		"add",
		str_reg(dest->get_size(), A),
		right_s.c_str(),
		""
	); break;
	case i_sub: put_str(FORM_4,
		"sub",
		str_reg(dest->get_size(), A),
		right_s.c_str(),
		""
	); break;
	case i_and: put_str(FORM_4,
		"and",
		str_reg(dest->get_size(), A),
		right_s.c_str(),
		""
	); break;
	case i_or : put_str(FORM_4,
		right_s.c_str(),
		str_reg(dest->get_size(), A),
		right_s.c_str(),
		""
	); break;
	case i_xor: put_str(FORM_4,
		"xor",
		str_reg(dest->get_size(), A),
		right_s.c_str(),
		""
	); break;
	}
	
	reg_d.set(A, dest);
	
}

static inline void call(lbl_pt result, lbl_pt proc){
	put_str(FORM_3, "call", proc->get_name(), "call()");
	reg_d.set(A, result);
}

static inline void cpy(lbl_pt dest, lbl_pt source, lbl_pt cnt){
	//FIXME: cpy()
}

static inline void dec(lbl_pt target){
	std::string target_s;
	
	Resolve(target_s, target);
	
	reftoval(target_s);
	
	put_str(FORM_3,
		"dec",
		target_s.c_str(),
		"dec()"
	);
}

static inline void
div(inst_code op, lbl_pt dest, lbl_pt dividend, lbl_pt divisor){
	Load(A, dividend);
	Load(D, divisor);
	
	if(
		static_cast<Primative*>(dividend->get_def())->is_signed() ||
		static_cast<Primative*>(divisor ->get_def())->is_signed()
	)
		 put_str(FORM_3, "idiv", str_reg(divisor->get_size(), D), "");
	else put_str(FORM_3, "div" , str_reg(divisor->get_size(), D), "");
	
	if(op == i_div){
		reg_d.set(A, dest);
		reg_d.clear(divisor);
	}else{
		reg_d.set(D, dest);
		reg_d.clear(dividend);
	}
}

static inline void inc(lbl_pt target){
	std::string target_s;
	
	Resolve(target_s, target);
	
	reftoval(target_s);
	
	put_str(FORM_3,
		"inc",
		target_s.c_str(),
		"dec()"
	);
}



//static inline void inv_l(sym_pt arg){
//	resolve_prime(arg);
//	put_str(FORM_3, "not", "FIXME", "");
//}

//static inline void inv_r(sym_pt result, sym_pt arg){
//	Load_prime(A, arg);
//	put_str(FORM_3, "not", "FIXME", "");
//	reg_d.set_val(A, result);
//}



//static inline void mul(sym_pt result, sym_pt dest, sym_pt source){
//	Load_prime(A, dest);
//	Load_prime(D, source);
//	
//	if(dest->is_signed() || source->is_signed())
//		put_str(FORM_3, "imul", str_reg(source->get_size(), D), "");
//	else put_str(FORM_3, "mul", str_reg(source->get_size(), D), "");
//	
//	// TODO: check for overflow
//	
//	reg_d.set_val(D, result);
//}

//static inline void neg_l(sym_pt arg){
//	Load_prime(A, arg);
//	put_str(FORM_3, "neg", "FIXME", "");
//}

//static inline void neg_r(sym_pt result, sym_pt arg){
//	Load_prime(A, arg);
//	put_str(FORM_3, "neg", "FIXME", "");
//	reg_d.set_val(A, result);
//}


static inline void ret(lbl_pt val){
	Load(A, val);
	put_str(FORM_2, "leave", "");
	put_str(FORM_3, "ret", str_num(param_sz), "");
}

static inline void shift_l(inst_code op, lbl_pt dest, lbl_pt count){
	std::string dest_s, count_s;
	
	Resolve(dest_s, dest);
	
	if(count->get_mode() == am_constant)
		Resolve(count_s, count);
	else {
		Load(C, count);
		count_s=str_reg(WORD, C);
	}
	
	switch(op){
	case i_shl: put_str(FORM_4,
		"shl",
		dest_s.c_str(),
		count_s.c_str(),
		""
	); break;
	case i_shr: put_str(FORM_4,
		"shr",
		dest_s.c_str(),
		count_s.c_str(),
		""
	); break;
	case i_rol: put_str(FORM_4,
		"rol",
		dest_s.c_str(),
		count_s.c_str(),
		""
	); break;
	case i_ror: put_str(FORM_4,
		"ror",
		dest_s.c_str(),
		count_s.c_str(),
		""
	); break;
	}
	
	if(count->get_mode() != am_constant) reg_d.clear(count);
}

static inline void
shift_r(inst_code op, lbl_pt dest, lbl_pt source, lbl_pt count){
	std::string count_s;
	
	if(dest->get_size() != source->get_size()){
		msg_print(NULL, V_ERROR, "Internal: shift_r(): size mismatch");
		throw;
	}
	
	Stash(A);
	Load(A, source);
	
	if(count->get_mode() == am_constant)
		Resolve(count_s, count);
	else {
		Load(C, count);
		count_s=str_reg(WORD, C);
	}
	
	switch(op){
	case i_shl: put_str(FORM_4,
		"shl",
		str_reg(dest->get_size(), A),
		count_s.c_str(),
		""
	); break;
	case i_shr: put_str(FORM_4,
		"shr",
		str_reg(dest->get_size(), A),
		count_s.c_str(),
		""
	); break;
	case i_rol: put_str(FORM_4,
		"rol",
		str_reg(dest->get_size(), A),
		count_s.c_str(),
		""
	); break;
	case i_ror: put_str(FORM_4,
		"ror",
		str_reg(dest->get_size(), A),
		count_s.c_str(),
		""
	); break;
	}
	
	if(count->get_mode() != am_constant) reg_d.clear(count);
	reg_d.set(A, dest);
}

static inline void sz(lbl_pt size, lbl_pt object){
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
	case i_ass: ass(inst->dest, inst->left             ); break;
	case i_cpy: cpy(inst->dest, inst->left, inst->right); break;
	case i_inc: inc(inst->dest                         ); break;
	case i_dec: dec(inst->dest                         ); break;
	
	case i_add:
	case i_sub:
	case i_and:
	case i_or :
	case i_xor:
		if(inst->right) binary_r(inst->op, inst->dest, inst->left, inst->right);
		else            binary_l(inst->op, inst->dest, inst->left             );
		break;
	
	case i_shl:
	case i_shr:
	case i_rol:
	case i_ror:
		if(inst->right) shift_r(inst->op, inst->dest, inst->left, inst->right);
		else            shift_l(inst->op, inst->dest, inst->left             );
		break;
	
	case i_call: call(inst->dest, inst->left ); break;
	case i_ret : ret (inst->left             ); break;
	
	case i_lbl : put_lbl(inst->left); break;
	
//	case i_jmp : jmp (inst->left             ); break;
//	case i_jf  : jf  (inst->left, inst->right); break;
//	case i_jt  : jt  (inst->left, inst->right); break;
	
	case i_sz: sz(inst->dest, inst->left); break;
	
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
	
	// Initialize the register descriptor
	reg_d.clear();
	
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
	blk_pt      blk;
	
	/************** SANITY CHECKS *************/
	
	if(!lbl || !routine){
		msg_print(NULL, V_ERROR, "Gen_routine(): Got a NULL pointer");
		throw;
	}
	
	/******** TODO SORT AUTOS AND PARAMETERS ********/
	
	/* sorting them by size should guarantee memory alignment.
	*/
	
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
	
	// place the label
	put_str("\n\n");
	put_lbl(lbl);
	
	put_str(FORM_3, "enter", str_num(frame_sz), "");
	
	/**************** MAIN LOOP ****************/
	
	if(!( blk=routine->first() )){
		msg_print(NULL, V_ERROR, "Gen_proc(): Empty Routine");
		throw;
	}
	
	do Gen_blk(blk); while(( blk=routine->next() ));
	
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


