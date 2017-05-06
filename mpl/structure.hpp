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

#ifndef _STRUCTURE_HPP
#define _STRUCTURE_HPP


#include <mpl/obj_container.hpp>

#define ot_struct ((obj_t) 4)


/******************************************************************************/
//                             STRUCTURE CLASS
/******************************************************************************/


class Structure: public Data{
	
public:
	Obj_List members;
	/****************************** CONSTRUCTOR *******************************/
	
	Structure(void): Data(){}
	
	/******************************* ACCESSOR *********************************/
	
	obj_t get_type (void) const{ return ot_struct; }
	
	index_t get_idx_cnt(void) const{
		Data * member;
		index_t count=0;
		
		member = members.first();
		do{
			count += member->get_idx_cnt();
		}while(( member = members.next() ));
		
		return count;
	}
	
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
	

};


#endif // _STRUCTURE_HPP


