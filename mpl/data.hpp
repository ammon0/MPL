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
	int    offset;
	size_t bytes;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Data(): Object() { offset = 0; bytes = 0; }
	
	/******************************* ACCESSOR *********************************/
	
	int    get_offset(void) const{ return offset; }
	size_t get_size  (void) const{ return bytes ; }
	
	bool is_static_data(void)const{
		if(get_sclass() == sc_private || get_sclass() == sc_public) return true;
		else return false;
	}
	
	/******************************* MUTATORS *********************************/
	
	void set_offset(int    set ){ offset = set; }
	void set_size  (size_t size){ bytes = size; }
	
};


#endif // _DATA_HPP


