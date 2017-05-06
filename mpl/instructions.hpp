/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the license terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/MPL
 *
 ******************************************************************************/

/**	@file instructions.hpp
 *	
 *	Definitions for portable instructions
 */


#ifndef _INSTRUCTIONS_HPP
#define _INSTRUCTIONS_HPP


#include <mpl/object.hpp>

typedef struct _root* DS;

/// intermediate op codes
typedef enum {
	i_nop,
	
	// accessors
	i_cpy,  ///< A:=B
	i_stor, ///< A[i]:= temp
	i_load, ///< temp:=   A[i]
	i_ref , ///< temp:=   A+i
	i_inc , ///< temp:= ++A[i], temp:= ++temp
	i_dec , ///< temp:= --A[i], temp:= --temp


/** TODO all compound data types are ultimately composed of primes. Every prime in the compound can then be assigned an index. If indexes can be resolved to offsets then all reads and writes can be done by indexes.

Load(reg, Data*, offset);
Store(reg, Data*, offset);

all intermediate indexing values then have to be handled by the front end.


*/


	// unary ops (8)
	i_neg ,
	i_inv ,
	i_sz  ,
	i_dref, ///< <Object>, <Prime>

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

	// logical
	i_not,
	i_and,
	i_or ,

	// flow control (6)
	i_jmp,
	i_jz ,
	i_lbl,
	i_loop,
	
	i_parm,
	i_call,
	i_rtrn,
	
	
	
	i_proc, ///< declare the begining of a procedure takes a lbl and count of operands as arguments
	
	i_NUM
} inst_code;

/**	This is a Quad instruction
 */
typedef struct{
	obj_pt     result;
	obj_pt     left;
	obj_pt     right;
	inst_code op;
	bool      used_next;
} Instruction;

/// a pointer to Instruction
typedef Instruction * inst_pt;


/**	A queue of  program instructions
*/
class Block{
	DS q;
public:
	Block(void);
	~Block(void);
	
	/// The number of instructions in this block
	uint count(void)const;
	
	/// the block releases any unused memory
	void flush(void);
	
	/// returns a inst_pt to the next instruction
	inst_pt next   (void)const;
	/// returns a inst_pt to the previous instruction
	inst_pt prev   (void)const;
	/// returns a inst_pt to the first instruction
	inst_pt first  (void)const;
	/// returns a inst_pt to the last instruction
	inst_pt last   (void)const;
	
	/// enqueues an instruction
	inst_pt enqueue(inst_pt inst);
	/// removes the current instruction
	inst_pt remove (void        );
};

/// A pointer to Block.
typedef Block * blk_pt;


#endif // _INSTRUCTIONS_HPP


