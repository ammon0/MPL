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

#ifndef _STRUCTURE_HPP
#define _STRUCTURE_HPP


#include <mpl/obj_container.hpp>


/******************************************************************************/
//                             STRUCTURE CLASS
/******************************************************************************/


class Structure: public Object{
	
public:
	Obj_List members;
	/****************************** CONSTRUCTOR *******************************/
	
	/******************************* ACCESSOR *********************************/
	
	virtual obj_t get_type (void) const{ return ot_struct; }
	
	const char * print_obj (void) const{}
	
	/******************************* MUTATORS *********************************/
	

};


#endif // _STRUCTURE_HPP


