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


#ifndef _PRIME_HPP
#define _PRIME_HPP


#include <mpl/object.hpp>


/******************************************************************************/
//                               DEFINITIONS
/******************************************************************************/


/// data sizes for numerical operands
typedef enum width_t{
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

#define ot_prime ((obj_t) 2)


/******************************************************************************/
//                          SIMPLE DATA TYPE CLASS
/******************************************************************************/


// This is the only object that can be accessed immediately

class Prime: public Data{
	width_t width;
	size_t  bytes;
	bool    sign;
	umax    value;
	/**	if sc_const then value is the immediate contant value. Otherwise value is
		the initialization of the variable.
	*/
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Prime(void){ width=w_none; bytes=0; sign=false; };
	
	/******************************* ACCESSOR *********************************/
	
	width_t get_width(void) const{ return width; }
	size_t  get_size (void) const{ return bytes ; }
	umax    get_value(void) const{ return value; }
	bool    is_signed(void) const{ return sign ; }
	obj_t   get_type (void) const{ return ot_prime; }
	
	const char * print_obj (void) const;
	
	/******************************* MUTATORS *********************************/
	
	void set_width   (width_t size);
	void set_size    (size_t size ){ bytes = size; }
	void set_signed  (void        );
	void set_value   (umax    val );
};


#endif // _PRIME_HPP


