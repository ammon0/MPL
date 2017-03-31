/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *
 ******************************************************************************/

/**	@file operand.hpp 
 *	
 *	Definitions for machine operands
 */


#ifndef _LABEL_HPP
#define _LABEL_HPP


#include <util/string_array.hpp>
#include <util/types.h>
#include <util/data.h>

#include <strings.h>

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
	st_mem,  ///< A location in memory
	st_code, ///< A jump location in the code
	st_temp, ///< A temporary variable created by the compiler
	st_NUM
} storage_t;


typedef struct{
	str_dx    label;
	width_t   width;
	storage_t type;
} Operand;

typedef Operand * op_pt;

class Operands{
	DS           index;  // from util/data.h
	String_Array labels; // from util/string_array.hpp
	
	inline const void * label(void * op){ return labels.get( (op_pt)op->label ); }
	inline imax          cmp(const void * left, const void * right){
		return strcmp((char*) left, (char*) right);
	}
	
public:
	Labels(void) { index = DS_new_bst(sizeof(Operand), false, &label, &cmp); }
	~Labels(void){ DS_delete(index); }
};

#endif // _LABEL_HPP


