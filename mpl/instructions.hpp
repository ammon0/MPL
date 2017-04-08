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
	i_rol,
	i_ror,

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
	i_jmp,
	i_jz ,
	i_lbl,
	i_loop,
	i_call,
	i_rtrn,
	
	i_NUM
} inst_code;

/**	This is a Quad instruction
 */
typedef struct{
	op_pt     result;
	op_pt     left;
	op_pt     right;
	inst_code op;
} Instruction;

typedef Instruction * inst_pt;


/**	A queue of  program instructions
*/
class Instructions{
	DS q;
	
public:
	Instructions(void);
	~Instructions(void);
	
	uint count  (void);
	bool isempty(void);
	
	inst_pt current(void);
	inst_pt next   (void);
	inst_pt prev   (void);
	inst_pt first  (void);
	inst_pt last   (void);
	
	inst_pt insert(inst_code op, op_pt result, op_pt left, op_pt right);
	inst_pt nq(inst_pt inst);
	inst_pt dq(void);
	
	void flush(void);
};

/**	A queue of basic blocks
*/
class Block_Queue{
private:
	DS bq;
public:
	 Block_Queue(void);
	~Block_Queue(void);
	
	bool           isempty(void            ) const;
	uint           count  (void            ) const;
	Instructions * first  (void            ) const;
	Instructions * last   (void            ) const;
	Instructions * next   (void            ) const;
	Instructions * nq     (Instructions * q)      ;
	Instructions * dq     (void            )      ;
};


#endif // _INSTRUCTIONS_HPP


