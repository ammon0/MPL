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


#include <mpl/object.hpp>

#include <util/msg.h>

#include <vector>

#define ot_array ((obj_t) 3)


/******************************************************************************/
//                               ARRAY CLASS
/******************************************************************************/


class Array: public Data{
	umax   count; ///< How many of these in the array
	Data * child; ///< An array of what must be sc_none they are anonymous and can only be stored as temp
	
public:
	std::vector<uint8_t> value;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Array(const char * full_name, umax quantity=0, Data * child_type=NULL)
	: Data(full_name){
		count = quantity;
		child = child_type;
	}
	
	/******************************* ACCESSOR *********************************/
	
	umax   get_count(void) const{ return count; }
	Data * get_child(void) const{ return child; }
	obj_t  get_type (void) const{ return ot_array; }
	size_t get_size (void) const{ return child? child->get_size()*count:0; }
	
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
	
	void set_child(Data * child_type){ child = child_type; }
	
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


