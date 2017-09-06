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


#ifndef _OBJECT_HPP
#define _OBJECT_HPP


#include <util/types.h>
#include <string>


typedef enum{
	ot_sym,
	ot_prime,
	ot_routine,
	ot_struct,
	ot_array
} obj_t;

/******************************************************************************/
//                           ALL NAMED OBJECTS
/******************************************************************************/


class Object{
	std::string label;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Object(const char * full_name){ label = full_name; }
	
	/******************************* ACCESSOR *********************************/
	
	const char * get_label (void) const{ return label.c_str(); }
	
	virtual       obj_t  get_type(void) const=0;
	virtual const char * print   (void) const=0;
	
	/******************************* MUTATORS *********************************/
	
	void set_label(const char * full_name){ label = full_name; }
	
};

typedef Object * obj_pt;


#endif // _OBJECT_HPP


