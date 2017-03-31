/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
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
	
}

#endif // _INSTRUCTIONS_HPP


