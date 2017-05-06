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


#include <mpl/data.hpp>


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
	width_t   width;
	bool      sign;
	umax      value;
	/**	if sc_const then value is the immediate contant value. Otherwise value is
		the initialization of the variable.
	*/

public:
	/****************************** CONSTRUCTOR *******************************/

	Prime(void);

	/******************************* ACCESSOR *********************************/
	
	width_t get_width  (void) const{ return width; }
	umax    get_value  (void) const{ return value; }
	bool    is_signed  (void) const{ return sign ; }
	index_t get_idx_cnt(void) const{ return 1    ; }
	
	virtual obj_t get_type (void) const{ return ot_prime; }
	virtual const char * print_obj (void) const;
	
	/******************************* MUTATORS *********************************/
	
	void set_width   (width_t size);
	void set_count   (umax    number);
	void set_signed  (void        );
	void set_init    (umax    val );
	void set_constant(umax    val );
};


#endif // _PRIME_HPP


