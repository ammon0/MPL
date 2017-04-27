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
	i_idx , ///< <Object>, <Object>   , <Prime>  access a member of an array
	i_mbr , ///< <Object>, <Structure>, <Object> access a member of a struct
	
	i_ref , ///< <Prime> , <Object>
	i_dref, ///< <Object>, <Prime>
	
	i_ass,
	i_cpy, ///< copy `right` bytes from `left` to `result`
	
	// unary ops (8)
	
	
	
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

/// A queue of basic blocks containing procedure information
//class Procedure{
//	DS     blocks;
//public:
//	Procedure(void);
//	~Procedure(void);
//	
//	/// is this empty?
//	bool isempty(void) const;
//	
//	/// returns the first block
//	blk_pt first(void) const;
//	/// returns the next block
//	blk_pt next (void) const;
//	
//	/**	add an instruction to the last block
//	 *	@param instruction a pointer to the instruction to add
//	 *	@return pointer to the instruction in its new location
//	 */
//	inst_pt add (inst_pt instruction);
//	
//	/// get the procedure operand
//	obj_pt  info;
//};

///// A Procedure pointer
//typedef Procedure * proc_pt;

/**	A queue of basic blocks
*/
//class Instruction_Queue{
//private:
//	DS q;
//public:
//	 Instruction_Queue(void);
//	~Instruction_Queue(void);
//	
//	/// is this empty?
//	bool isempty(void) const;
//	
//	/// get a pointer to the current procedure
//	proc_pt proc(void) const;
//	
//	/// returns the first block
//	proc_pt first(void) const;
//	/// returns the next block
//	proc_pt next (void) const;
//	
//	/// add an instruction to the last block
//	inst_pt add (inst_pt instruction);
//};


#endif // _INSTRUCTIONS_HPP


