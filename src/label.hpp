/*******************************************************************************
 *
 *	occ : The Omega Code Compiler
 *
 *	Copyright (c) 2017 Ammon Dodson
 *
 ******************************************************************************/

/**	@file
 *	
 */


#ifndef _LABEL_HPP
#define _LABEL_HPP


// will pull in string_array.hpp to contain the various labels.
#include "string_array.hpp"


typedef enum{
	w_none,
	w_byte,
	w_byte2,
	w_byte4,
	w_byte8,
	w_max,
	w_word
} width_t;


typedef struct{
	str_dx name;
	width_t width;
}


#endif // _LABEL_HPP


