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


#ifndef _SYM_HPP
#define _SYM_HPP

#include <util/types.h>
#include <string>

#include "def.hpp"

typedef imax offset_t;

typedef enum access_mode{
	static_pub,    ///< label
	static_priv,   ///< label
	static_extern, ///< label
	stack_fparam,  ///< BP+(2*stack_width)+label
	stack_aparam,  ///< SP+label
	stack_auto,    ///< BP-label
	temp,          ///< register or spill
	constant       ///< ??? includes offsets
} access_m;

/*	I need a way to know how much space to reserve at a symbol location
	I need a way to calculate the value of a symbol if it is a constant offset
*/

class Symbol{
	std::string label;
	access_m    m;
	def_pt      def;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	
	/******************************* ACCESSOR *********************************/
	
	const char * lbl (void)const{ return label.c_str(); }
	access_m     mode(void)const{ return m            ; }
	
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	
};

typedef Symbol * sym_pt;

#endif // _SYM_HPP


