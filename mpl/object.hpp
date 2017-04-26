/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the licence terms with this software. If
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
	sc_none,   ///< This is just to catch errors; it is not used.
	
	sc_temp,  ///< A compiler generated temporary
	sc_data,  ///< A static storage location (static)
	sc_stack, ///< A stack variable (auto)
	sc_param, ///< A formal parameter
	
	sc_extern,
	sc_const,  ///< A compile-time constant. An immediate.
	
	sc_code,   ///< A jump location in the code
	sc_NUM     ///< This is the count of storage classes
} storage_class_t;

typedef enum{
	ot_base,
	ot_prime,
	ot_routine,
	ot_struct
} obj_t;


/******************************************************************************/
//                           OBJECT BASE CLASS
/******************************************************************************/


class Object{
	std::string label;
	
protected:
	umax            count;  ///< How many of these in the array
	storage_class_t sclass; ///< The storage class of the object
	constexpr static const char * str_sclass[sc_NUM]= {
		"none  ",
		"temp  ",
		"data  ",
		"stack ",
		"param ",
		"extern",
		"const ",
		"code  "
	}; ///< to facilitate the print funtions
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Object(void);
	
	/******************************* ACCESSOR *********************************/
	
	bool            named     (void) const{ return !label.empty(); }
	const char *    get_label (void) const{ return label.c_str(); }
	storage_class_t get_sclass(void) const{ return sclass       ; }
	umax            get_count (void) const{ return count        ; }
	
	virtual obj_t get_type (void) const{ return ot_base; }
	virtual const char * print_obj (void) const=0;
	
	/******************************* MUTATORS *********************************/
	
	virtual void set_sclass(storage_class_t storage_class);
	virtual void set_count (umax            number       );
	void set_name  (const char *    name         );
	
};

typedef Object * obj_pt;


#endif // _OBJECT_HPP


