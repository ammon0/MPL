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

#ifndef _STRUCT_INST_HPP
#define _STRUCT_INST_HPP


#include <mpl/struct_def.hpp>

#define ot_struct_inst ((obj_t) 6)


class Struct_inst: public Data{
	Struct_def * layout;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Struct_inst();
	
	/******************************* ACCESSOR *********************************/
	
	obj_t        get_type  (void) const{ return ot_struct_inst    ; }
	size_t       get_size  (void) const{ return layout->get_size(); }
	Struct_def * get_layout(void) const{ return layout            ; }
	
};


#endif // _STRUCT_INST_HPP


