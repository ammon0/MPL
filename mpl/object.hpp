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


#ifndef _OBJECT_HPP
#define _OBJECT_HPP


#include <util/types.h>
#include <string>


/******************************************************************************/
//                               DEFINITIONS
/******************************************************************************/


typedef enum{
	sc_none   , ///< This is just to catch errors; it is not used.
	sc_private, ///< private program image
	sc_public , ///< public program image
	sc_extern , ///< public program image declared in another module
	sc_stack  , ///< A stack variable (auto)
	sc_param  , ///< A formal parameter
	sc_temp   , ///< A compiler generated temporary
	sc_const  , ///< A compile-time constant. An immediate.
	sc_NUM      ///< This is the count of storage classes
} storage_class_t;

typedef enum{
	ot_base,
	ot_routine,
	ot_prime,
	ot_struct,
	ot_array
} obj_t;


/******************************************************************************/
//                           OBJECT BASE CLASS
/******************************************************************************/


class Object{
	std::string label;
	
protected:
	storage_class_t sclass; ///< The storage class of the object
	constexpr static const char * str_sclass[sc_NUM]= {
		"none ",
		"prv  ",
		"pub  ",
		"stack",
		"param",
		"temp ",
		"const"
	}; ///< to facilitate the print funtions
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Object(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool named(void) const{ return !label.empty(); }
	bool is_mem(void) const{
		switch(sclass){
		case sc_private:
		case sc_public :
		case sc_extern :
		case sc_stack  :
		case sc_param  :return true;
		case sc_none :
		case sc_temp :
		case sc_const: return false;
		case sc_NUM:
		default    : throw;
		}
	}
	
	const char *    get_label (void) const{ return label.c_str(); }
	storage_class_t get_sclass(void) const{ return sclass       ; }
	
	virtual obj_t get_type (void) const{ return ot_base; }
	
	virtual const char * print_obj     (void) const=0;
	virtual bool         is_static_data(void) const=0;
	
	/******************************* MUTATORS *********************************/
	
	// this is virtual so it can be disabled for Routine
	virtual void set_sclass(storage_class_t storage_class);
	void set_name  (const char * name);
	
};

typedef Object * obj_pt;


#endif // _OBJECT_HPP

