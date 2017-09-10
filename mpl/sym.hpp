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


#ifndef _SYM_HPP
#define _SYM_HPP


#include <mpl/instructions.hpp>
#include <util/types.h>
#include <string>


typedef struct _root * DS;
typedef class Symbol       * sym_pt;
typedef class Definition   * def_pt;
typedef class Label        * lbl_pt;


typedef enum{
	st_label,
	st_prime,
	st_array,
	st_struct,
	st_routine
} sym_t;


#define COLLISION_CHAR ((char)'#')

/******************************************************************************/
//                            ALL NAMED OBJECTS
/******************************************************************************/


class Symbol{
	std::string name;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Symbol(const char * full_name){ name = full_name; }
	
	/******************************* ACCESSOR *********************************/
	
	const char * get_name (void) const{ return name.c_str(); }
	
	virtual       sym_t  get_type(void) const=0;
	virtual const char * print   (void) const=0;
	
	/******************************* MUTATORS *********************************/
	
	void set_label(const char * full_name){ name = full_name; }
	
};


/******************************************************************************/
//                            THE SYMBOL INDEX
/******************************************************************************/


class Sym_index{
	DS index;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Sym_index(void);
	~Sym_index(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool  isempty(void);
	
	sym_pt find (const char * name) const; ///< find an object by name
	sym_pt first(      void       ) const; ///< Returns the first object
	sym_pt next (      void       ) const; ///< Returns the next object
	
	/******************************* MUTATORS *********************************/
	
	sym_pt remove(const char   * name  ); ///< Remove an object by its name
	sym_pt add   (      sym_pt   object); ///< add a new object
	
};





/******************************************************************************/
//                          TYPE DEFINITIONS BASE CLASS
/******************************************************************************/


/*
the code generator doesn't actually need to know anything about the structure of data. the only reason these exist is because the sizes are machine dependent. we use these to calculate the sizes of various offset symbols and that's it.
*/


class Definition: public Symbol{
	size_t size=0;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Definition(const char * full_name): Symbol(full_name){}
	
	/******************************* ACCESSOR *********************************/
	
	size_t get_size(void)const{ return size; }
	
	/******************************* MUTATORS *********************************/
	
	void set_size(size_t bytes){ size = bytes; }
};





/******************************************************************************/
//                             DATA TYPES
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

class Primative: public Definition{
	width_t width;
	bool    signd;
	umax    value;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Primative();
	
	/******************************* ACCESSOR *********************************/
	
	umax    get_value(void)const{ return value; }
	width_t get_width(void)const{ return width; }
	bool    is_signed(void)const{ return signd; }
	
	sym_t        get_type(void)const{ return st_prime; }
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	
};


/******************************************************************************/
//                              MEMORY LABELS
/******************************************************************************/


/* A label is a name that represents a memory location or register. All program instructions act on labels only. The amount of space to reserve at each labeled location is determined by querying the label's definition pointer.
*/

typedef enum{
	am_none         , ///< This is just to catch errors; it is not used.
	am_static_pub   , ///< label
	am_static_priv  , ///< label
	am_static_extern, ///< label
	am_stack_fparam , ///< BP+(2*stack_width)+label
	am_stack_aparam , ///< SP+label
	am_stack_auto   , ///< BP-label
	am_temp         , ///< register or spill
	am_constant     , ///< ??? includes offsets
	am_NUM            ///< This is the count of storage classes
} access_mode;


class Label: public Symbol{
	access_mode mode=am_none;
	def_pt      def=NULL;
	
public:
	bool        live=false;
	
protected:
	constexpr static const char * str_sclass[am_NUM]= {
		"none  ",
		"pub   ",
		"prv   ",
		"extern",
		"fparam",
		"aparam",
		"auto  ",
		"temp  ",
		"const "
	}; ///< to facilitate the print funtions
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Label(const char * name): Symbol(name){}
	
	/******************************* ACCESSOR *********************************/
	
	access_mode get_mode(void)const{ return mode  ; }
	def_pt      get_def (void)const{ return def   ; }
	sym_t       get_type(void)const{ return st_label; }
	size_t      get_size(void)const{ return def->get_size(); }
	umax        get_value(void)const{
		if(mode != am_constant) throw;
		return static_cast<Primative*>(def)->get_value();
	}
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	void set_def(def_pt);
	void set_mode(access_mode);
	
};


class Array: public Definition{
	def_pt child;
	umax   count;
	
public:
	std::string string_lit;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Array();
	
	/******************************* ACCESSOR *********************************/
	
	def_pt       get_child(void)const{ return child   ; }
	sym_t        get_type (void)const{ return st_array; }
	const char * print(void) const{}
	
	/******************************* MUTATORS *********************************/
	
	
};



class Structure: public Definition{
	DS fields;
	
public:
	
	
	/****************************** CONSTRUCTOR *******************************/
	
	Structure(void);
	~Structure(void);
	
	
	/******************************* ACCESSOR *********************************/
	
	bool isempty(void);
	umax count  (void);
	
	lbl_pt find   (const char * name) const;
	lbl_pt first  (void             ) const;
	lbl_pt next   (void             ) const;
	
	sym_t        get_type(void)const{ return st_struct; }
	const char * print(void) const{
		std::string str;
		lbl_pt field;
		
		str = "Struct: ";
		str += get_name();
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
	
	lbl_pt remove(const char * name  ); ///< Remove an object by its name
	lbl_pt add   (      lbl_pt object); ///< add a new object
	
};


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
	
	blk_pt first(void) const;
	blk_pt next (void) const;
	
	sym_t        get_type(void)const{ return st_routine; }
	const char * print(void) const;
	
	/******************************* MUTATORS *********************************/
	
	inst_pt add_inst (inst_pt instruction);
};


#endif // _SYM_HPP


