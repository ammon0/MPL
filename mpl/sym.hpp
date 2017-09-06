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

#include <mpl/def.hpp>

/* A symbol is a name that represents a memory location or register. All program instructions act on symbols only. The amount of space to reserve at each symbol location is determined by querying the symbol's definition pointer.
*/

typedef enum{
	am_none         , ///< This is just to catch errors; it is not used.
	am_static_pub   , ///< label
	am_static_priv  , ///< label
	am_static_extern, ///< label
	am_stack_fparam , ///< BP+(2*stack_width)+label
	am_stack_aparam , ///< SP+label
	am_stack_auto   , ///< BP-label
	am_temp         , ///< register or spill
	am_constant     , ///< ??? includes offsets
	am_NUM            ///< This is the count of storage classes
} access_mode;


class Symbol: public Object{
	access_mode mode;
	def_pt      def;
	
protected:
	constexpr static const char * str_sclass[am_NUM]= {
		"none  ",
		"pub   ",
		"prv   ",
		"extern",
		"fparam",
		"aparam",
		"auto  ",
		"temp  ",
		"const "
	}; ///< to facilitate the print funtions
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	
	/******************************* ACCESSOR *********************************/
	
	access_mode get_mode(void)const{ return mode  ; }
	def_pt      get_def (void)const{ return def   ; }
	obj_t       get_type(void)const{ return ot_sym; }
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	
};

typedef Symbol * sym_pt;

#endif // _SYM_HPP


