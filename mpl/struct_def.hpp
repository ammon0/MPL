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

#ifndef _STRUCT_DEF_HPP
#define _STRUCT_DEF_HPP


#include <mpl/obj_container.hpp>

#define ot_struct_def ((obj_t) 5)

class Struct_def: public Object {
	size_t bytes;
	
public:
	Obj_List members;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Struct_def(): Object("CONSTRUCTION ERROR"){}
	Struct_def(const char * full_name): Object(full_name){}
	
	/******************************* ACCESSOR *********************************/
	
	obj_t get_type (void) const{ return ot_struct_def; }
	
	size_t get_size(void) const{ return bytes; }
	
	const char * print_obj (void) const{
		std::string str;
		obj_pt obj;
		
		str = "Struct: ";
		str += get_label();
		str += "\n";
		
		if(( obj = members.first() )){
			do{
				str += "\t";
				str += obj->print_obj();
				str += "\n";
			}while(( obj = members.next() ));
		}
		
		str += "\n";
		
		return str.c_str();
	}
	
	/******************************* MUTATORS *********************************/
	
	void set_size(size_t size){ bytes = size; }
	
};


#endif // _STRUCT_DEF_HPP


