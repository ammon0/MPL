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

#ifndef _ARRAY_HPP
#define _ARRAY_HPP


#include <mpl/data.hpp>

#include <util/msg.h>

#define ot_array ((obj_t) 3)


/******************************************************************************/
//                               ARRAY CLASS
/******************************************************************************/

// HOW DO WE HANDLE ANYTHING BIGGER THAN A PRIME??????


class Array: public Data{
	umax   count; ///< How many of these in the array
	Data * child; ///< An array of what must be sc_none they are anonymous and can only be stored as temp
	
	std::string lit;
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Array(umax quantity) : Data() { count = quantity; child = NULL; }
	
	/******************************* ACCESSOR *********************************/
	
	umax   get_count(void) const{ return count; }
	Data * get_child(void) const{ return child; }
	
	index_t get_idx_cnt(void) const{
		return count*child->get_idx_cnt();
	}
	
	virtual obj_t get_type (void) const{ return ot_array; }
	
	const char * print_obj (void) const{
		std::string str;
		
		str = "Array ";
		str += get_label();
		str += ": ";
		str += get_count();
		str += " of ";
		str += child->print_obj();
		str += "\n";
		
		return str.c_str();
	}
	
	/******************************* MUTATORS *********************************/
	
	void set_count(umax number){
		if(count == 1){
			if(number != 0) count = number;
			else{
				msg_print(NULL, V_ERROR,
					"Object::set_count(): the count cannot be 0");
				throw;
			}
		}
		else{
			msg_print(NULL, V_ERROR,
				"Object::set_count(): the array size is already set");
			throw;
		}
	}
};


#endif // _ARRAY_HPP


