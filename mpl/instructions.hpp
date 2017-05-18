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
	l_ass,
	
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
	 *	ref. We will have to do some kind of load operation after.
	 *	refs are used to resolve offsets
	 */
	r_ref,
	
	// destructive (l-values)
	l_neg,
	l_not,
	l_add,
	l_sub,
	l_and,
	l_or ,
	l_xor,
	l_shl,
	l_shr,
	l_rol,
	l_ror,
	l_inc,
	l_dec,
	
	// non-destructive (r-values)
	r_neg,
	r_not,
	r_add,
	r_sub,
	r_and,
	r_or ,
	r_xor,
	r_shl,
	r_shr,
	r_rol,
	r_ror,
	
	/**** OPERANDS MUST BE IN REGISTERS *****/
	
	r_mul,
	r_div,
	r_mod,
	
	/********* REDUCE TO IMMEDIATE **********/
	
	i_sz,
	
	// flow control (6)
	i_jmp,
	i_jz ,
	i_lbl,
	
	i_call,
	i_ret,
	
	i_proc, ///< declare the begining of a procedure takes a lbl and count of operands as arguments
	
	i_NUM
} inst_code;

/**	This is a Quad instruction
 */
typedef struct{
	obj_pt     result; // typically an r-value
	obj_pt     dest;   // this operand is typically overwritten
	obj_pt     source;
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


