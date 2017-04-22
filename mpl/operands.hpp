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

/**	@file operands.hpp
 *	
 *	Definitions for portable operands
 */


#ifndef _LABEL_HPP
#define _LABEL_HPP


#include <mpl/string_array.hpp>

#include <util/types.h>
#include <util/data.h>

#include <string.h>

/// data sizes for numerical operands
typedef enum{
	w_none,
	w_byte,
	w_byte2,
	w_byte4,
	w_byte8,
	w_max,
	w_word,
	w_ptr,
	w_NUM
} width_t;

/// Storage classes for operands
typedef enum{
	st_none,   ///< This is just to catch errors; it is not used.
	st_temp,   ///< A compiler generated temporary
	st_static, ///< A static storage location
	st_auto,   ///< A stack variable
	st_param,  ///< A formal parameter
	
	st_const,  ///< A compile-time constant. An immediate.
	st_string, ///< A string constant
	
	st_code,   ///< A jump location in the code
	st_NUM     ///< This is the count of storage classes
} segment_t;

/// pointer to an Operand
typedef struct Operand * op_pt;

/// A single MPL operand
typedef struct Operand{
	str_dx    label;
	
	segment_t type;
	
	//auto and param
	umax BP_offset;
	
	// temp, const, static, auto
	width_t   width;
	bool      sign;
	
	// constants
	umax      const_value;
	str_dx    string_content;
	
private:
	// this stuff is for the optimizer
	typedef class Block * blk_pt;
	bool live;
	friend void Liveness(blk_pt);
	
	// these bits only apply to procedures
private:
	DS formal_params, autos;
public:
	uint param_cnt(void) const{ return DS_count(formal_params); }
	
	op_pt first_param(void) const;
	op_pt  next_param(void) const;
	op_pt first_auto (void) const;
	op_pt  next_auto (void) const;
	
	op_pt add_param(op_pt parameter);
	op_pt add_auto (op_pt auto_var);
} Operand;

/** This is the symbol table
*/
class Operands{
	DS index;  // from util/data.h
	
public:
	static String_Array * labels; // from util/string_array.hpp
	
	/// The constructor needs a pointer to the string space
	Operands(String_Array * array);
	~Operands(void);
	
	/// Find an operand given its name
	op_pt find(char * label);
	/// Remove an operand by its name
	void  remove(const char * label);
	/// Add a new operand
	op_pt add(str_dx label, width_t size, segment_t where, bool sign);
	
	/// Returns the first operand
	op_pt first(void)const;
	/// Returns the next operand
	op_pt next (void)const;
};

#endif // _LABEL_HPP


