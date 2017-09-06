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

#include <mpl/instructions.hpp>
#include <mpl/def.hpp>

typedef struct _root* DS;



/******************************************************************************/
//                              ROUTINE CLASS
/******************************************************************************/


class Routine: public Definition{
	DS blocks;
	
public:
	Structure formal_params;
	Structure auto_storage;
	uint      concurrent_temps=0;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Routine(const char * full_name);
	~Routine(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool isempty(void) const;
	
	blk_pt get_first_blk(void) const;
	blk_pt get_next_blk (void) const;
	
	obj_t        get_type(void)const{ return ot_routine; }
	const char * print(void) const;
	
	/******************************* MUTATORS *********************************/
	
	inst_pt add_inst (inst_pt instruction);
};


#endif // _ROUTINE_HPP


