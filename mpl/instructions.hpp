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
	/****************************** MEMORY WRITES *****************************/
	// destructive (l-values)
	
	/// ass(sym_pt dest, sym_pt temp, NULL)
	l_ass,
	
	/**	cpy(sym_pt dest, sym_pt source, sym_pt width) */
	l_cpy,
	
	// xxx(sym_pt dest, sym_pt source, NULL)
	l_neg, // a = -a
	l_not, // a = ~a
	l_add, // a += b
	l_sub, // a -= b
	l_and, // a &= b
	l_or , // a |= b
	l_xor, // a ^= b
	l_shl, // a <<= b
	l_shr, // a >>= b
	l_rol,
	l_ror,
	l_inc, // ++
	l_dec, // --
	
	/********************* READS AND REGISTER ARITHMETIC **********************/
	
	/// load(sym_pt temp, sym_pt addr, sym_pt width)
	r_load,
	
	// non-destructive (r-values)
	// xxx(sym_pt result, sym_pt a, sym_pt b)
	r_neg, // r = -a
	r_not, // r = ~a
	r_add, // r = a + b
	r_sub, // r = a - b
	r_and, // r = a & b
	r_or , // r = a | b
	r_xor, // r = a ^ b
	r_shl, // r = a << b
	r_shr, // r = a >> b
	r_rol,
	r_ror,
	
	r_mul, // r = a * b
	r_div, // r = a / b
	r_mod, // r = a % b
	
	/// resz(sym_pt temp, sym_pt width, NULL)
	r_resz, ///< create a temp of a different size
	
	/**	sz(sym_pt temp, sym_pt data, NULL)
		Return the size of the data object in bytes
	*/
	i_sz,
	
	/****************************** FLOW CONTROL ******************************/
	
	i_lbl,
	i_jmp,
	i_jz ,
	i_jnz,
	
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
	obj_pt    r;
	obj_pt    a;
	obj_pt    b;
	inst_code op;
	bool      used_next;
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


