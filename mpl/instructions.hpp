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
	
	/************ WRITE TO MEMORY ************/
	
	/**	ass(Data * dest, Data * source, NULL) */
	i_ass,
	
	/// Same as dec
	i_inc,
	/**	dec(Prime * arg, NULL, NULL)
	 *	Increments data at the location indicated by the l-value arg.
	 */
	i_dec,
	
	/**	parm(Prime * parameter) ???
	 *	
	 */
	i_parm,
	
	/******** WRITE TO TEMP REGISTER ********/
	
	/**	dref(Data * object, Prime * reference)
	 *	converts an l-value to its corresponding r-value
	 */
	i_dref,
	
	/**	ref(Data * field, Array * array, Prime * index)
	 *	returns a reference (l-value) to the field indicated by the index. By
	 *	returning the reference no memory access is made, and we can do another
	 *	ref. We will have to do some kind of load operation.
	 *	refs are used to resolve offsets
	 */
	i_ref,
	
	
	i_neg ,
	i_inv ,
	i_add ,
	i_sub ,
	i_band,
	i_bor ,
	i_xor ,
	i_shl,
	i_shr,
	i_rol,
	i_ror,
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
	
	/**** OPERANDS MUST BE IN REGISTERS *****/
	
	i_mul,
	i_div,
	i_mod,
	
	/********* REDUCE TO IMMEDIATE **********/
	
	i_sz,
	
	// flow control (6)
	i_jmp,
	i_jz ,
	i_lbl,
	i_loop,
	
	
	i_call,
	i_ret,
	
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


