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
 *	Definitions for machine operands
 */


#ifndef _LABEL_HPP
#define _LABEL_HPP


#include <util/string_array.hpp>
#include <util/types.h>
#include <util/data.h>

#include <string.h>

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

typedef enum{
	st_none,
	st_temp,  ///< A compiler generated temporary
	st_const, ///< A compile-time constant
	st_data,  ///< A storage location
	st_code,  ///< A jump location in the code
	st_NUM
} segment_t;

/// A single MPL operand
typedef struct{
	str_dx    label;
	width_t   width;
	segment_t type;
	umax      const_value;
	bool      sign;
} Operand;

typedef Operand * op_pt;


/** This is the symbol table
*/
class Operands{
	DS index;  // from util/data.h
	
public:
	static String_Array * labels; // from util/string_array.hpp
	
	Operands(String_Array * array);
	~Operands(void);
	
	op_pt find(char * label);
	void  remove(char * label);
	op_pt add(str_dx label, width_t size, segment_t where, bool sign);
};

#endif // _LABEL_HPP


