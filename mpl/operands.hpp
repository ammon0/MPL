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


#include <util/string_array.hpp>
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
	
	st_const,  ///< A compile-time constant. An immediate.
	st_string, ///< A string constant
	
	st_code,   ///< A jump location in the code
	st_NUM     ///< This is the count of storage classes
} segment_t;

/// A single MPL operand
typedef struct{
	str_dx    label;
	
	segment_t type;
	
	//auto
	umax SP_offset;
	
	// temp, const, static, auto
	width_t   width;
	bool      sign;
	
	// constants
	umax      const_value;
	str_dx    string_content;
	
	bool live;
} Operand;

/// pointer to an Operand
typedef Operand * op_pt;


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
};

#endif // _LABEL_HPP


