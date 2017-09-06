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


#ifndef _DEF_HPP
#define _DEF_HPP


#include <mpl/object.hpp>
#include <util/data.h>
#include <vector>

/*
the code generator doesn't actually need to know anything about the structure of data. the only reason these exist is because the sizes are machine dependent. we use these to calculate the sizes of various offset symbols and that's it.
*/


class Definition: public Object{
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Definition(const char * full_name): Object(full_name){}
	/******************************* ACCESSOR *********************************/
	
	/******************************* MUTATORS *********************************/
	
};

typedef Definition * def_pt;


class Data: public Definition{
	size_t size;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Data(const char * full_name): Definition(full_name){}
	
	/******************************* ACCESSOR *********************************/
	
	size_t get_size(void)const{ return size; }
	
	/******************************* MUTATORS *********************************/
	
	void set_size(size_t bytes){ size = bytes; }
};


/******************************************************************************/
//                            PRIMATIVE DATA TYPES
/******************************************************************************/


/// data sizes for numerical operands
typedef enum width_t{
	w_none,
	w_byte,
	w_byte2,
	w_byte4,
	w_byte8,
	w_max,
	w_word,
	w_ptr,
	w_NUM
} width_t;

/* The way signedness is represented is machine dependent so the machine instructions for signed and unsigned arithmetic may be different.
*/

typedef enum{
	pt_unsign,
	pt_signed
} signedness_t;

class Primative: public Data{
	width_t      width;
	signedness_t sign ;
	umax         value;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Primative();
	
	/******************************* ACCESSOR *********************************/
	
	umax    get_value(void)const{ return value; }
	width_t get_width(void)const{ return width; }
	
	obj_t        get_type(void)const{ return ot_prime; }
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	
};


/******************************************************************************/
//                                  ARRAY TYPES
/******************************************************************************/


class Array: public Data{
	def_pt child;
	umax   count;
	
public:
	std::string string_lit;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Array();
	
	/******************************* ACCESSOR *********************************/
	
	def_pt       get_child(void)const{ return child   ; }
	obj_t        get_type (void)const{ return ot_array; }
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	
};


/******************************************************************************/
//                               STRUCTURE TYPES
/******************************************************************************/


class Structure: public Data{
	DS fields;
	
public:
	
	
	/****************************** CONSTRUCTOR *******************************/
	
	Structure(void);
	~Structure(void);
	
	
	/******************************* ACCESSOR *********************************/
	
	bool isempty(void);
	umax count  (void);
	
	Data * find   (const char * name) const;
	Data * first  (void             ) const;
	Data * next   (void             ) const;
	
	obj_t        get_type(void)const{ return ot_struct; }
	const char * print(void) const{
		std::string str;
		Data * field;
		
		str = "Struct: ";
		str += get_label();
		str += "\n";
		
		if(( field = first() )){
			do{
				str += "\t";
				str += field->print();
				str += "\n";
			}while(( field = next() ));
		}
		
		str += "\n";
		
		return str.c_str();
	}
	
	/******************************* MUTATORS *********************************/
	
	Data * remove(const char * name  ); ///< Remove an object by its name
	Data * add   (      Data * object); ///< add a new object
	
};


#endif // _DEF_HPP


