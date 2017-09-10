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


typedef class  Label * lbl_pt;
typedef struct _root * DS;

#include <util/types.h>

/*

textual symbols in the source are macro replaced with a numerical expression such as a number for static variables, or a BP or SP relative offset. A temp symbol is also just some number that is in a register. All symbols must be explicitly dereferenced to get the "variable value". the dereference must specify the width.

Part of the problem is the inconsistent behavior of the assembler. temp labels (registers) are always l-values and so are their dereferences, while all other labels are r-values, but their dereferences are still l-values.

the `result` of most instructions will be a temp label (register). But there will be plenty of occasions where intermediate results are not necessary and the instruction could store its result in memory. It follows that any label in the `result` field must be an l-value: if it is not a temp label then it will have to be dereferenced.

LABEL TYPES
temporary label	l-value		[l-value]
constant label	r-value		[l-value]
memory label	r-value		[l-value]

INSTRUCTION FORMS
OP		DEST	LEFT	RIGHT
ass		dest	source
copy	dest	source	bytes
l_unary	arg
jmp				lbl
call	ret_val	lbl
sz		size	sym????
l_bin	dest	arg
r_unary	dest	arg
r_bin	dest	left	right


// this is the same thing
load(lbl val, lbl ref, width)
dref(lbl val, lbl ref, width)

// this is the same thing
assign(lbl ref, lbl val, width)
store (lbl ref, lbl val, width)


array[i]++ would be translated to:

T1:= i		* array_step
T2:= array	+ T1
inc [T2]

structure.field

T1:= structure	+ field

where field would have to be a predefined constant

If any variables are read into registers from memory they must be explicitly stored by front-end instructions. Stash() and Reg_man.clear() have no way to do it.

*/


/// intermediate op codes
typedef enum {
	
	/********************* MEMORY WRITES **********************/
	
	/** ass(lbl_pt dest, lbl_pt temp, NULL)
	the assignment instruction is a special case where the destination is always dereferenced.
	*/
	i_ass,
	/**	cpy(lbl_pt dest, lbl_pt source, lbl_pt width) 
	The copy instruction is special in that both the source and destination are always dereferenced.
	*/
	i_cpy,
	
	i_inc, // r++
	i_dec, // r--
	// inc and dec, much like ass, always dereference
	
	/******************************  ******************************/
	
	/// load(lbl_pt temp, lbl_pt addr, lbl_pt width)
	i_load,
	
	// non-destructive (r-values)
	// xxx(lbl_pt result, lbl_pt a, lbl_pt b)
	i_neg, // r = -a or r = -r
	i_not, // r = ~a or r = ~r
	
	i_add, // r = a + b or	r += l
	i_sub, // r = a - b or	r -= l
	i_and, // r = a & b or	r &= l
	i_or , // r = a | b or	r |= l
	i_xor, // r = a ^ b or	r ^= l
	
	i_shl, // r = a << b or	r <<= l
	i_shr, // r = a >> b or	r >>= l
	i_rol,
	i_ror,
	
	i_mul, // r = a * b
	i_div, // r = a / b
	i_mod, // r = a % b
	
	/// resz(lbl_pt temp, lbl_pt width, NULL)
	i_resz, ///< create a temp of a different size
	
	/**	sz(lbl_pt temp, lbl_pt data, NULL)
		Return the size of the data object in bytes
	*/
	i_sz,
	
	/******************************  ******************************/
	
	i_lbl,
	i_jmp,
	i_jf ,
	i_jt,
	
	/** setup(NULL, Routine * routine, NULL)
	 *	push a parameter structure onto the stack
	 */
	i_setup,
	
	/** call(Data * ret_val, Routine * routine, NULL)
	 *	push a parameter structure onto the stack
	 */
	i_call,
	
	i_ret,
	
	i_NUM
} inst_code;

/**	This is a Quad instruction
 */
typedef struct{
	lbl_pt    dest; // the destination must always be an l-value.
	lbl_pt    left;
	lbl_pt    right;
	inst_code op;
	bool      used_next=false;
} Instruction;

/// a pointer to Instruction
typedef Instruction * inst_pt;


/**	A basic block
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


