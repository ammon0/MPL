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


#ifndef _X86_REG_HPP
#define _X86_REG_HPP

#include <mpl/sym.hpp>

/// These are all the x86 "general purpose" registers
typedef enum{
	A,   ///< Accumulator
	B,   ///< General Purpose
	C,   ///< Counter
	D,   ///< Data
	SI,  ///< Source Index
	DI,  ///< Destination Index
	BP,  ///< Base Pointer
	SP,  ///< Stack Pointer
	R8,  ///< General Purpose
	R9,  ///< General Purpose
	R10, ///< General Purpose
	R11, ///< General Purpose
	R12, ///< General Purpose
	R13, ///< General Purpose
	R14, ///< General Purpose
	R15, ///< General Purpose
	NUM_reg
} reg_t;


typedef struct _root * DS;

class Reg_man{
	DS index;
	
	
	/* what this really becomes is a label translation from labels in the symbol index to register labels or [spilled temps].
	*/
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Reg_man(void);
	~Reg_man(void);
	
	/******************************* ACCESSOR *********************************/
	
	void  find(std::string &lbl_s, lbl_pt label);
	lbl_pt get (reg_t registr);
	void   get_open(std::string &);
	
	/******************************* MUTATORS *********************************/
	
	void set  (reg_t registr, lbl_pt label);
	void clear(lbl_pt label);
	void clear(void);
};


#endif // _X86_REG_HPP


