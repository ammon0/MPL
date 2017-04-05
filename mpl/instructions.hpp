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

/**	@file instructions.hpp
 *	
 *	Definitions for machine instructions
 */


#ifndef _INSTRUCTIONS_HPP
#define _INSTRUCTIONS_HPP


#include "operands.hpp"


typedef enum {
	i_nop,

	// unary ops (8)
	i_ass ,
	i_ref ,
	i_dref,
	i_neg ,
	i_not ,
	i_inv ,
	i_inc ,
	i_dec ,
	i_sz  ,

	// binary ops (19)
	i_mul,
	i_div,
	i_mod,
	i_exp,
	i_lsh,
	i_rsh,

	i_add ,
	i_sub ,
	i_band,
	i_bor ,
	i_xor ,

	i_eq ,
	i_neq,
	i_lt ,
	i_gt ,
	i_lte,
	i_gte,

	i_and,
	i_or ,

	// flow control (6)
	i_jmp ,
	i_jz  ,
	
	i_NUM
} inst_code;

/**	This is a Quad instruction
 */
typedef struct{
	op_pt     result;
	op_pt     left;
	op_pt     right;
	inst_code inst;
} Instruction;

typedef Instruction * inst_pt;

class Instructions{
	DS q;
	
public:
	Instructions(void){ q = DS_new_list(sizeof(Instruction)); }
	~Instructions(void){ DS_delete(q); }
	
	inst_pt current(void){ return (inst_pt)DS_current (q); }
	inst_pt next   (void){ return (inst_pt)DS_next    (q); }
	inst_pt prev   (void){ return (inst_pt)DS_previous(q); }
	inst_pt last   (void){ return (inst_pt)DS_last    (q); }
	
	inst_pt insert(inst_code inst, op_pt result, op_pt left, op_pt right){
		Instruction i;
		
		i.inst   = inst;
		i.result = result;
		i.left   = left;
		i.right  = right;
		
		return (inst_pt)DS_insert(q, &i);
	}
};

#endif // _INSTRUCTIONS_HPP


