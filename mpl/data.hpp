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


#ifndef _DATA_HPP
#define _DATA_HPP


#include <mpl/object.hpp>


class Data: public Object{
	int BP_offset;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Data(): Object() { BP_offset = 0; }
	
	/******************************* ACCESSOR *********************************/
	
	int get_offset(void) const{ return BP_offset; }
	
	bool is_static_data(void)const{
		if(get_sclass() == sc_private || get_sclass() == sc_public) return true;
		else return false;
	}
	
	/******************************* MUTATORS *********************************/
	
	void set_offset(int set){ BP_offset = set; }
	
};


#endif // _DATA_HPP


