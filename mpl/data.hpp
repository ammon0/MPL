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


/// a type for offsets
typedef imax offset_t;
typedef umax index_t;

class Data: public Object{
	offset_t offset;
	/** if this is a stack parent object then the offset is from BP. If this is a
	 *	structure member then it is anonymous and this is the offset from the
	 *	parent object.
	 *	if this is the child of an array the offset should be 0.
	*/
	
	size_t bytes;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Data(): Object() { offset = 0; bytes = 0; }
	
	/******************************* ACCESSOR *********************************/
	
	offset_t get_offset(void) const{ return offset; }
	size_t   get_size  (void) const{ return bytes ; }
	
	bool is_static_data(void)const{
		if(get_sclass() == sc_private || get_sclass() == sc_public) return true;
		else return false;
	}
	
	virtual index_t get_idx_cnt(void) const=0;
	
	/******************************* MUTATORS *********************************/
	
	void set_offset(offset_t set ){ offset = set; }
	void set_size  (size_t   size){ bytes = size; }
	
};


#endif // _DATA_HPP


