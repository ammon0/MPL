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


#ifndef _ROUTINE_HPP
#define _ROUTINE_HPP

#include <mpl/obj_container.hpp>
#include <mpl/instructions.hpp>

typedef struct _root* DS;


/******************************************************************************/
//                              ROUTINE CLASS
/******************************************************************************/


class Routine: public Object{
	DS blocks;
	
public:
	Obj_List formal_params;
	Obj_List autos;
	Obj_List temps;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Routine(void);
	~Routine(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool isempty(void) const;
	
	blk_pt get_first_blk(void) const;
	blk_pt get_next_blk (void) const;
	
	obj_t get_type (void) const{ return ot_routine; }
	const char * print_obj (void) const;
	
	bool is_static_data(void) const{ return false; }
	
	/******************************* MUTATORS *********************************/
	
	void set_sclass(storage_class_t storage_class);
	
	inst_pt add_inst (inst_pt instruction);
};


#endif // _ROUTINE_HPP


