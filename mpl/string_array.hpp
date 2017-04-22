/*******************************************************************************
 *
 *	lib-util : A Utility Library
 *
 *	Copyright (c) 2016-2017 Ammon Dodson
 *	You should have received a copy of the license terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/lib-util
 *
 ******************************************************************************/

/** @file string_array.hpp
 *
 *	An array of arbitrary length, NULL terminated strings
 *
 */

#ifndef _STRING_ARRAY_HPP
#define _STRING_ARRAY_HPP

#include <util/types.h>


/******************************************************************************/
//                             TYPE DEFINITIONS
/******************************************************************************/


typedef size_t str_dx; ///< indexes into the string_array

#define NO_NAME ((str_dx)UINT_MAX)

// Exceptions
#define STRING_EXCEPTION_MEM 0 ///< Out of memory exception
#define STRING_EXCEPTION_MAX 1 ///< Overflow Exception


/******************************************************************************/
//                              CLASS DEFINITION
/******************************************************************************/


class String_Array{
private:
	char * array;
	str_dx size;
	str_dx next;
	
public:
	 String_Array(void);
	~String_Array(void);
	
	      str_dx operator+(const char * name)      ;
	      str_dx add      (const char * name)      ;
	const char * get      (const str_dx dx  ) const;
};


#endif // _STRING_ARRAY_HPP


