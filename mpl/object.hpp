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
	
	// storage allocated
	sc_private, ///< private program image
	sc_public , ///< public program image
	sc_stack  , ///< A stack variable (auto)
	sc_param  , ///< A formal parameter
	
	sc_member , ///< A member of a compound object
	
	// no storage allocated
	sc_extern , ///< public program image declared in another module
	sc_temp   , ///< A compiler generated temporary
	sc_const  , ///< A compile-time constant. An immediate. inline
	sc_NUM      ///< This is the count of storage classes
} storage_class_t;

typedef uint obj_t;
#define ot_base ((obj_t) 0)


/******************************************************************************/
//                           OBJECT BASE CLASS
/******************************************************************************/


class Object{
	std::string label;
	
protected:
	storage_class_t sclass; ///< The storage class of the object
	constexpr static const char * str_sclass[sc_NUM]= {
		"none  ",
		"prv   ",
		"pub   ",
		"stack ",
		"param ",
		"membr ",
		"extern",
		"temp  ",
		"const "
	}; ///< to facilitate the print funtions
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Object(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool named(void) const{ return !label.empty(); }
	
	const char *    get_label (void) const{ return label.c_str(); }
	storage_class_t get_sclass(void) const{ return sclass       ; }
	
	virtual obj_t get_type (void) const{ return ot_base; }
	
	virtual const char * print_obj     (void) const=0;
	
	/******************************* MUTATORS *********************************/
	
	// this is virtual so it can be disabled for Routine
	virtual void set_sclass(storage_class_t storage_class);
	void set_name  (const char * name);
	
};

typedef Object * obj_pt;


#endif // _OBJECT_HPP
